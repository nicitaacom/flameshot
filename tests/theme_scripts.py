#!/usr/bin/env python3
"""Verify seasonal themes cannot overwrite profiles or restart Flameshot."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = root / "contrib/flameshot-theme"
with tempfile.TemporaryDirectory(prefix="flameshot-seasonal-test-") as directory:
    work = Path(directory)
    data = work / "data"
    config = work / "config"
    theme = data / "flameshot-theme"
    shutil.copytree(source, theme)
    ini = config / "flameshot/flameshot.ini"
    ini.parent.mkdir(parents=True)
    imported = """[General]
uiColor=#123456
contrastUiColor=#654321
drawColor=#abcdef
userColors=#123456, #abcdef
drawThickness=4
savePathLocationCount=10
savePathLocation10=/imported/location10
showSelectionGeometryEnabled=false
[Shortcuts]
TYPE_SAVE_LOCATION_10=Ctrl+Alt+F10
"""
    ini.write_text(imported)
    commands = work / "bin"
    commands.mkdir()
    date = commands / "date"
    def month(value):
        date.write_text("#!/bin/sh\necho " + str(value) + "\n")
        date.chmod(0o755)
    for name in ("pkill", "setsid", "flameshot"):
        command = commands / name
        command.write_text("#!/bin/sh\necho forbidden process restart >&2\nexit 99\n")
        command.chmod(0o755)
    env = dict(os.environ, XDG_DATA_HOME=str(data), XDG_CONFIG_HOME=str(config),
               PATH=str(commands) + os.pathsep + os.environ["PATH"])
    seasonal = source / "flameshot-seasonal-theme"
    month(10)
    subprocess.run(["bash", str(seasonal)], env=env, check=True)
    assert ini.read_text() == imported, "Daily update overwrote the imported profile"
    assert "#c05cff" in (theme / "current.qss").read_text()
    stamp = (theme / "current.qss").stat().st_mtime_ns
    subprocess.run(["bash", str(seasonal)], env=env, check=True)
    assert (theme / "current.qss").stat().st_mtime_ns == stamp
    month(11)
    subprocess.run(["bash", str(seasonal)], env=env, check=True)
    assert ini.read_text() == imported, "Season change overwrote the imported profile"
    assert (theme / ".current").read_text().strip() == "halloween"
    subprocess.run(["bash", str(source / "flameshot-theme"), "green"], env=env, check=True)
    assert "uiColor=#60ff00" in ini.read_text(), "Explicit theme selection did not apply colors"
    assert "savePathLocation10=/imported/location10" in ini.read_text()
    assert "TYPE_SAVE_LOCATION_10=Ctrl+Alt+F10" in ini.read_text()
    assert "showSelectionGeometryEnabled=false" in ini.read_text()
    print("PASS: daily and seasonal theme updates preserve imported profiles; explicit theme selection works")
