# DQIV Cheats Overlay — upstream 1.4

Adds a **Cheats** page to **Misc > Settings** while an adventure is loaded.
It provides battle invincibility, no random encounters, EXP/gold multipliers,
money and medal actions, party recovery, and adding items by name and quantity.
Cheat switches default to off and multipliers to x1.

## Supported game version

Use [GoldieLeGenie/DQIV-Recompiled release 1.4](https://github.com/GoldieLeGenie/DQIV-Recompiled/tree/1.4),
commit `793a40ea08a703ae36d41bba263782d541ae2bdb`.
This overlay uses the 1.4 game symbols and menu headers. Earlier releases,
including the original personal fork at `e1826f9`, need the previous overlay.
Future upstream releases need a compatibility check before applying this one.

## Install and build on Windows

1. Download and extract the [upstream 1.4 source ZIP](https://github.com/GoldieLeGenie/DQIV-Recompiled/archive/refs/tags/1.4.zip)
   into a fresh folder. It must contain `build.bat`, `build.ps1`, `CMakeLists.txt`,
   `game`, and `native`.
2. Extract the cheats overlay. If it has a `Copy_into_repository` folder, open
   that folder; if you downloaded this add-on repository, open its root instead.
3. Copy `CMakeLists.txt` and the `game`, `native`, `tests`, and `docs` folders
   into the game folder. Merge folders and replace the supplied files.
   Do not copy an enclosing overlay folder or a clone's `.git` directory.
4. Confirm `native/Cheats.cpp` and `native/Cheats.hpp` exist **and** the
   `add_executable(DragonQuestIV ...)` entry in `CMakeLists.txt` includes
   `native/Cheats.cpp`. The build file is part of the overlay; copying just
   `game` and `native` leaves unresolved cheat symbols at link time.
5. Install Visual Studio 2022 with **Desktop development with C++**, the Windows
   SDK, and C++ CMake tools. The game uses the existing **Win32** build.
6. Put your European **YIVP** Nintendo DS ROM (`.nds`, or a ZIP containing it)
   in `ROMandOBBfileHere`. The Android OBB is optional for HD audio.
7. Double-click `build.bat`. Wait for `Created dist\DragonQuestIV.exe`, then
   start that executable. No ROM, assets, or compiled game is included here.

For an existing installation, keep its `dist\saves` folder backed up. Use a
fresh 1.4 source folder when changing game versions rather than mixing old and
new source files. Copying this overlay alone does not upgrade an older game.

## Use

Load an adventure and open **Misc > Settings**. With default controls, **Q**
once from the Game page opens Cheats. Q/W or LB/RB turns pages; Z/Space or
gamepad A confirms. Left/right changes an option, selected item, or quantity.
Money, items, and recovery persist when the adventure is saved normally;
resetting cheats does not undo them. Use a copied save for initial play checks.

## Checks for development

With Python 3 and a C++20 `g++` on Linux or WSL, run the ROM-free host suite:

```sh
python3 tests/cheats/run_host_tests.py
```

To also check all six modified C++ sources against the **actual 1.4 headers**,
pass a full upstream 1.4 source tree (it can be an unmodified extracted ZIP):

```sh
python3 tests/cheats/run_host_tests.py --game-root /path/to/DQIV-Recompiled-1.4
```

The runner also checks that the packaged CMake file registers `Cheats.cpp`.
Host tests keep AddressSanitizer and UndefinedBehaviorSanitizer enabled. These
checks do not replace an MSVC Win32 build or play testing; see
[docs/cheats-testing.md](docs/cheats-testing.md).
