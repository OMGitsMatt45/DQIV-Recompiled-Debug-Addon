# Native Cheats page: build and play checks

The overlay adds an in-game page at **Misc > Settings > Cheats**. L/R turns
pages (Q/W on the default keyboard, LB/RB on a gamepad). Left/right changes a
toggle, multiplier, selected item or quantity. A/confirm runs a one-shot action.

## Build

Use the repository's existing Windows build: Visual Studio 2022 with Desktop
development with C++, a European YIVP ROM in `ROMandOBBfileHere`, and
`build.bat`. The optional Android OBB supplies HD sound. The build stays Win32.

The patch targets upstream **1.4**, commit
`793a40ea08a703ae36d41bba263782d541ae2bdb`. Apply it to that release's full
source tree, including the overlay's `CMakeLists.txt`, which compiles
`native/Cheats.cpp` into the executable. Do not apply this version to an older
fork or assume compatibility with a later upstream release.
Use a copied save for the first play check, since the one-shot actions change
the adventure when it is subsequently saved.

## Automated checks

Run `python3 tests/cheats/run_host_tests.py` on Linux or WSL with Python 3 and a
C++20 `g++`. The harness compiles the actual native settings, cheat actions and
Settings panel. It extracts selected game methods unchanged from production
sources and supplies small game-state and renderer boundaries.

The host suite runs directly from this partial add-on repository; no game tree
or ROM is needed. It checks that the packaged CMake executable includes
`Cheats.cpp`. For a production-header check, also run:

```sh
python3 tests/cheats/run_host_tests.py --game-root /path/to/DQIV-Recompiled-1.4
```

This checks all six modified C++ translation units against the real upstream
1.4 headers, without the game/renderer fixtures. The upstream tree is read only
and need not have the overlay installed. It catches renamed game symbols and
missing menu headers that the focused fixtures cannot detect. The Linux syntax
check uses `-fpermissive` for the game's existing 32-bit pointer-to-int casts;
it does not validate the Win32 ABI or execute a Windows linker.

The checks cover configuration defaults and round trips, multiplier limits and
overflow, invalid/full/partly full inventory cases, recovery and party-view
restoration, battle-only HP/MP protection, pending encounters and scripted
battles, reward eligibility and the existing double-reward effect, menu
navigation, action dispatch, item/quantity limits, six language paths, and
keyboard rebinding. Address and undefined-behavior sanitizers stay enabled;
leak detection is disabled because some sandboxes deny its /proc access.

These checks do not validate the MSVC Win32 ABI, assets, actual font metrics,
audio, complete battles, or loading/saving the cartridge image.

## In-game checks

1. Start with a fresh settings file. Load an adventure and open Cheats. Both
   switches show No and both multipliers show x1. The adventure-log Settings
   screen has the original four pages and no actions for an unloaded game.
2. Turn through every page in both directions with keyboard and gamepad.
   Select every cheat row, including Back. Check for overlapping labels,
   clipped item names and stale selection arrows in English and Japanese,
   then in the other four languages. Check DS-screen and 16:9 layouts.
3. Fill gold, bank, coins and medals. Verify 999,999 gold, 99,999,000 bank,
   999,999 casino coins and 99 unspent mini medals in their normal menus.
   Moving left/right over these actions must not execute them.
4. Add one medicinal herb, add enough to reach 99, and try adding another.
   Check the reported added quantity. Try an existing stack after an empty
   bag slot, then a new item in a full bag. There must be no duplicates,
   overwritten items or quantities above 99.
5. With injured, poisoned and fallen members, including carriage members,
   run Heal / revive party. Verify HP/MP, revival and status clearing.
   Return to the normal menus and check party order and carriage access.
6. In an ordinary battle, enable invincibility before entering. Verify party
   HP/MP remain unchanged and monsters still take damage. Disable it, enter
   another battle and verify damage and MP costs resume. Check that field
   damage and ordinary healing retain their original behavior.
7. Enable No random battles in a dungeon and on the world map. Walk through
   encounter terrain and cross map boundaries. Start a scripted/boss battle.
   Disable the switch and confirm random battles resume. Save, then load
   the adventure with the switch off to confirm the save's original encounter
   flag remains intact.
8. Compare the same ordinary encounter at x1, x2 and x64 rewards. Check both
   messages and actual EXP/gold. Fallen characters and battle NPCs retain
   the game's normal reward eligibility. Check a level-up with spell learning.
9. Save the adventure after money/items/recovery changes, quit and reload.
   Verify adventure changes and settings persistence. Reset cheats and
   verify both switches and both multipliers return to defaults while already
   awarded money/items remain.

## Scope

The first page supplies native features. It has no raw Action Replay parser,
character level/stat editor, wall bypass, or arbitrary story-flag editor.
The invincibility switch freezes battle HP/MP; it does not clear status effects
or automatically revive a character already fallen before battle.
