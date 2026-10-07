# Fork regression checks

From this checkout, run:

```sh
python3 tests/run_fork_regressions.py build
```

Requires a configured Ninja build, Qt6Test, pkg-config, Xvfb, and dbus-run-session.
The runner builds the application and links the regression harness against its
compiled objects, replacing the CLI main function. It uses a separate display,
session bus, configuration directory, and temporary save folders. It does not
change the desktop session's Flameshot settings or clipboard.

The checks exercise the resolution controls and actual overlay rendering, the
three default save locations and all ten folder shortcuts (including hidden
toolbar buttons), both double-click behaviors, and profile import persistence.
A small preload library adjusts only the test process's realtime clock across
local midnight; monotonic timers and the machine's clock are unaffected.
Persistence is also checked with pending settings writes, a fresh process,
invalid and empty imports, and importing the currently active profile.

The ten-location implementation and the double-click fixes predate this
restoration. The regression checks preserve the behavior from commits
`a08d5bfe`, `7c22ab11`, and `7b591489` while the reverted resolution and
configuration-import changes are restored.

The settings checks also cover opening directly on Options, the label/value
order, usable window resizing, loading the historical QSS palettes, and live
stylesheet replacements without changing the active profile.

Run the scheduler checks separately:

```sh
python3 tests/theme_scripts.py
```

These use temporary XDG directories and a simulated month. They verify that
scheduled theme updates never write to an imported INI or restart Flameshot,
while an explicit theme selection can still change the four palette keys.
