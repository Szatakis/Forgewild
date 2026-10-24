#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace forgewild {

constexpr int TileSize = 16;

enum class Tile : std::uint8_t {
    Air,
    GrassEdge,
    Grass,
    DirtTop,
    DirtMiddle,
    DirtBottom,
    Stone,
    Barrier,
    Flower0,
    Flower1,
    Flower2,
    Bush,
    HousePart0,
    HousePart1,
    HousePart2,
    HousePart3,
    HousePart4,
    HousePart5,
    HousePart6,
    HousePart7,
    HousePart8,
    HousePart9,
    HousePart10,
    HousePart11,
    HousePart12,
    HousePart13,
    HousePart14,
    HousePart15,
    HousePart16,
    HousePart17,
    HousePart18,
    HousePart19,
    HousePart20,
    HousePart21,
    HousePart22,
    HousePart23,
    Tree0Part0,
    Tree0Part1,
    Tree0Part2,
    Tree0Part3,
    Tree0Part4,
    Tree0Part5,
    Tree0Part6,
    Tree0Part7,
    Tree0Part8,
    Tree0Part9,
    Tree0Part10,
    Tree0Part11,
    Tree1Part0,
    Tree1Part1,
    Tree1Part2,
    Tree1Part3,
    Tree1Part4,
    Tree1Part5,
    Tree1Part6,
    Tree1Part7,
    Tree1Part8,
    Tree1Part9,
    Tree1Part10,
    Tree1Part11,
    TreeHiveOn,
    TreeHiveOff
};

class GameMap {
public:
    GameMap(int width, int height, Tile fill = Tile::Air);

    int width() const noexcept;
    int height() const noexcept;
    Tile get(int x, int y) const noexcept;
    void set(int x, int y, Tile tile);

private:
    int width_;
    int height_;
    std::vector<Tile> tiles_;
};

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

struct Player {
    float x = 0.0F;
    float y = 0.0F;
    float health = 100.0F;
    float stamina = 100.0F;
    float velocityY = 0.0F;
    float jumpCooldown = 0.0F;
    float longJumpCooldown = 0.0F;
    float dashCooldown = 0.0F;
    float dashTime = 0.0F;
    float animationTime = 0.0F;
    int animationFrame = 0;
    int facing = 1;
    float lastHorizontalInput = 0.0F;
    bool onGround = false;
    bool dashing = false;
};

struct PlayerInput {
    bool left = false;
    bool right = false;
    bool jump = false;
    bool longJump = false;
    bool crouch = false;
    bool sprint = false;
    bool dash = false;
};

struct PlayerPositions {
    float player0X = 24576.0F;
    float player0Y = 464.0F;
    float player1X = 24576.0F;
    float player1Y = 464.0F;
};

struct GameSave {
    GameMap map;
    PlayerPositions players;
};

GameMap generateMap(int width, int height, std::uint32_t seed, bool dungeonCleared = false);
const std::vector<std::string>& tileNames();
std::string tileName(Tile tile);
Tile tileFromName(const std::string& name);
bool isSolid(Tile tile) noexcept;
int findGroundY(const GameMap& map, int blockX) noexcept;
int playerSpawnY(const GameMap& map, int blockX) noexcept;
bool collides(const GameMap& map, const Rect& rect) noexcept;
void ensurePlayerAboveGround(Player& player, const GameMap& map);
void updatePlayer(Player& player, const GameMap& map, const PlayerInput& input, float deltaTime);
void saveGame(const std::filesystem::path& directory, const GameMap& map, const PlayerPositions& players);
GameSave loadGame(const std::filesystem::path& directory);
bool hasSave(const std::filesystem::path& directory);

} // namespace forgewild
