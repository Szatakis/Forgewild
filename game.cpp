#include "game.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace forgewild {
namespace {

constexpr float PlayerWidth = 16.0F;
constexpr float PlayerHeight = 32.0F;
constexpr float Gravity = 3600.0F;
constexpr float JumpSpeed = 600.0F;
constexpr float LongJumpSpeed = 840.0F;
constexpr float JumpCooldown = 0.35F;
constexpr float LongJumpCooldown = 1.5F;
constexpr float DashCooldown = 3.0F;
constexpr float DashDuration = 0.25F;
constexpr float MaxStamina = 100.0F;
constexpr float SprintStaminaPerSecond = 20.0F;
constexpr float DashStaminaCost = 25.0F;
constexpr float StaminaRegenerationPerSecond = 30.0F;
constexpr float DashSpeed = 480.0F;

bool isTree(Tile tile) noexcept {
    const auto value = static_cast<std::uint8_t>(tile);
    return value >= static_cast<std::uint8_t>(Tile::Tree0Part0)
        && value <= static_cast<std::uint8_t>(Tile::Tree1Part11);
}

Tile treeTile(int tree, int part) {
    return static_cast<Tile>(static_cast<int>(tree == 0 ? Tile::Tree0Part0 : Tile::Tree1Part0) + part);
}

int groundBlockY(const GameMap& map, int blockX) noexcept {
    for (int y = 0; y < map.height(); ++y) {
        if (isSolid(map.get(blockX, y))) {
            return y;
        }
    }
    return -1;
}

bool moveAxis(Player& player, const GameMap& map, float amount, bool horizontal) {
    if (amount == 0.0F) {
        return true;
    }

    const int steps = std::max(1, static_cast<int>(std::ceil(std::abs(amount))));
    const float step = amount / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        Rect next{player.x, player.y, PlayerWidth, PlayerHeight};
        if (horizontal) {
            next.x += step;
        } else {
            next.y += step;
        }
        if (collides(map, next)) {
            return false;
        }
        if (horizontal) {
            player.x = next.x;
        } else {
            player.y = next.y;
        }
    }
    return true;
}

void placeHouse(GameMap& map, int x, int y) {
    constexpr std::array<Tile, 24> parts = {
        Tile::HousePart0, Tile::HousePart1, Tile::HousePart2, Tile::HousePart3, Tile::HousePart4, Tile::HousePart5,
        Tile::HousePart6, Tile::HousePart7, Tile::HousePart8, Tile::HousePart9, Tile::HousePart10, Tile::HousePart11,
        Tile::HousePart12, Tile::HousePart13, Tile::HousePart14, Tile::HousePart15, Tile::HousePart16, Tile::HousePart17,
        Tile::HousePart18, Tile::HousePart19, Tile::HousePart20, Tile::HousePart21, Tile::HousePart22, Tile::HousePart23
    };
    for (int part = 0; part < static_cast<int>(parts.size()); ++part) {
        map.set(x + part % 6, y + part / 6, parts[static_cast<std::size_t>(part)]);
    }
}

float readPosition(const std::string& line, const std::string& expectedKey) {
    const auto separator = line.find('=');
    if (separator == std::string::npos || line.substr(0, separator) != expectedKey) {
        throw std::runtime_error("Invalid player_stats.txt entry; expected " + expectedKey);
    }
    std::size_t parsed = 0;
    const float value = std::stof(line.substr(separator + 1), &parsed);
    if (parsed != line.size() - separator - 1 || !std::isfinite(value)) {
        throw std::runtime_error("Invalid player position in player_stats.txt");
    }
    return value;
}

} // namespace

GameMap::GameMap(int width, int height, Tile fill)
    : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Map dimensions must be positive");
    }
    const auto count = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (count > std::numeric_limits<std::size_t>::max()) {
        throw std::length_error("Map is too large");
    }
    tiles_.assign(static_cast<std::size_t>(count), fill);
}

int GameMap::width() const noexcept {
    return width_;
}

int GameMap::height() const noexcept {
    return height_;
}

Tile GameMap::get(int x, int y) const noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return Tile::Air;
    }
    return tiles_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

void GameMap::set(int x, int y, Tile tile) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        throw std::out_of_range("Map tile coordinates are out of range");
    }
    tiles_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)] = tile;
}

const std::vector<std::string>& tileNames() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> result = {
            "air", "grass_le", "grass_bl", "dirt_top", "dirt_middle", "dirt_bottom", "stone", "barier",
            "flower0", "flower1", "flower2", "bush"
        };
        for (int part = 0; part < 24; ++part) {
            result.push_back("house_part" + std::to_string(part));
        }
        for (int tree = 0; tree < 2; ++tree) {
            for (int part = 0; part < 12; ++part) {
                result.push_back("tree" + std::to_string(tree) + "_part" + std::to_string(part));
            }
        }
        result.emplace_back("tree_hiveon");
        result.emplace_back("tree_hiveoff");
        return result;
    }();
    return names;
}

std::string tileName(Tile tile) {
    const auto index = static_cast<std::size_t>(tile);
    if (index >= tileNames().size()) {
        throw std::out_of_range("Unknown map tile value");
    }
    return tileNames()[index];
}

Tile tileFromName(const std::string& name) {
    const auto& names = tileNames();
    const auto found = std::find(names.begin(), names.end(), name);
    if (found == names.end()) {
        throw std::runtime_error("Unknown tile in map_data.txt: " + name);
    }
    return static_cast<Tile>(std::distance(names.begin(), found));
}

bool isSolid(Tile tile) noexcept {
    return tile == Tile::Grass || tile == Tile::DirtTop || tile == Tile::DirtMiddle
        || tile == Tile::DirtBottom || tile == Tile::Stone || tile == Tile::Barrier;
}

int findGroundY(const GameMap& map, int blockX) noexcept {
    const int ground = groundBlockY(map, blockX);
    return ground < 0 ? 0 : ground * TileSize;
}

int playerSpawnY(const GameMap& map, int blockX) noexcept {
    return findGroundY(map, blockX) - 2 * TileSize;
}

bool collides(const GameMap& map, const Rect& rect) noexcept {
    if (rect.width <= 0.0F || rect.height <= 0.0F) {
        return false;
    }
    const int firstX = static_cast<int>(std::floor(rect.x / TileSize));
    const int lastX = static_cast<int>(std::floor((rect.x + rect.width - 0.001F) / TileSize));
    const int firstY = static_cast<int>(std::floor(rect.y / TileSize));
    const int lastY = static_cast<int>(std::floor((rect.y + rect.height - 0.001F) / TileSize));
    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            if (x < 0 || x >= map.width() || y >= map.height() || (y >= 0 && isSolid(map.get(x, y)))) {
                return true;
            }
        }
    }
    return false;
}

void ensurePlayerAboveGround(Player& player, const GameMap& map) {
    player.x = std::clamp(player.x, 0.0F, static_cast<float>(map.width() * TileSize - 16));
    const int blockX = std::clamp(static_cast<int>(player.x) / TileSize, 0, map.width() - 1);
    if (!collides(map, {player.x, player.y, 16.0F, 32.0F})) {
        return;
    }
    player.y = static_cast<float>(playerSpawnY(map, blockX));
    while (player.y > 0.0F && collides(map, {player.x, player.y, 16.0F, 32.0F})) {
        player.y -= static_cast<float>(TileSize);
    }
    player.velocityY = 0.0F;
    player.onGround = false;
}

GameMap generateMap(int width, int height, std::uint32_t seed, bool dungeonCleared) {
    if (width < 32 || height < 38) {
        throw std::invalid_argument("A world must be at least 32 tiles wide and 38 tiles high");
    }

    std::mt19937 random(seed);
    std::uniform_int_distribution<int> initialHeight(24, 28);
    std::uniform_int_distribution<int> slope(-1, 1);
    std::uniform_real_distribution<float> chance(0.0F, 1.0F);
    std::uniform_int_distribution<int> treeType(0, 1);

    GameMap map(width, height);
    std::vector<int> surface(static_cast<std::size_t>(width));
    int ground = initialHeight(random);
    int plateau = 0;
    int direction = 0;
    for (int x = 0; x < width; ++x) {
        if (plateau == 0) {
            direction = slope(random);
            plateau = 3;
        }
        surface[static_cast<std::size_t>(x)] = ground;
        if (--plateau == 0) {
            ground = std::clamp(ground + direction, 20, std::min(32, height - 6));
        }
    }

    for (int x = 0; x < width; ++x) {
        const int surfaceY = surface[static_cast<std::size_t>(x)];
        for (int y = surfaceY; y < height; ++y) {
            Tile tile = Tile::Stone;
            if (y == surfaceY) {
                tile = Tile::Grass;
            } else if (y == surfaceY + 1) {
                tile = Tile::DirtTop;
            } else if (y <= surfaceY + 4) {
                tile = Tile::DirtMiddle;
            } else if (y == surfaceY + 5) {
                tile = Tile::DirtBottom;
            }
            map.set(x, y, tile);
        }
    }

    const int houseX = width / 4;
    constexpr int houseWidth = 6;
    int dungeonX = width / 4 + 200 + static_cast<int>(seed % 301U);
    dungeonX = std::clamp(dungeonX, houseX + houseWidth + 3, width - 11);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (map.get(x, y) != Tile::Grass || y == 0 || (x >= houseX && x < houseX + houseWidth)) {
                continue;
            }
            const float roll = chance(random);
            if (roll < 0.20F) {
                map.set(x, y - 1, Tile::GrassEdge);
            } else if (roll < 0.25F) {
                map.set(x, y - 1, Tile::Bush);
            } else if (roll < 0.27F) {
                map.set(x, y - 1, Tile::Flower0);
            } else if (roll < 0.29F) {
                map.set(x, y - 1, Tile::Flower1);
            } else if (roll < 0.31F) {
                map.set(x, y - 1, Tile::Flower2);
            }
            break;
        }
    }

    for (int x = 2; x < width - 2; ++x) {
        for (int y = 0; y < height; ++y) {
            const Tile groundCover = map.get(x, y);
            if (groundCover != Tile::GrassEdge && groundCover != Tile::Bush) {
                continue;
            }
            if (chance(random) < 0.25F && y > 4) {
                bool canPlace = true;
                for (int xx = x - 2; xx < x + 3 && canPlace; ++xx) {
                    for (int yy = y - 4; yy <= y; ++yy) {
                        if (isTree(map.get(xx, yy)) || (xx >= houseX && xx < houseX + houseWidth)) {
                            canPlace = false;
                            break;
                        }
                    }
                }
                if (canPlace) {
                    const int type = treeType(random);
                    map.set(x, y, treeTile(type, 10));
                    map.set(x, y - 1, treeTile(type, 7));
                    map.set(x, y - 2, treeTile(type, 4));
                    map.set(x, y - 3, treeTile(type, 1));
                    map.set(x - 1, y, treeTile(type, 9));
                    map.set(x - 1, y - 1, treeTile(type, 6));
                    map.set(x - 1, y - 2, treeTile(type, 3));
                    map.set(x - 1, y - 3, treeTile(type, 0));
                    map.set(x + 1, y, treeTile(type, 11));
                    map.set(x + 1, y - 1, treeTile(type, 8));
                    map.set(x + 1, y - 2, treeTile(type, 5));
                    map.set(x + 1, y - 3, treeTile(type, 2));
                }
            }
            break;
        }
    }

    const int houseY = groundBlockY(map, houseX);
    for (int dx = 0; dx < houseWidth; ++dx) {
        map.set(houseX + dx, houseY, Tile::Grass);
        map.set(houseX + dx, houseY + 1, Tile::DirtTop);
    }
    placeHouse(map, houseX, houseY - 4);

    int dungeonGround = 0;
    for (int dx = 0; dx < 10; ++dx) {
        dungeonGround = std::max(dungeonGround, groundBlockY(map, dungeonX + dx));
    }
    for (int dx = 0; dx < 10; ++dx) {
        for (int y = 0; y < dungeonGround; ++y) {
            map.set(dungeonX + dx, y, Tile::Air);
        }
        map.set(dungeonX + dx, dungeonGround, Tile::Grass);
        map.set(dungeonX + dx, dungeonGround + 1, Tile::DirtTop);
    }
    map.set(dungeonX, dungeonGround - 10, dungeonCleared ? Tile::TreeHiveOff : Tile::TreeHiveOn);

    for (const int x : {0, width - 1}) {
        const int grassY = groundBlockY(map, x);
        for (int y = 0; y < grassY; ++y) {
            map.set(x, y, Tile::Barrier);
        }
    }
    return map;
}

void updatePlayer(Player& player, const GameMap& map, const PlayerInput& input, float deltaTime) {
    const float dt = std::clamp(deltaTime, 0.0F, 0.05F);
    player.jumpCooldown = std::max(0.0F, player.jumpCooldown - dt);
    player.longJumpCooldown = std::max(0.0F, player.longJumpCooldown - dt);
    player.dashCooldown = std::max(0.0F, player.dashCooldown - dt);

    float horizontal = input.right ? 1.0F : (input.left ? -1.0F : 0.0F);
    player.lastHorizontalInput = horizontal;
    if (horizontal != 0.0F) {
        player.facing = horizontal > 0.0F ? 1 : -1;
    }

    float speed = 180.0F;
    if (input.crouch && player.onGround) {
        speed = 60.0F;
    } else if (input.sprint && player.onGround && horizontal != 0.0F && player.stamina > 0.0F) {
        speed = 220.0F;
        player.stamina = std::max(0.0F, player.stamina - SprintStaminaPerSecond * dt);
    }

    if (input.jump && player.onGround && player.jumpCooldown <= 0.0F) {
        player.velocityY = -JumpSpeed;
        player.jumpCooldown = JumpCooldown;
        player.onGround = false;
    } else if (input.longJump && player.onGround && player.longJumpCooldown <= 0.0F) {
        player.velocityY = -LongJumpSpeed;
        player.longJumpCooldown = LongJumpCooldown;
        player.onGround = false;
    }

    if (input.dash && player.dashCooldown <= 0.0F && !player.dashing
        && player.stamina >= DashStaminaCost) {
        player.dashing = true;
        player.dashTime = DashDuration;
        player.dashCooldown = DashCooldown;
        player.stamina -= DashStaminaCost;
    }

    if (!moveAxis(player, map, horizontal * speed * dt, true)) {
        horizontal = 0.0F;
    }

    if (player.dashing) {
        if (!moveAxis(player, map, static_cast<float>(player.facing) * DashSpeed * dt, true)) {
            player.dashing = false;
            player.dashTime = 0.0F;
        } else {
            player.dashTime -= dt;
            if (player.dashTime <= 0.0F) {
                player.dashing = false;
            }
        }
    }

    player.velocityY = std::min(player.velocityY + Gravity * dt, 1200.0F);
    if (!moveAxis(player, map, player.velocityY * dt, false)) {
        if (player.velocityY > 0.0F) {
            player.onGround = true;
        }
        player.velocityY = 0.0F;
    } else {
        player.onGround = false;
    }

    if (horizontal == 0.0F && player.onGround && !player.dashing) {
        player.stamina = std::min(MaxStamina, player.stamina + StaminaRegenerationPerSecond * dt);
    }

    player.animationTime += dt;
    if (player.animationTime >= 0.13F) {
        player.animationTime = 0.0F;
        player.animationFrame = horizontal == 0.0F ? 0 : (player.animationFrame + 1) % 3;
    }
}

void saveGame(const std::filesystem::path& directory, const GameMap& map, const PlayerPositions& players) {
    std::filesystem::create_directories(directory);
    const auto mapPath = directory / "map_data.txt";
    std::ofstream mapFile(mapPath, std::ios::binary | std::ios::trunc);
    if (!mapFile) {
        throw std::runtime_error("Could not open " + mapPath.string() + " for writing");
    }
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (x != 0) {
                mapFile << ',';
            }
            mapFile << tileName(map.get(x, y));
        }
        mapFile << '\n';
    }
    mapFile.close();
    if (!mapFile) {
        throw std::runtime_error("Could not write map data to " + mapPath.string());
    }

    const auto statsPath = directory / "player_stats.txt";
    std::ofstream statsFile(statsPath, std::ios::binary | std::ios::trunc);
    if (!statsFile) {
        throw std::runtime_error("Could not open " + statsPath.string() + " for writing");
    }
    statsFile << "player0_x=" << players.player0X << '\n' << "player0_y=" << players.player0Y << '\n' << "player1_x=" << players.player1X << '\n' << "player1_y=" << players.player1Y << '\n';
    statsFile.close();
    if (!statsFile) {
        throw std::runtime_error("Could not write player data to " + statsPath.string());
    }
}

GameSave loadGame(const std::filesystem::path& directory) {
    const auto mapPath = directory / "map_data.txt";
    std::ifstream mapFile(mapPath, std::ios::binary);
    if (!mapFile) {
        throw std::runtime_error("Could not open " + mapPath.string() + " for reading");
    }
    std::vector<std::vector<Tile>> rows;
    std::string line;
    while (std::getline(mapFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        std::vector<Tile> row;
        std::size_t start = 0;
        while (start <= line.size()) {
            const auto end = line.find(',', start);
            const auto length = end == std::string::npos ? line.size() - start : end - start;
            if (length == 0) {
                throw std::runtime_error("Empty tile name in " + mapPath.string());
            }
            row.push_back(tileFromName(line.substr(start, length)));
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        if (!rows.empty() && row.size() != rows.front().size()) {
            throw std::runtime_error("Map rows have inconsistent widths in " + mapPath.string());
        }
        rows.push_back(std::move(row));
    }
    if (rows.empty() || rows.front().empty()) {
        throw std::runtime_error("Map file is empty: " + mapPath.string());
    }

    GameMap map(static_cast<int>(rows.front().size()), static_cast<int>(rows.size()));
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            map.set(x, y, rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]);
        }
    }

    const auto statsPath = directory / "player_stats.txt";
    std::ifstream statsFile(statsPath, std::ios::binary);
    if (!statsFile) {
        throw std::runtime_error("Could not open " + statsPath.string() + " for reading");
    }
    std::array<std::string, 4> statsLines;
    for (auto& statsLine : statsLines) {
        if (!std::getline(statsFile, statsLine)) {
            throw std::runtime_error("Incomplete player data in " + statsPath.string());
        }
        if (!statsLine.empty() && statsLine.back() == '\r') {
            statsLine.pop_back();
        }
    }
    GameSave result{std::move(map), {}};
    result.players.player0X = readPosition(statsLines[0], "player0_x");
    result.players.player0Y = readPosition(statsLines[1], "player0_y");
    result.players.player1X = readPosition(statsLines[2], "player1_x");
    result.players.player1Y = readPosition(statsLines[3], "player1_y");
    return result;
}

bool hasSave(const std::filesystem::path& directory) {
    return std::filesystem::exists(directory / "map_data.txt") && std::filesystem::exists(directory / "player_stats.txt");
}

} // namespace forgewild
