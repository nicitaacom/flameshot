#!/usr/bin/env python3
"""Run native fork regressions against an existing CMake build (Qt6Test required).

Usage: python3 tests/run_fork_regressions.py [build directory]
Reuses the checkout's compiled objects, replacing only the CLI main function.
All settings and output files live in a disposable temporary directory.
"""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "build"
subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True)
cache = (build / "CMakeCache.txt").read_text()
if "CMAKE_GENERATOR:INTERNAL=Ninja" in cache:
    commands = subprocess.check_output(
        ["ninja", "-C", str(build), "-t", "commands", "src/flameshot"], text=True
    ).splitlines()
    compile_args = shlex.split(next(line for line in commands if " -c " + str(root / "src/main.cpp") in line))
    link_args = shlex.split(next(line for line in commands if " -o src/flameshot " in line).split(" && ")[1])
    command_directory = build
else:
    compiler = next(line.split("=", 1)[1] for line in cache.splitlines()
                    if line.startswith("CMAKE_CXX_COMPILER:FILEPATH="))
    flags = (build / "src/CMakeFiles/flameshot.dir/flags.make").read_text().splitlines()
    compile_args = [compiler]
    for name in ("CXX_DEFINES", "CXX_INCLUDES", "CXX_FLAGS"):
        compile_args += shlex.split(next(line.split("=", 1)[1] for line in flags if line.startswith(name + " =")))
    compile_args += ["-o", "placeholder.o", "-c", "placeholder.cpp"]
    link_args = shlex.split((build / "src/CMakeFiles/flameshot.dir/link.txt").read_text())
    command_directory = build / "src"
qt = shlex.split(subprocess.check_output(["pkg-config", "--cflags", "--libs", "Qt6Test"], text=True))
with tempfile.TemporaryDirectory(prefix="flameshot-regressions-") as directory:
    work = Path(directory)
    obj, binary = work / "regressions.o", work / "regressions"
    for flag in ("-o", "-c"):
        compile_args[compile_args.index(flag) + 1] = str(obj if flag == "-o" else root / "tests/fork_regressions.cpp")
    for flag in ("-MT", "-MF"):
        if flag in compile_args:
            i = compile_args.index(flag)
            del compile_args[i:i+2]
    compile_args = [arg for arg in compile_args if arg != "-MD"]
    subprocess.run(compile_args + ["-O0", '-DFORK_SOURCE_DIR="' + str(root) + '"'] + qt, cwd=command_directory, check=True)
    link_args[link_args.index("-o") + 1] = str(binary)
    link_args = [str(obj) if arg.endswith("CMakeFiles/flameshot.dir/main.cpp.o") else arg for arg in link_args]
    subprocess.run(link_args + qt, cwd=command_directory, check=True)
    clock = work / "clock.so"
    subprocess.run(["cc", "-shared", "-fPIC", str(root / "tests/fork_test_clock.c"),
                    "-ldl", "-o", str(clock)], check=True)
    env = dict(os.environ, XDG_CONFIG_HOME=str(work / "config"), XDG_DATA_HOME=str(work / "data"), QT_QPA_PLATFORM="xcb")
    subprocess.run(["dbus-run-session", "--", "xvfb-run", "-a", "env", "LD_PRELOAD=" + str(clock), str(binary)], env=env, check=True)
