#!/usr/bin/env python3
"""Run cheat code with small game/renderer fixtures, without ROM resources.

The game methods under test are extracted unchanged from production sources.
This exercises logic and menu dispatch, not the Windows ABI or actual graphics.
Requires Python 3 and a C++20 g++ (Linux or WSL). Pass --game-root pointing
at an unmodified upstream 1.4 checkout to also check the production headers.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def function(path, marker):
    text = (ROOT / path).read_text()
    start = text.index(marker)
    brace = text.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def check_build_sources():
    cmake = (ROOT / "CMakeLists.txt").read_text()
    executable = re.search(r"add_executable\s*\(\s*DragonQuestIV\b([^)]*)\)", cmake)
    if not executable or "native/Cheats.cpp" not in executable.group(1).split():
        raise RuntimeError("CMakeLists.txt must compile native/Cheats.cpp into DragonQuestIV")
    print("Cheats.cpp is registered in the DragonQuestIV executable.", flush=True)


def check_production_sources(game_root):
    """Use actual game declarations, without the host renderer/game fixtures."""
    includes = [
        ROOT / "native", ROOT / "game/include", game_root / "native",
        game_root / "native/compat", game_root / "game/include",
        game_root / "game/libs/NitroSDK/include",
        game_root / "game/libs/NitroSystem/include",
    ]
    required = [
        "native/SaveStore.hpp", "game/include/main/global/GlobalDQ4.hpp",
        "game/include/main/menu/MenuPartsDraw.hpp",
        "game/include/ov016/UnkMaterielMenuDraw/MaterielMenuDraw.hpp",
    ]
    for path in required:
        if not (game_root / path).is_file():
            raise RuntimeError(f"Missing upstream 1.4 file: {game_root / path}")
    sources = [
        "native/Cheats.cpp", "native/Settings.cpp",
        "game/src/main/status/HaveStatusInfo.cpp",
        "game/src/main/status/BasePartyStatus.cpp",
        "game/src/main/encount/Encount.cpp",
        "game/src/ov016/MenuSettings/SettingsPanel.cpp",
    ]
    command = [os.environ.get("CXX", "g++"), "-std=c++20", "-DDQIV_NATIVE=1",
               "-fpermissive", "-fsyntax-only"]
    command += [f"-I{path}" for path in includes]
    command += [str(ROOT / path) for path in sources]
    subprocess.run(command, check=True, timeout=60)
    print("6 production sources pass with upstream game headers (no host fixtures).", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-root", type=Path,
                        help="Full upstream 1.4 source tree for production-header checks")
    args = parser.parse_args()
    check_build_sources()
    if args.game_root:
        check_production_sources(args.game_root.resolve())
    wrappers = [
        "main/menu/MenuBase.hpp",
        "ov016/MenuTemplate/MenuTemplate_materiel.hpp",
        "ov016/UnkMaterielMenuDraw/MaterielMenuDraw.hpp",
        "main/menu/MenuPartsDraw.hpp",
        "main/status/GameStatus.hpp", "main/status/OptionStatus.hpp",
        "main/status/PartyStatus.hpp", "main/status/PlayerStatus.hpp",
        "main/param/Param.hpp", "main/text/TextAPI.hpp", "main/dss/Pad.hpp",
        "nitro/pad.h", "SaveStore.hpp",
    ]
    methods = [
        ("game/src/main/status/HaveStatusInfo.cpp", "bool nativeBattleInvincible("),
    ]
    for method in ["setHp", "addBaseHp", "setMp", "addMp", "recovery", "revival"]:
        methods.append(("game/src/main/status/HaveStatusInfo.cpp",
                        f"THUMB void status::HaveStatusInfo::{method}("))
    for method in ["reflectBattleExp", "reflectBattleGold"]:
        methods.append(("game/src/main/status/BasePartyStatus.cpp",
                        f"THUMB void status::PartyStatus::{method}("))
    for method in ["isEncounted", "isEncountedNext"]:
        methods.append(("game/src/main/encount/Encount.cpp",
                        f"THUMB bool encount::Encount::{method}("))
    for method in ["forceBrew", "forceEventBrew"]:
        methods.append(("game/src/main/encount/Encount.cpp",
                        f"THUMB void encount::Encount::{method}("))
    with tempfile.TemporaryDirectory(prefix="dqiv-cheats-tests-") as temporary:
        temp = Path(temporary)
        stubs = temp / "include"
        for path in wrappers:
            wrapper = stubs / path
            wrapper.parent.mkdir(parents=True, exist_ok=True)
            wrapper.write_text('#include "HostGame.hpp"\n')
        hooks = temp / "GameHooks.cpp"
        hooks.write_text('#include "HostGame.hpp"\n#include "Settings.hpp"\n' +
                         "\n\n".join(function(path, marker) for path, marker in methods))
        binary = temp / "cheats_tests"
        command = [
            os.environ.get("CXX", "g++"), "-std=c++20", "-DDQIV_NATIVE=1",
            "-fpermissive", "-g", "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer", f"-I{stubs}", f"-I{HERE}",
            f"-I{ROOT / 'native'}", f"-I{ROOT / 'game/include'}",
            str(HERE / "host_tests.cpp"), str(ROOT / "native/Cheats.cpp"),
            str(ROOT / "native/Settings.cpp"),
            str(ROOT / "game/src/ov016/MenuSettings/SettingsPanel.cpp"),
            str(hooks), "-o", str(binary),
        ]
        subprocess.run(command, check=True, timeout=60)
        environment = os.environ.copy()
        # LeakSanitizer needs /proc/ptrace access unavailable in some sandboxes.
        # Address and undefined-behavior checks remain enabled and fail the run.
        environment["ASAN_OPTIONS"] = environment.get("ASAN_OPTIONS", "") + ":detect_leaks=0:halt_on_error=1"
        environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        subprocess.run([str(binary), str(temp / "saves")], check=True, timeout=30, env=environment)


if __name__ == "__main__":
    main()
