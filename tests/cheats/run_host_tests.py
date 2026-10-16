#!/usr/bin/env python3
"""Run cheat code with small game/renderer fixtures, without ROM resources.

The game methods under test are extracted unchanged from production sources.
This exercises logic and menu dispatch, not the Windows ABI or actual graphics.
Requires Python 3 and a C++20 g++ (Linux or WSL).
"""
from pathlib import Path
import os
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


def main():
    wrappers = [
        "main/menu/MenuBase.hpp",
        "ov016/MenuTemplate/MenuTemplate_materiel.hpp",
        "ov016/UnkMaterielMenuDraw/UnkMaterielMenuDraw_0216ce4c.hpp",
        "main/menu/UnkMenuPartsDraw.hpp",
        "main/status/GameStatus.hpp", "main/status/OptionStatus.hpp",
        "main/status/PartyStatus.hpp", "main/status/PlayerStatus.hpp",
        "main/param/Param.hpp", "main/text/TextAPI.hpp", "main/dss/Pad.hpp",
        "nitro/pad.h",
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
