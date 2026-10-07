# Build the nicitaacom fork

The fork enables Imgur upload on Ctrl+D by default and embeds the Crazy
Mechanics theme and checkbox artwork. A fresh installation needs no separate
stylesheet setup. An existing `~/.local/share/flameshot-theme/current.qss`
overrides the bundled theme.

On the Kali desktop used for verification, the requested dependency list
resolves with apt:

```sh
sudo apt update
sudo apt install -y \
  git cmake g++ build-essential \
  qt6-base-dev qt6-tools-dev-tools qt6-tools-dev qt6-svg-dev \
  libkf6guiaddons-dev libqt6dbus6 libqt6network6 libqt6core6 \
  libqt6widgets6 libqt6gui6 libqt6svg6 qt6-qpa-plugins \
  openssl ca-certificates qt6-image-formats-plugins desktop-file-utils
```

For a fresh clone:

```sh
mkdir -p ~/Documents/GitHub
git clone https://github.com/nicitaacom/flameshot.git ~/Documents/GitHub/flameshot
cd ~/Documents/GitHub/flameshot

git remote get-url origin
git rev-parse --short HEAD
cmake --version
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/flameshot-nicitaacom \
  -DUSE_LAUNCHER_ABSOLUTE_PATH=ON
cmake --build build -j"$(nproc)"

sudo cmake --install build
```

When reusing an existing build directory, add `-DENABLE_IMGUR=ON` to the
configure command to override an older cached `ENABLE_IMGUR=OFF` value.
The two entries in `build/CMakeCache.txt` that identify the fork features are
`ENABLE_IMGUR:BOOL=ON` and `CMAKE_HOME_DIRECTORY` pointing at this checkout.

Create a launcher that names the exact installed executable:

```sh
mkdir -p ~/.local/share/applications
cat > ~/.local/share/applications/flameshot-nicitaacom.desktop <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Flameshot (nicitaacom)
Comment=Screenshot tool built from nicitaacom/flameshot
Exec=/opt/flameshot-nicitaacom/bin/flameshot
Icon=/opt/flameshot-nicitaacom/share/icons/hicolor/128x128/apps/org.flameshot.Flameshot.png
Categories=Graphics;
StartupWMClass=flameshot
Terminal=false
DESKTOP
update-desktop-database ~/.local/share/applications
```

Replace an already running daemon before launching the installed build. The
single-instance guard otherwise lets the older process keep hosting captures:

```sh
pkill -x flameshot || true
/opt/flameshot-nicitaacom/bin/flameshot --version
/opt/flameshot-nicitaacom/bin/flameshot
```

Confirm the running binary:

```sh
for pid in $(pgrep -x flameshot); do
  readlink /proc/"$pid"/exe
done
```

The startup stylesheet loader was lost in the earlier theme merge. The old
build also cached Imgur upload off. The restored loader, bundled theme, and
fork default address both causes. Scheduled theme updates now change only
the stylesheet and preserve imported profiles.
