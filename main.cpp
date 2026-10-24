#include "game.h"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using namespace forgewild;
constexpr int ScreenWidth = 960;
constexpr int ScreenHeight = 540;
constexpr int MapWidth = 12000;
constexpr int MapHeight = 70;
constexpr Color PanelColor{12, 24, 34, 225};
constexpr Color AccentColor{138, 205, 125, 255};

enum class Screen {
    MainMenu,
    PlayMenu,
    Settings,
    Credits,
    Game,
    Pause,
    DeleteSave,
    VolumeSettings,
    LanguageSettings,
    QualitySettings,
    AutosaveSettings,
    Tutorial,
    SaveFolder
};

struct Button {
    Rectangle bounds;
    const char* label;
};

std::filesystem::path locateAssets() {
    const std::array<std::filesystem::path, 3> candidates = {
        std::filesystem::path(GetApplicationDirectory()),
        std::filesystem::current_path(),
        std::filesystem::path(FORGEWILD_DATA_DIR)
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "Textures" / "Menu" / "bgmenu.png")
            && std::filesystem::exists(candidate / "PDD")) {
            return candidate;
        }
    }
    throw std::runtime_error("Could not find the game data folder next to the executable.");
}

class TextureStore {
public:
    explicit TextureStore(const std::filesystem::path& root) {
        for (const auto& name : tileNames()) {
            load("tile:" + name, root / "Textures" / "Tiles" / (name + ".png"));
        }
        for (const auto& name : {
                 "bgmenu", "bgplay", "bgsettings", "bgcredits", "bgpausemenu",
                 "hud", "hudinv", "spopup", "volumepopup", "languagepopup",
                 "qualitypopup", "autosavepopup"
             }) {
            load(name, root / "Textures" / "Menu" / (std::string(name) + ".png"));
        }
        load("background:night", root / "Textures" / "Tiles" / "BG" / "bggame_night.png");
        load("background:day", root / "Textures" / "Tiles" / "BG" / "bggame_day.png");
        load("cursor", root / "Textures" / "cursor.png");
        for (int player = 0; player < 2; ++player) {
            const auto folder = root / "Textures" / ("Player" + std::to_string(player));
            load(playerKey(player, "idle"), folder / "idle.png");
            for (int frame = 1; frame <= 3; ++frame) {
                load(playerKey(player, "left" + std::to_string(frame)), folder / ("left" + std::to_string(frame) + ".png"));
                load(playerKey(player, "right" + std::to_string(frame)), folder / ("right" + std::to_string(frame) + ".png"));
                for (const auto& direction : {"idle", "left", "right"}) {
                    const auto name = "jump" + std::to_string(frame) + "_" + direction;
                    load(playerKey(player, name), folder / (name + ".png"));
                }
            }
        }
        for (const auto& item : {"sword", "berry", "leaf", "stick", "wood", "wand"}) {
            load("item:" + std::string(item), root / "Textures" / "Items" / (std::string(item) + ".png"));
        }
        const auto fontPath = root / "font.TTF";
        font_ = LoadFontEx(fontPath.string().c_str(), 24, nullptr, 0);
        if (font_.texture.id == 0) {
            throw std::runtime_error("Could not load font: " + fontPath.string());
        }
    }

    ~TextureStore() {
        for (const auto& [name, texture] : textures_) {
            (void)name;
            UnloadTexture(texture);
        }
        if (font_.texture.id != 0) {
            UnloadFont(font_);
        }
    }

    Texture2D get(const std::string& key) const {
        const auto found = textures_.find(key);
        if (found == textures_.end()) {
            throw std::runtime_error("Texture is not loaded: " + key);
        }
        return found->second;
    }

    Font font() const noexcept {
        return font_;
    }

    void setFiltering(TextureFilter filter) const {
        for (const auto& [name, texture] : textures_) {
            (void)name;
            SetTextureFilter(texture, filter);
        }
    }

private:
    void load(const std::string& key, const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("Missing game texture: " + path.string());
        }
        const auto texture = LoadTexture(path.string().c_str());
        if (texture.id == 0) {
            throw std::runtime_error("Could not load texture: " + path.string());
        }
        textures_.emplace(key, texture);
    }

    static std::string playerKey(int player, const std::string& name) {
        return "player" + std::to_string(player) + ":" + name;
    }

    std::unordered_map<std::string, Texture2D> textures_;
    Font font_{};
};

class MusicPlayer {
public:
    explicit MusicPlayer(const std::filesystem::path& root) : root_(root) {}

    ~MusicPlayer() {
        stop();
    }

    void play(const std::filesystem::path& relativePath, bool loop, float volume) {
        const auto path = root_ / relativePath;
        if (activePath_ == path && loaded_) {
            SetMusicVolume(music_, volume);
            music_.looping = loop;
            return;
        }
        stop();
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("Missing music file: " + path.string());
        }
        music_ = LoadMusicStream(path.string().c_str());
        if (music_.frameCount == 0) {
            throw std::runtime_error("Could not load music: " + path.string());
        }
        music_.looping = loop;
        SetMusicVolume(music_, volume);
        PlayMusicStream(music_);
        activePath_ = path;
        loaded_ = true;
    }

    void update() {
        if (loaded_) {
            UpdateMusicStream(music_);
        }
    }

    void setVolume(float volume) {
        if (loaded_) {
            SetMusicVolume(music_, volume);
        }
    }

    bool finished() const {
        return loaded_ && !music_.looping && !IsMusicStreamPlaying(music_);
    }

    void stop() {
        if (loaded_) {
            StopMusicStream(music_);
            UnloadMusicStream(music_);
            music_ = {};
            activePath_.clear();
            loaded_ = false;
        }
    }

private:
    std::filesystem::path root_;
    std::filesystem::path activePath_;
    Music music_{};
    bool loaded_ = false;
};

void drawTexture(Texture2D texture, float x, float y, float width, float height, Color tint = WHITE) {
    const Rectangle source{0.0F, 0.0F, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    const Rectangle destination{x, y, width, height};
    DrawTexturePro(texture, source, destination, Vector2{0.0F, 0.0F}, 0.0F, tint);
}

void drawText(Font font, const std::string& text, float x, float y, float size, Color color = RAYWHITE) {
    DrawTextEx(font, text.c_str(), Vector2{x, y}, size, 1.0F, color);
}

void drawButton(Font font, const Button& button, Vector2 mouse, bool whiteHover = false) {
    const bool hovered = CheckCollisionPointRec(mouse, button.bounds);
    if (whiteHover) {
        DrawRectangleRounded(button.bounds, 0.12F, 6, Color{28, 24, 55, 220});
        if (hovered) {
            DrawRectangleRounded({button.bounds.x - 4.0F, button.bounds.y - 4.0F,
                                  button.bounds.width + 8.0F, button.bounds.height + 8.0F},
                                 0.12F, 6, Color{255, 255, 255, 18});
            DrawRectangleRounded({button.bounds.x - 2.0F, button.bounds.y - 2.0F,
                                  button.bounds.width + 4.0F, button.bounds.height + 4.0F},
                                 0.12F, 6, Color{255, 255, 255, 28});
            DrawRectangleRounded(button.bounds, 0.12F, 6, Color{255, 255, 255, 38});
            DrawRectangleRoundedLines(button.bounds, 0.12F, 6, Color{255, 255, 255, 110});
        } else {
            DrawRectangleRoundedLines(button.bounds, 0.12F, 6, Color{205, 205, 225, 150});
        }
    } else {
        DrawRectangleRounded(button.bounds, 0.12F, 6, hovered ? Color{39, 75, 64, 240} : PanelColor);
        DrawRectangleRoundedLines(button.bounds, 0.12F, 6,
                                  hovered ? AccentColor : Color{104, 137, 119, 255});
    }
    const float textWidth = MeasureTextEx(font, button.label, 21.0F, 1.0F).x;
    drawText(font, button.label, button.bounds.x + (button.bounds.width - textWidth) / 2.0F,
             button.bounds.y + (button.bounds.height - 24.0F) / 2.0F, 21.0F);
}

bool clicked(const Button& button, Vector2 mouse) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, button.bounds);
}

std::string tileTextureKey(Tile tile) {
    return "tile:" + tileName(tile);
}

class Application {
public:
    Application(std::filesystem::path root, RenderTexture2D target)
        : root_(std::move(root)), textures_(root_), music_(root_), target_(target) {
        jumpSound_ = LoadSound((root_ / "Audio" / "SFX" / "jump.wav").string().c_str());
        dashSound_ = LoadSound((root_ / "Audio" / "SFX" / "dashend.wav").string().c_str());
        if (jumpSound_.frameCount == 0 || dashSound_.frameCount == 0) {
            throw std::runtime_error("Could not load the player sound effects.");
        }
        SetSoundVolume(jumpSound_, 0.35F);
        SetSoundVolume(dashSound_, 0.35F);
        music_.play(std::filesystem::path("Audio") / "Music" / "menu.wav", true,
                    masterVolume_ * musicVolume_);
    }

    ~Application() {
        music_.stop();
        if (jumpSound_.frameCount != 0) {
            UnloadSound(jumpSound_);
        }
        if (dashSound_.frameCount != 0) {
            UnloadSound(dashSound_);
        }
    }

    void run() {
        while (!WindowShouldClose() && !shouldExit_) {
            const float dt = std::min(GetFrameTime(), 0.05F);
            handleGlobalInput();
            update(dt);
            draw();
        }
        if (worldLoaded_) {
            saveWorld();
        }
    }

private:
    std::filesystem::path saveDirectory() const {
        return root_ / "PDD";
    }

    void handleGlobalInput() {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (screen_ == Screen::Game) {
                screen_ = Screen::Pause;
            } else if (screen_ == Screen::Pause) {
                screen_ = Screen::Game;
            } else if (screen_ == Screen::Settings) {
                screen_ = settingsFromPause_ ? Screen::Pause : Screen::MainMenu;
            } else if (screen_ == Screen::VolumeSettings
                       || screen_ == Screen::LanguageSettings
                       || screen_ == Screen::QualitySettings
                       || screen_ == Screen::AutosaveSettings) {
                screen_ = settingsFromPause_ ? Screen::Pause : Screen::Settings;
            } else if (screen_ == Screen::Tutorial || screen_ == Screen::SaveFolder) {
                screen_ = Screen::Settings;
            } else if (screen_ == Screen::DeleteSave) {
                screen_ = screenBeforeDelete_;
            } else if (screen_ != Screen::MainMenu) {
                screen_ = Screen::MainMenu;
            }
        }
    }

    void beginGame(bool newWorld) {
        if (newWorld || !hasSave(saveDirectory())) {
            world_ = generateMap(MapWidth, MapHeight,
                static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
            const int spawnX = (world_.width() / 4) * TileSize;
            const int spawnY = playerSpawnY(world_, world_.width() / 4);
            positions_ = {static_cast<float>(spawnX), static_cast<float>(spawnY),
                          static_cast<float>(spawnX), static_cast<float>(spawnY)};
            saveWorld();
        } else {
            auto save = loadGame(saveDirectory());
            world_ = std::move(save.map);
            positions_ = save.players;
        }
        worldLoaded_ = true;

        player0_ = Player{};
        player1_ = Player{};
        player0_.x = positions_.player0X;
        player0_.y = positions_.player0Y;
        player1_.x = positions_.player1X;
        player1_.y = positions_.player1Y;
        ensurePlayerAboveGround(player0_, world_);
        ensurePlayerAboveGround(player1_, world_);
        saveWorld();
        coopEnabled_ = false;
        inventory_.fill("");
        inventory_[0] = "sword";
        selectedSlot_ = 0;
        inventoryOpen_ = false;
        nextMusicTrack_ = 0;
        screen_ = Screen::Game;
        music_.stop();
        music_.play(gameTrack(nextMusicTrack_), false, masterVolume_ * musicVolume_);
    }

    std::filesystem::path gameTrack(int index) const {
        return std::filesystem::path("Audio") / "Music" / "Game" / (std::to_string(index + 1) + ".wav");
    }

    void saveWorld() {
        positions_ = {player0_.x, player0_.y, player1_.x, player1_.y};
        saveGame(saveDirectory(), world_, positions_);
        lastSaveTime_ = GetTime();
        saveMessageTime_ = GetTime();
    }

    void deleteWorld() {
        std::error_code error;
        for (const auto& file : {"map_data.txt", "player_stats.txt"}) {
            std::filesystem::remove(saveDirectory() / file, error);
            if (error) {
                throw std::runtime_error("Could not delete save file: " + error.message());
            }
        }
        worldLoaded_ = false;
        screen_ = screenBeforeDelete_;
    }

    PlayerInput firstPlayerInput() const {
        return {
            IsKeyDown(KEY_A), IsKeyDown(KEY_D),
            IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W), IsKeyDown(KEY_E),
            IsKeyDown(KEY_S), IsKeyDown(KEY_LEFT_SHIFT), IsKeyPressed(KEY_R)
        };
    }

    PlayerInput secondPlayerInput() const {
        return {
            IsKeyDown(KEY_LEFT), IsKeyDown(KEY_RIGHT),
            IsKeyDown(KEY_UP), IsKeyDown(KEY_SLASH), IsKeyDown(KEY_DOWN),
            false, IsKeyPressed(KEY_BACKSLASH)
        };
    }

    void update(float dt) {
        music_.update();
        const bool playingGame = screen_ == Screen::Game;
        const int desiredMusicMode = playingGame ? 1 : 0;
        if (musicMode_ != desiredMusicMode) {
            musicMode_ = desiredMusicMode;
            if (playingGame) {
                music_.play(gameTrack(nextMusicTrack_), false, masterVolume_ * musicVolume_);
            } else {
                music_.play(std::filesystem::path("Audio") / "Music" / "menu.wav", true,
                            masterVolume_ * musicVolume_);
            }
        }
        if (playingGame) {
            if (music_.finished()) {
                nextMusicTrack_ = (nextMusicTrack_ + 1) % 3;
                music_.play(gameTrack(nextMusicTrack_), false, masterVolume_ * musicVolume_);
            }
        }

        if (screen_ == Screen::Game) {
            if (IsKeyPressed(KEY_TAB)) {
                inventoryOpen_ = !inventoryOpen_;
            }
            if (IsKeyPressed(KEY_F2)) {
                coopEnabled_ = !coopEnabled_;
                if (coopEnabled_ && player1_.x == player0_.x && player1_.y == player0_.y) {
                    player1_.x = player0_.x + 24.0F;
                    player1_.y = player0_.y;
                }
            }
            if (IsKeyPressed(KEY_F5)) {
                saveWorld();
            }
            if (IsKeyPressed(KEY_Q)) {
                inventory_[static_cast<std::size_t>(selectedSlot_)].clear();
            }
            for (int slot = 0; slot < 8; ++slot) {
                if (IsKeyPressed(KEY_ONE + slot)) {
                    selectedSlot_ = slot;
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                const Vector2 mouse = logicalMouse();
                const Rectangle pauseButton{772.0F, 23.0F, 38.0F, 38.0F};
                if (CheckCollisionPointRec(mouse, pauseButton)) {
                    screen_ = Screen::Pause;
                }
                if (inventoryOpen_) {
                    const int slot = inventorySlotAt(mouse);
                    if (slot >= 0) {
                        if (slot < 8) {
                            selectedSlot_ = slot;
                        }
                        if (!inventory_[static_cast<std::size_t>(slot)].empty()) {
                            draggingInventory_ = true;
                            draggedFrom_ = slot;
                        }
                    }
                }
            }
            if (inventoryOpen_ && draggingInventory_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                const int target = inventorySlotAt(logicalMouse());
                if (target >= 0 && target != draggedFrom_) {
                    std::swap(inventory_[static_cast<std::size_t>(target)],
                              inventory_[static_cast<std::size_t>(draggedFrom_)]);
                }
                draggingInventory_ = false;
                draggedFrom_ = -1;
            }

            PlayerInput input0 = firstPlayerInput();
            const PlayerInput input1 = secondPlayerInput();
            if (((input0.jump && player0_.jumpCooldown <= 0.0F)
                 || (input0.longJump && player0_.longJumpCooldown <= 0.0F))
                && player0_.onGround) {
                PlaySound(jumpSound_);
            }
            if (input0.dash && player0_.dashCooldown <= 0.0F) {
                PlaySound(dashSound_);
            }
            updatePlayer(player0_, world_, input0, dt);
            if (coopEnabled_) {
                updatePlayer(player1_, world_, input1, dt);
            }
            if (autoSaveMinutes_ > 0 && GetTime() - lastSaveTime_ > autoSaveMinutes_ * 60.0) {
                saveWorld();
            }
        }

        if (screen_ == Screen::VolumeSettings && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse = logicalMouse();
            if (mouse.y >= 189.0F && mouse.y <= 222.0F) {
                masterVolume_ = sliderValue(mouse.x);
            } else if (mouse.y >= 266.0F && mouse.y <= 303.0F) {
                musicVolume_ = sliderValue(mouse.x);
            } else if (mouse.y >= 355.0F && mouse.y <= 392.0F) {
                soundVolume_ = sliderValue(mouse.x);
            }
            music_.setVolume(masterVolume_ * musicVolume_);
            SetSoundVolume(jumpSound_, soundVolume_ * masterVolume_);
            SetSoundVolume(dashSound_, soundVolume_ * masterVolume_);
        }
    }

    static float sliderValue(float x) {
        return std::clamp((x - 375.0F) / 418.0F, 0.0F, 1.0F);
    }

    Vector2 logicalMouse() const {
        const float scale = std::min(
            static_cast<float>(GetScreenWidth()) / ScreenWidth,
            static_cast<float>(GetScreenHeight()) / ScreenHeight);
        const float offsetX = (static_cast<float>(GetScreenWidth()) - ScreenWidth * scale) / 2.0F;
        const float offsetY = (static_cast<float>(GetScreenHeight()) - ScreenHeight * scale) / 2.0F;
        return Vector2{
            (static_cast<float>(GetMouseX()) - offsetX) / scale,
            (static_cast<float>(GetMouseY()) - offsetY) / scale
        };
    }

    int inventorySlotAt(Vector2 mouse) const {
        constexpr float startX = 272.0F;
        constexpr float startY = 150.0F;
        if (mouse.x < startX || mouse.x >= startX + 8.0F * 52.0F
            || mouse.y < startY || mouse.y >= startY + 4.0F * 56.0F) {
            return -1;
        }
        const int column = static_cast<int>((mouse.x - startX) / 52.0F);
        const int row = static_cast<int>((mouse.y - startY) / 56.0F);
        return row * 8 + column;
    }

    void draw() {
        BeginTextureMode(target_);
        ClearBackground(BLACK);
        switch (screen_) {
        case Screen::MainMenu:
            drawMainMenu();
            break;
        case Screen::PlayMenu:
            drawPlayMenu();
            break;
        case Screen::Settings:
            drawSettings();
            break;
        case Screen::Credits:
            drawCredits();
            break;
        case Screen::Game:
            drawGame();
            break;
        case Screen::Pause:
            drawPause();
            break;
        case Screen::DeleteSave:
            drawDeleteSave();
            break;
        case Screen::VolumeSettings:
            drawVolumeSettings();
            break;
        case Screen::LanguageSettings:
            drawLanguageSettings();
            break;
        case Screen::QualitySettings:
            drawQualitySettings();
            break;
        case Screen::AutosaveSettings:
            drawAutosaveSettings();
            break;
        case Screen::Tutorial:
            drawTutorial();
            break;
        case Screen::SaveFolder:
            drawSaveFolder();
            break;
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        const Rectangle source{0.0F, 0.0F, static_cast<float>(target_.texture.width),
                               -static_cast<float>(target_.texture.height)};
        const float scale = std::min(
            static_cast<float>(GetScreenWidth()) / ScreenWidth,
            static_cast<float>(GetScreenHeight()) / ScreenHeight);
        const float width = ScreenWidth * scale;
        const float height = ScreenHeight * scale;
        const Rectangle destination{(GetScreenWidth() - width) / 2.0F,
                                    (GetScreenHeight() - height) / 2.0F, width, height};
        DrawTexturePro(target_.texture, source, destination, Vector2{0.0F, 0.0F}, 0.0F, WHITE);
        const Texture2D cursor = textures_.get("cursor");
        constexpr float cursorWidth = 16.0F;
        const float cursorHeight = cursorWidth * static_cast<float>(cursor.height) / cursor.width;
        drawTexture(cursor, static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()),
                    cursorWidth, cursorHeight);
        EndDrawing();
    }

    void drawBackground(const std::string& key) {
        drawTexture(textures_.get(key), 0.0F, 0.0F, static_cast<float>(ScreenWidth), static_cast<float>(ScreenHeight));
    }

    static void drawWhiteGlow(Rectangle bounds) {
        DrawRectangleRounded({bounds.x - 4.0F, bounds.y - 4.0F, bounds.width + 8.0F, bounds.height + 8.0F},
                             0.15F, 6, Color{255, 255, 255, 8});
        DrawRectangleRounded({bounds.x - 2.0F, bounds.y - 2.0F, bounds.width + 4.0F, bounds.height + 4.0F},
                             0.12F, 6, Color{255, 255, 255, 12});
        DrawRectangleRounded(bounds, 0.1F, 6, Color{255, 255, 255, 20});
    }

    static void drawImageButton(const Button& button, Vector2 mouse, bool highlighted = false) {
        if (!highlighted && !CheckCollisionPointRec(mouse, button.bounds)) {
            return;
        }
        drawWhiteGlow(button.bounds);
    }

    void drawMainMenu() {
        drawBackground("bgmenu");
        const std::array<Button, 4> buttons = {{
            {{15.0F, 153.0F, 290.0F, 70.0F}, "Play"},
            {{15.0F, 247.0F, 290.0F, 78.0F}, "Settings"},
            {{15.0F, 341.0F, 290.0F, 73.0F}, "Credits"},
            {{15.0F, 435.0F, 290.0F, 74.0F}, "Quit"}
        }};
        const Vector2 mouse = logicalMouse();
        for (const auto& button : buttons) {
            drawImageButton(button, mouse);
        }
        if (clicked(buttons[0], mouse) || IsKeyPressed(KEY_ENTER)) {
            screen_ = Screen::PlayMenu;
        } else if (clicked(buttons[1], mouse)) {
            settingsFromPause_ = false;
            screen_ = Screen::Settings;
        } else if (clicked(buttons[2], mouse)) {
            screen_ = Screen::Credits;
        } else if (clicked(buttons[3], mouse)) {
            shouldExit_ = true;
        }
    }

    void drawPlayMenu() {
        drawBackground("bgplay");
        const std::array<Button, 3> buttons = {{
            {{34.0F, 262.0F, 290.0F, 71.0F}, "Continue"},
            {{34.0F, 344.0F, 290.0F, 70.0F}, "New world"},
            {{34.0F, 423.0F, 290.0F, 71.0F}, "Back"}
        }};
        const Vector2 mouse = logicalMouse();
        for (const auto& button : buttons) {
            drawImageButton(button, mouse);
        }
        if (clicked(buttons[0], mouse)) {
            beginGame(false);
        } else if (clicked(buttons[1], mouse)) {
            beginGame(true);
        } else if (clicked(buttons[2], mouse)) {
            screen_ = Screen::MainMenu;
        }
    }

    void drawSettings() {
        drawMenuSettings(false);
    }

    void drawMenuSettings(bool pauseMenu) {
        settingsFromPause_ = pauseMenu;
        drawBackground(pauseMenu ? "bgpausemenu" : "bgsettings");
        const std::array<Button, 8> buttons = {{
            {{24.0F, 137.0F, 290.0F, 73.0F}, "Volume"},
            {{24.0F, 213.0F, 290.0F, 72.0F}, "Language"},
            {{24.0F, 289.0F, 290.0F, 70.0F}, "Quality"},
            {{24.0F, 366.0F, 290.0F, 72.0F}, "Back"},
            {{507.0F, 137.0F, 290.0F, 73.0F}, "Delete Save"},
            {{507.0F, 213.0F, 290.0F, 72.0F}, "Auto Save"},
            {{507.0F, 289.0F, 290.0F, 70.0F}, pauseMenu ? "End Player" : "Tutorial"},
            {{507.0F, 366.0F, 290.0F, 72.0F}, pauseMenu ? "Menu" : "Save Folder"}
        }};
        const Vector2 mouse = logicalMouse();
        for (const auto& button : buttons) {
            drawImageButton(button, mouse);
        }
        if (clicked(buttons[0], mouse)) {
            screen_ = Screen::VolumeSettings;
        } else if (clicked(buttons[1], mouse)) {
            screen_ = Screen::LanguageSettings;
        } else if (clicked(buttons[2], mouse)) {
            screen_ = Screen::QualitySettings;
        } else if (clicked(buttons[3], mouse)) {
            screen_ = pauseMenu ? Screen::Game : Screen::MainMenu;
        } else if (clicked(buttons[4], mouse)) {
            screenBeforeDelete_ = pauseMenu ? Screen::Pause : Screen::Settings;
            screen_ = Screen::DeleteSave;
        } else if (clicked(buttons[5], mouse)) {
            screen_ = Screen::AutosaveSettings;
        } else if (clicked(buttons[6], mouse)) {
            if (pauseMenu) {
                coopEnabled_ = !coopEnabled_;
                if (coopEnabled_ && player1_.x == player0_.x && player1_.y == player0_.y) {
                    player1_.x = player0_.x + 24.0F;
                    ensurePlayerAboveGround(player1_, world_);
                }
            } else {
                screen_ = Screen::Tutorial;
            }
        } else if (clicked(buttons[7], mouse)) {
            if (pauseMenu) {
                saveWorld();
                screen_ = Screen::PlayMenu;
            } else {
                screen_ = Screen::SaveFolder;
            }
        }
    }

    void drawVolumeSettings() {
        drawBackground("volumepopup");
        const std::array<float, 3> values = {masterVolume_, musicVolume_, soundVolume_};
        const std::array<float, 3> sliderY = {199.0F, 278.0F, 367.0F};
        for (int i = 0; i < 3; ++i) {
            const float y = sliderY[static_cast<std::size_t>(i)];
            DrawRectangle(380, static_cast<int>(y), static_cast<int>(410.0F * values[static_cast<std::size_t>(i)]),
                          14, Color{151, 121, 183, 185});
        }
        const Button back{{109.0F, 394.0F, 173.0F, 43.0F}, "Back"};
        const Vector2 mouse = logicalMouse();
        drawImageButton(back, mouse);
        if (clicked(back, mouse) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = settingsFromPause_ ? Screen::Pause : Screen::Settings;
        }
    }

    void drawLanguageSettings() {
        drawBackground("languagepopup");
        const std::array<Button, 2> choices = {{
            {{134.0F, 186.0F, 568.0F, 69.0F}, "English"},
            {{134.0F, 263.0F, 568.0F, 69.0F}, "Polski"}
        }};
        const Vector2 mouse = logicalMouse();
        for (std::size_t i = 0; i < choices.size(); ++i) {
            drawImageButton(choices[i], mouse, static_cast<int>(i) == language_);
            const float width = MeasureTextEx(textures_.font(), choices[i].label, 27.0F, 1.0F).x;
            drawText(textures_.font(), choices[i].label,
                     choices[i].bounds.x + (choices[i].bounds.width - width) / 2.0F,
                     choices[i].bounds.y + 20.0F, 27.0F);
        }
        const Button back{{109.0F, 394.0F, 173.0F, 43.0F}, "Back"};
        drawImageButton(back, mouse);
        if (clicked(choices[0], mouse)) {
            language_ = 0;
        } else if (clicked(choices[1], mouse)) {
            language_ = 1;
        } else if (clicked(back, mouse) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = settingsFromPause_ ? Screen::Pause : Screen::Settings;
        }
    }

    void drawQualitySettings() {
        drawBackground("qualitypopup");
        const std::array<Button, 2> choices = {{
            {{134.0F, 186.0F, 568.0F, 69.0F}, "Low"},
            {{134.0F, 263.0F, 568.0F, 69.0F}, "High"}
        }};
        const Vector2 mouse = logicalMouse();
        for (std::size_t i = 0; i < choices.size(); ++i) {
            drawImageButton(choices[i], mouse, static_cast<int>(i) == quality_);
            const float width = MeasureTextEx(textures_.font(), choices[i].label, 27.0F, 1.0F).x;
            drawText(textures_.font(), choices[i].label,
                     choices[i].bounds.x + (choices[i].bounds.width - width) / 2.0F,
                     choices[i].bounds.y + 20.0F, 27.0F);
        }
        const Button back{{109.0F, 394.0F, 173.0F, 43.0F}, "Back"};
        drawImageButton(back, mouse);
        if (clicked(choices[0], mouse)) {
            quality_ = 0;
            textures_.setFiltering(TEXTURE_FILTER_POINT);
        } else if (clicked(choices[1], mouse)) {
            quality_ = 1;
            textures_.setFiltering(TEXTURE_FILTER_BILINEAR);
        } else if (clicked(back, mouse) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = settingsFromPause_ ? Screen::Pause : Screen::Settings;
        }
    }

    void drawAutosaveSettings() {
        drawBackground("autosavepopup");
        const std::array<Button, 3> buttons = {{
            {{134.0F, 186.0F, 568.0F, 69.0F}, "Auto-save interval"},
            {{134.0F, 263.0F, 568.0F, 69.0F}, "Save"},
            {{103.0F, 388.0F, 196.0F, 49.0F}, "Back"}
        }};
        const Vector2 mouse = logicalMouse();
        for (const auto& button : buttons) {
            drawImageButton(button, mouse);
        }
        const std::string status = autoSaveMinutes_ == 0
            ? "OFF"
            : "ON - every " + std::to_string(autoSaveMinutes_) + " min";
        drawText(textures_.font(), status, 560.0F, 347.0F, 18.0F, RAYWHITE);
        if (clicked(buttons[0], mouse)) {
            constexpr std::array<int, 4> intervals = {0, 1, 5, 10};
            const auto current = std::find(intervals.begin(), intervals.end(), autoSaveMinutes_);
            autoSaveMinutes_ = intervals[(static_cast<std::size_t>(std::distance(intervals.begin(), current)) + 1)
                                         % intervals.size()];
        } else if (clicked(buttons[1], mouse)) {
            if (worldLoaded_) {
                saveWorld();
            }
        } else if (clicked(buttons[2], mouse) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = settingsFromPause_ ? Screen::Pause : Screen::Settings;
        }
    }

    void drawTutorial() {
        drawBackground("bgcredits");
        DrawRectangleRounded({60.0F, 60.0F, 840.0F, 410.0F}, 0.04F, 6, Color{28, 24, 55, 220});
        drawText(textures_.font(), "HOW TO PLAY", 90.0F, 82.0F, 32.0F);
        const std::array<std::string, 6> instructions = {
            "Move: A / D     Jump: SPACE or W     Long jump: E",
            "Crouch: S     Sprint: LEFT SHIFT     Dash: R",
            "Player 2: arrow keys     UP: jump     /: long jump",
            "Tab: inventory     1-8: hotbar     Q: drop item",
            "F2: toggle player 2     F5: save",
            "ESC: pause     F11: fullscreen"
        };
        float y = 151.0F;
        for (const auto& instruction : instructions) {
            constexpr float maxWidth = 760.0F;
            const float measuredWidth = MeasureTextEx(textures_.font(), instruction.c_str(), 18.0F, 1.0F).x;
            const float fontSize = measuredWidth > maxWidth ? 18.0F * maxWidth / measuredWidth : 18.0F;
            drawText(textures_.font(), instruction, 91.0F, y, fontSize);
            y += 42.0F;
        }
        const Button back{{60.0F, 474.0F, 173.0F, 43.0F}, "Back"};
        drawButton(textures_.font(), back, logicalMouse(), true);
        if (clicked(back, logicalMouse()) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = Screen::Settings;
        }
    }

    void drawSaveFolder() {
        drawBackground("bgcredits");
        DrawRectangleRounded({60.0F, 60.0F, 840.0F, 410.0F}, 0.04F, 6, Color{28, 24, 55, 220});
        drawText(textures_.font(), "SAVE FOLDER", 90.0F, 95.0F, 32.0F);
        drawText(textures_.font(), saveDirectory().string(), 90.0F, 173.0F, 17.0F);
        drawText(textures_.font(),
                 language_ == 0 ? "Click Copy path to copy this location." : "Kliknij Copy path, aby skopiowac lokalizacje.",
                 90.0F, 223.0F, 18.0F);
        const Button copy{{90.0F, 295.0F, 270.0F, 52.0F}, "Copy path"};
        const Button back{{109.0F, 394.0F, 173.0F, 43.0F}, "Back"};
        const Vector2 mouse = logicalMouse();
        drawButton(textures_.font(), copy, mouse, true);
        drawButton(textures_.font(), back, mouse, true);
        if (saveFolderCopied_) {
            drawText(textures_.font(), language_ == 0 ? "Path copied." : "Skopiowano sciezke.", 90.0F, 260.0F, 18.0F);
        }
        if (clicked(copy, mouse)) {
            const std::string path = std::filesystem::absolute(saveDirectory()).string();
            SetClipboardText(path.c_str());
            saveFolderCopied_ = true;
        } else if (clicked(back, mouse) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = Screen::Settings;
        }
    }

    void drawCredits() {
        drawBackground("bgcredits");
        std::ifstream credits(root_ / "PDD" / "credits.txt");
        std::string line;
        float y = 145.0F;
        if (!credits) {
            throw std::runtime_error("Could not open PDD/credits.txt.");
        }
        while (std::getline(credits, line) && y < 445.0F) {
            drawText(textures_.font(), line, 480.0F - MeasureTextEx(textures_.font(), line.c_str(), 20.0F, 1.0F).x / 2.0F,
                     y, 20.0F);
            y += 29.0F;
        }
        const Button back{{24.0F, 465.0F, 180.0F, 48.0F}, "Back"};
        const Vector2 mouse = logicalMouse();
        drawButton(textures_.font(), back, mouse, true);
        if (clicked(back, mouse)) {
            screen_ = Screen::MainMenu;
        }
    }

    void drawGame() {
        drawTexture(textures_.get("background:night"), 0.0F, 0.0F,
                    static_cast<float>(ScreenWidth), static_cast<float>(ScreenHeight));
        const float cameraX = cameraPositionX();
        const float cameraY = cameraPositionY();
        const int firstX = std::max(0, static_cast<int>(cameraX) / TileSize);
        const int lastX = std::min(world_.width(), (static_cast<int>(cameraX) + ScreenWidth) / TileSize + 1);
        const int firstY = std::max(0, static_cast<int>(cameraY) / TileSize);
        const int lastY = std::min(world_.height(), (static_cast<int>(cameraY) + ScreenHeight) / TileSize + 1);
        for (int y = firstY; y < lastY; ++y) {
            for (int x = firstX; x < lastX; ++x) {
                const Tile tile = world_.get(x, y);
                if (tile == Tile::Air) {
                    continue;
                }
                drawTexture(textures_.get(tileTextureKey(tile)), x * TileSize - cameraX,
                            y * TileSize - cameraY, static_cast<float>(TileSize), static_cast<float>(TileSize));
            }
        }
        if (coopEnabled_) {
            drawPlayer(player1_, 1, cameraX, cameraY);
        }
        drawPlayer(player0_, 0, cameraX, cameraY);
        drawHud();
        drawInventory();
        const Button pause{{772.0F, 23.0F, 38.0F, 38.0F}, ""};
        drawImageButton(pause, logicalMouse());
        if (GetTime() - saveMessageTime_ < 2.0) {
            drawText(textures_.font(), "Saved", 850.0F, 74.0F, 16.0F, AccentColor);
        }
    }

    float cameraPositionX() const {
        const float focus = coopEnabled_ ? (player0_.x + player1_.x) / 2.0F : player0_.x;
        return std::clamp(focus - ScreenWidth / 2.0F, 0.0F,
                          static_cast<float>(world_.width() * TileSize - ScreenWidth));
    }

    float cameraPositionY() const {
        const float focus = coopEnabled_ ? (player0_.y + player1_.y) / 2.0F : player0_.y;
        return std::clamp(focus - ScreenHeight / 2.0F, 0.0F,
                          static_cast<float>(world_.height() * TileSize - ScreenHeight));
    }

    void drawPlayer(const Player& player, int index, float cameraX, float cameraY) {
        std::string frame;
        if (!player.onGround) {
            const char* direction = player.lastHorizontalInput == 0.0F
                ? "idle"
                : (player.facing > 0 ? "right" : "left");
            frame = "jump" + std::to_string(std::clamp(player.animationFrame + 1, 1, 3)) + "_" + direction;
        } else if (player.lastHorizontalInput == 0.0F) {
            frame = "idle";
        } else {
            frame = (player.facing > 0 ? "right" : "left")
                + std::to_string(std::clamp(player.animationFrame + 1, 1, 3));
        }
        const auto key = "player" + std::to_string(index) + ":" + frame;
        drawTexture(textures_.get(key), player.x - cameraX, player.y - cameraY, 32.0F, 32.0F);
    }

    void drawHud() {
        drawTexture(textures_.get("hud"), 0.0F, 0.0F, 960.0F, 540.0F);
        constexpr int barX = 29;
        constexpr int barWidth = 202;
        constexpr int barHeight = 11;
        DrawRectangle(barX, 27, barWidth, barHeight, Color{24, 20, 28, 210});
        DrawRectangle(barX, 27,
                      static_cast<int>(barWidth * std::clamp(player0_.health / 100.0F, 0.0F, 1.0F)),
                      barHeight, Color{215, 48, 57, 255});
        DrawRectangle(barX, 61, barWidth, barHeight, Color{24, 20, 28, 210});
        DrawRectangle(barX, 61,
                      static_cast<int>(barWidth * std::clamp(player0_.stamina / 100.0F, 0.0F, 1.0F)),
                      barHeight, Color{55, 145, 235, 255});
        DrawRectangle(829, 25, 120, 120, Color{255, 255, 255, 100});
        if (world_.width() > 0 && world_.height() > 0) {
            for (int px = 0; px < 120; ++px) {
                const int blockX = px * world_.width() / 120;
                const int surface = groundBlock(world_, blockX);
                if (surface >= 0) {
                    const int py = std::clamp(surface * 120 / world_.height(), 0, 119);
                    DrawPixel(829 + px, 25 + py, Color{74, 107, 68, 255});
                }
            }
            const int playerX = std::clamp(static_cast<int>(player0_.x * 120.0F / (world_.width() * TileSize)), 0, 119);
            DrawCircle(829 + playerX, 30, 3.0F, RED);
        }
    }

    static int groundBlock(const GameMap& map, int blockX) {
        for (int y = 0; y < map.height(); ++y) {
            if (isSolid(map.get(blockX, y))) {
                return y;
            }
        }
        return -1;
    }

    void drawInventory() {
        constexpr float hotbarX = 251.0F;
        constexpr float hotbarY = 472.0F;
        for (int slot = 0; slot < 8; ++slot) {
            const Rectangle bounds{hotbarX + slot * 57.0F, hotbarY, 52.0F, 52.0F};
            if (slot == selectedSlot_) {
                DrawRectangleRec(bounds, Color{255, 255, 255, 30});
            }
            const auto& item = inventory_[static_cast<std::size_t>(slot)];
            if (!item.empty()) {
                drawTexture(textures_.get("item:" + item), bounds.x + 10.0F, bounds.y + 9.0F, 32.0F, 32.0F);
            }
        }
        if (!inventoryOpen_) {
            return;
        }
        const Rectangle panel{244.0F, 98.0F, 472.0F, 324.0F};
        DrawRectangleRounded(panel, 0.06F, 8, PanelColor);
        DrawRectangleRoundedLines(panel, 0.06F, 8, RAYWHITE);
        drawText(textures_.font(), "Inventory", 386.0F, 111.0F, 24.0F);
        const float startX = 272.0F;
        const float startY = 150.0F;
        for (int slot = 0; slot < 32; ++slot) {
            const int column = slot % 8;
            const int row = slot / 8;
            const Rectangle bounds{startX + column * 52.0F, startY + row * 56.0F, 46.0F, 46.0F};
            DrawRectangleRec(bounds, Color{40, 48, 47, 230});
            DrawRectangleLinesEx(bounds, 1.0F, Color{124, 145, 133, 255});
            const auto& item = inventory_[static_cast<std::size_t>(slot)];
            if (!item.empty()) {
                drawTexture(textures_.get("item:" + item), bounds.x + 8.0F, bounds.y + 8.0F, 30.0F, 30.0F);
            }
        }
        if (draggingInventory_ && draggedFrom_ >= 0) {
            const auto& item = inventory_[static_cast<std::size_t>(draggedFrom_)];
            if (!item.empty()) {
                const auto mouse = logicalMouse();
                drawTexture(textures_.get("item:" + item), mouse.x - 16.0F, mouse.y - 16.0F, 32.0F, 32.0F);
            }
        }
    }

    void drawPause() {
        drawMenuSettings(true);
    }

    void drawDeleteSave() {
        drawBackground("spopup");
        const Button yes{{151.0F, 306.0F, 290.0F, 79.0F}, "Yes"};
        const Button no{{506.0F, 306.0F, 290.0F, 79.0F}, "No"};
        const Button back{{106.0F, 390.0F, 195.0F, 49.0F}, "Back"};
        const Vector2 mouse = logicalMouse();
        drawImageButton(yes, mouse);
        drawImageButton(no, mouse);
        drawImageButton(back, mouse);
        if (clicked(yes, mouse)) {
            deleteWorld();
        } else if (clicked(no, mouse) || clicked(back, mouse) || IsKeyPressed(KEY_ESCAPE)) {
            screen_ = screenBeforeDelete_;
        }
    }

    std::filesystem::path root_;
    TextureStore textures_;
    MusicPlayer music_;
    RenderTexture2D target_{};
    Sound jumpSound_{};
    Sound dashSound_{};
    Screen screen_ = Screen::MainMenu;
    GameMap world_{MapWidth, MapHeight};
    PlayerPositions positions_{};
    Player player0_{};
    Player player1_{};
    std::array<std::string, 32> inventory_{};
    int selectedSlot_ = 0;
    int nextMusicTrack_ = 0;
    int draggedFrom_ = -1;
    int musicMode_ = 0;
    int quality_ = 0;
    int language_ = 0;
    int autoSaveMinutes_ = 1;
    Screen screenBeforeDelete_ = Screen::Pause;
    bool coopEnabled_ = false;
    bool inventoryOpen_ = false;
    bool draggingInventory_ = false;
    bool settingsFromPause_ = false;
    bool worldLoaded_ = false;
    bool shouldExit_ = false;
    bool saveFolderCopied_ = false;
    float masterVolume_ = 0.75F;
    float musicVolume_ = 0.55F;
    float soundVolume_ = 0.65F;
    double lastSaveTime_ = 0.0;
    double saveMessageTime_ = 0.0;
};

} // namespace

int main() {
    bool windowReady = false;
    bool audioReady = false;
    try {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(ScreenWidth, ScreenHeight, "Forgewild");
        windowReady = true;
        SetWindowMinSize(640, 360);
        HideCursor();
        InitAudioDevice();
        audioReady = true;
        SetTargetFPS(60);
        RenderTexture2D target = LoadRenderTexture(ScreenWidth, ScreenHeight);
        if (target.id == 0) {
            throw std::runtime_error("Could not create the game render target.");
        }
        const auto root = locateAssets();
        {
            Application application(root, target);
            application.run();
        }
        UnloadRenderTexture(target);
        CloseAudioDevice();
        CloseWindow();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Forgewild: " << error.what() << '\n';
        if (audioReady) {
            CloseAudioDevice();
        }
        if (windowReady) {
            CloseWindow();
        }
        return 1;
    }
}
