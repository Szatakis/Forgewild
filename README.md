# Forgewild (C++)

Forgewild is a cross-platform C++17 port of the original Python/Pygame game. It uses
[raylib 5.5](https://www.raylib.com/) for its window, graphics, input, and audio.
The game code and game-logic tests are in the repository root; textures, sound,
font, and save data stay in their existing folders.

## Requirements

- CMake 3.20 or newer and Git.
- A C++17 compiler (Visual Studio 2022 on Windows, Xcode command-line tools on macOS,
  or GCC/Clang on Linux).
- On Linux, raylib's window dependencies: X11, OpenGL, Wayland, and the development
  packages listed in `.github/workflows/cpp.yml` (including ALSA headers).
- Internet access on the first configure so CMake can download raylib.

## Build and run

From the repository root:

```sh
# Linux and macOS
sh ./build.sh
./build/Forgewild
```

On Windows, use PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
.\build\Forgewild.exe
```

You can also use CMake directly on any supported platform:

```sh
cmake -S . -B build
cmake --build build --config Release --parallel
```

The build copies `Textures`, `Audio`, `PDD`, and `font.TTF` next to the executable.
Save files are written to that copied `PDD` directory.

## Tests

```sh
# Linux and macOS
sh ./test.sh

# Windows PowerShell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test.ps1
```

The CTest suite checks deterministic world generation, terrain and collision
behavior, player movement, and save-file round trips. It does not need to open a
game window.

## Controls

- Player 1: A/D to move, Space or W to jump, E for a long jump, S to crouch,
  Shift to sprint, R to dash.
- Player 2: arrow keys to move, Up to jump, `/` for a long jump, Down to crouch,
  `\` to dash. Add or remove player 2 in the pause menu or press F2.
- Tab opens the inventory; 1-8 selects a hotbar slot; Q drops the selected item.
- Escape pauses, F5 saves, and F11 toggles fullscreen.

Online hosting and networking were not implemented in the original game, so this
port preserves local cooperative play only.
