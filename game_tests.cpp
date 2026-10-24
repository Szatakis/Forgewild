#include "game.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testMapGeneration() {
    const auto first = forgewild::generateMap(96, 70, 12345U);
    const auto second = forgewild::generateMap(96, 70, 12345U);
    require(first.width() == 96 && first.height() == 70, "generated map dimensions should be retained");
    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            require(first.get(x, y) == second.get(x, y), "map generation should be deterministic for a fixed seed");
        }
    }
    require(first.get(0, 0) == forgewild::Tile::Barrier, "world edge should be bounded by a barrier");
    require(forgewild::isSolid(first.get(0, forgewild::findGroundY(first, 0) / forgewild::TileSize)),
            "surface should contain solid ground");
    const int spawnX = (first.width() / 4) * forgewild::TileSize;
    const int spawnY = forgewild::playerSpawnY(first, first.width() / 4);
    require(!forgewild::collides(first, {static_cast<float>(spawnX), static_cast<float>(spawnY), 16.0F, 32.0F}),
            "new player spawn should sit above the terrain");
    bool hasHouse = false;
    bool hasDungeon = false;
    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const auto tile = first.get(x, y);
            hasHouse = hasHouse || (tile >= forgewild::Tile::HousePart0 && tile <= forgewild::Tile::HousePart23);
            hasDungeon = hasDungeon || tile == forgewild::Tile::TreeHiveOn;
        }
    }
    require(hasHouse, "generated world should contain the house");
    require(hasDungeon, "generated world should contain the tree dungeon");
}

void testCollisionAndMovement() {
    forgewild::GameMap map(20, 20);
    for (int x = 0; x < map.width(); ++x) {
        map.set(x, 10, forgewild::Tile::Grass);
    }
    require(forgewild::collides(map, {16.0F, 160.0F, 16.0F, 32.0F}), "player should collide with ground");
    require(!forgewild::collides(map, {16.0F, 100.0F, 16.0F, 32.0F}), "air above the ground should be clear");
    require(forgewild::collides(map, {-1.0F, 100.0F, 16.0F, 32.0F}), "world edge should block the player");

    forgewild::Player player;
    player.x = 32.0F;
    player.y = 128.0F;
    forgewild::PlayerInput input;
    input.right = true;
    forgewild::updatePlayer(player, map, input, 1.0F / 60.0F);
    require(player.x > 32.0F, "horizontal input should move the player");
    input = {};
    player.y = 128.0F;
    player.onGround = true;
    input.jump = true;
    forgewild::updatePlayer(player, map, input, 1.0F / 60.0F);
    require(player.velocityY < 0.0F, "jump input should apply upward velocity");

    player.x = 32.0F;
    player.y = 160.0F;
    player.velocityY = 500.0F;
    forgewild::ensurePlayerAboveGround(player, map);
    require(!forgewild::collides(map, {player.x, player.y, 16.0F, 32.0F}),
            "a saved player embedded in the ground should be moved above it");
    require(player.y == 128.0F && player.velocityY == 0.0F,
            "ground recovery should place the player on the surface and clear falling speed");
}

void testStamina() {
    forgewild::GameMap map(20, 20);
    for (int x = 0; x < map.width(); ++x) {
        map.set(x, 10, forgewild::Tile::Grass);
    }

    forgewild::Player sprinter;
    sprinter.x = 32.0F;
    sprinter.y = 128.0F;
    sprinter.onGround = true;
    forgewild::PlayerInput sprintInput;
    sprintInput.right = true;
    sprintInput.sprint = true;
    forgewild::updatePlayer(sprinter, map, sprintInput, 0.05F);
    require(sprinter.stamina < 100.0F && sprinter.stamina > 98.0F,
            "sprinting should consume stamina");

    forgewild::Player dasher;
    dasher.x = 32.0F;
    dasher.y = 128.0F;
    dasher.onGround = true;
    forgewild::PlayerInput dashInput;
    dashInput.dash = true;
    forgewild::updatePlayer(dasher, map, dashInput, 0.05F);
    require(dasher.dashing && std::abs(dasher.stamina - 75.0F) < 0.01F,
            "starting a dash should consume its stamina cost");

    forgewild::Player resting;
    resting.x = 32.0F;
    resting.y = 128.0F;
    resting.onGround = true;
    resting.stamina = 50.0F;
    forgewild::updatePlayer(resting, map, {}, 0.05F);
    require(resting.stamina > 50.0F,
            "standing still on the ground should restore stamina");

    forgewild::Player exhausted;
    exhausted.x = 32.0F;
    exhausted.y = 128.0F;
    exhausted.onGround = true;
    exhausted.stamina = 0.0F;
    sprintInput = {};
    sprintInput.right = true;
    sprintInput.sprint = true;
    forgewild::updatePlayer(exhausted, map, sprintInput, 0.05F);
    require(std::abs(exhausted.x - 41.0F) < 0.01F && exhausted.stamina == 0.0F,
            "sprinting with an empty stamina bar should use normal movement speed");

    forgewild::Player tooTiredToDash;
    tooTiredToDash.x = 32.0F;
    tooTiredToDash.y = 128.0F;
    tooTiredToDash.onGround = true;
    tooTiredToDash.stamina = 24.0F;
    dashInput = {};
    dashInput.right = true;
    dashInput.dash = true;
    forgewild::updatePlayer(tooTiredToDash, map, dashInput, 0.05F);
    require(!tooTiredToDash.dashing && std::abs(tooTiredToDash.stamina - 24.0F) < 0.01F,
            "a dash should not start when stamina is below its cost");
}

void testSaveRoundTrip() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("forgewild-tests-" + std::to_string(stamp));
    try {
        const auto map = forgewild::generateMap(48, 70, 99U);
        const forgewild::PlayerPositions positions{123.5F, 456.25F, 789.0F, 321.0F};
        forgewild::saveGame(directory, map, positions);
        require(forgewild::hasSave(directory), "saved world should be discoverable");
        const auto loaded = forgewild::loadGame(directory);
        require(loaded.map.width() == map.width() && loaded.map.height() == map.height(),
                "saved map dimensions should round-trip");
        for (int y = 0; y < map.height(); ++y) {
            for (int x = 0; x < map.width(); ++x) {
                require(loaded.map.get(x, y) == map.get(x, y), "saved map tiles should round-trip");
            }
        }
        require(std::abs(loaded.players.player0X - positions.player0X) < 0.01F,
                "player 0 X coordinate should round-trip");
        require(std::abs(loaded.players.player1Y - positions.player1Y) < 0.01F,
                "player 1 Y coordinate should round-trip");
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
        throw;
    }
    std::filesystem::remove_all(directory);
}

} // namespace

int main() {
    try {
        testMapGeneration();
        testCollisionAndMovement();
        testStamina();
        testSaveRoundTrip();
        std::cout << "All Forgewild game logic tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
