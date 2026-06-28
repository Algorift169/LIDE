#!/bin/bash
set -e

# build_iso.sh - Build a lightweight BlackLine Linux live ISO

WORKSPACE="/home/israfil/Desktop/LIDE"
ISO_DIR="$WORKSPACE/iso"
BUILD_DIR="$ISO_DIR/build"
RFS_DIR="$ISO_DIR/rfs"
INITRD_ROOT="$ISO_DIR/initrd_root"

# Detect kernel version
KERNEL_VERSION="6.19.14+kali-amd64"
VMLINUZ="/boot/vmlinuz-$KERNEL_VERSION"

echo "=== Step 1: Compiling BlackLine components ==="
cd "$WORKSPACE"
make
make blackline-session

echo "=== Step 2: Preparing clean build directories ==="
rm -rf "$ISO_DIR/build" "$ISO_DIR/rfs" "$ISO_DIR/initrd_root"
mkdir -p "$BUILD_DIR/boot/grub"
mkdir -p "$BUILD_DIR/live"

# Setup merged-/usr structure in RootFS
mkdir -p "$RFS_DIR/usr/bin" "$RFS_DIR/usr/sbin" "$RFS_DIR/usr/lib" "$RFS_DIR/usr/lib64"
ln -s usr/bin "$RFS_DIR/bin"
ln -s usr/sbin "$RFS_DIR/sbin"
ln -s usr/lib "$RFS_DIR/lib"
ln -s usr/lib64 "$RFS_DIR/lib64"

mkdir -p "$INITRD_ROOT/bin"
mkdir -p "$INITRD_ROOT/lib/modules"
mkdir -p "$INITRD_ROOT/dev" "$INITRD_ROOT/proc" "$INITRD_ROOT/sys"

echo "=== Step 3: Gathering LIDE binaries and host binaries ==="
# We will copy basic tools to the rootfs
BINARIES=(
    # Core utilities
    "/bin/bash"
    "/usr/bin/busybox"
    "/usr/bin/sudo"
    "/bin/su"
    "/usr/bin/nano"
    "/usr/bin/curl"
    "/usr/bin/htop"
    "/usr/bin/tree"
    "/usr/bin/Xorg"
    "/usr/lib/xorg/Xorg"
    "/usr/lib/xorg/Xorg.wrap"
    "/usr/bin/xinit"
    "/usr/bin/feh"
    "/usr/bin/dbus-daemon"
    "/usr/bin/dbus-launch"
    "/usr/bin/dbus-uuidgen"
    "/lib/systemd/systemd-udevd"
    "/usr/bin/udevadm"
    "/bin/kmod"
    "/usr/bin/xterm"
    # X11 utilities
    "/usr/bin/xsetroot"
    "/usr/bin/xrdb"
    # XKB utilities (required for Xorg keyboard initialization)
    "/usr/bin/xkbcomp"
    "/usr/bin/setxkbmap"
    # LIDE binaries compiled in the workspace
    "$WORKSPACE/blackline-wm"
    "$WORKSPACE/blackline-panel"
    "$WORKSPACE/blackline-launcher"
    "$WORKSPACE/blackline-tools"
    "$WORKSPACE/blackline-background"
    "$WORKSPACE/blackline-fm"
    "$WORKSPACE/blackline-editor"
    "$WORKSPACE/blackline-calculator"
    "$WORKSPACE/blackline-system-monitor"
    "$WORKSPACE/blackline-clipboard"
    "$WORKSPACE/voidfox"
    "$WORKSPACE/tools/firefox/firefox-wrapper"
    "$WORKSPACE/blackline-terminal"
    "$WORKSPACE/blackline-image-viewer"
    "$WORKSPACE/blackline-file-roller"
    "$WORKSPACE/blackline-settings"
    "$WORKSPACE/blackline-command-palette"
    "$WORKSPACE/blackline-session"
)

# Helper function to copy library dependencies using ldd
copy_deps() {
    local file="$1"
    local dest="$2"
    # Follow symlinks for libraries
    ldd "$file" 2>/dev/null | grep -o '/[^ ]*' | while read -r lib; do
        if [ -f "$lib" ]; then
            local parent_dir=$(dirname "$lib")
            mkdir -p "$dest/$parent_dir"
            cp -L -n "$lib" "$dest/$lib" 2>/dev/null || true
        fi
    done
}

echo "Copying binaries and their dependencies into RootFS..."
for bin in "${BINARIES[@]}"; do
    if [ -f "$bin" ]; then
        # Determine the target path
        dest_path=""
        if [[ "$bin" == "$WORKSPACE/"* ]]; then
            # LIDE binaries go to /usr/local/bin
            name=$(basename "$bin")
            dest_path="/usr/local/bin/$name"
        else
            dest_path="$bin"
        fi
        
        mkdir -p "$RFS_DIR/$(dirname "$dest_path")"
        cp -L "$bin" "$RFS_DIR/$dest_path"
        copy_deps "$bin" "$RFS_DIR"
    else
        echo "Warning: Binary not found: $bin"
    fi
done

echo "Copying VoidFox browser homepage assets..."
cp -L "$WORKSPACE/tools/web-browser/homePage.html" "$RFS_DIR/usr/local/bin/"
cp -L "$WORKSPACE/tools/web-browser/homePage.css" "$RFS_DIR/usr/local/bin/"
cp -L "$WORKSPACE/tools/web-browser/homePage.js" "$RFS_DIR/usr/local/bin/"


echo "Installing BusyBox symlinks into rootfs..."
if [ -f "$RFS_DIR/usr/bin/busybox" ]; then
    for path in $("$RFS_DIR/usr/bin/busybox" --list-full); do
        target_path="$RFS_DIR/$path"
        mkdir -p "$(dirname "$target_path")"
        if [ ! -e "$target_path" ] && [ ! -L "$target_path" ]; then
            ln -sf /usr/bin/busybox "$target_path"
        fi
    done
else
    echo "ERROR: busybox not found in rootfs"
    exit 1
fi

echo "Configuring sudo policies and plugins..."
mkdir -p "$RFS_DIR/usr/libexec/sudo"
cp -a /usr/libexec/sudo/* "$RFS_DIR/usr/libexec/sudo/"
mkdir -p "$RFS_DIR/etc"
cp -p /etc/sudo.conf "$RFS_DIR/etc/" 2>/dev/null || true
cat << 'EOF' > "$RFS_DIR/etc/sudoers"
defaults        env_reset
defaults        mail_badpass
defaults        secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

root    ALL=(ALL:ALL) ALL
%admin  ALL=(ALL) ALL
%sudo   ALL=(ALL:ALL) ALL

# Allow live user to run all commands without password
live    ALL=(ALL:ALL) NOPASSWD: ALL
EOF
chmod 0440 "$RFS_DIR/etc/sudoers"
chmod 4755 "$RFS_DIR/usr/bin/sudo"
chmod 4755 "$RFS_DIR/usr/bin/su" 2>/dev/null || true

echo "=== Step 4: Copying Xorg Modules & configuration ==="
# Xorg modules
mkdir -p "$RFS_DIR/usr/lib/xorg/modules"
cp -a /usr/lib/xorg/modules/* "$RFS_DIR/usr/lib/xorg/modules/"
# Gather dependencies for xorg modules too
find /usr/lib/xorg/modules/ -name "*.so" | while read -r mod; do
    copy_deps "$mod" "$RFS_DIR"
done

# Mesa DRI drivers and GTK3 dynamic modules
mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/dri"
cp -a /usr/lib/x86_64-linux-gnu/dri/* "$RFS_DIR/usr/lib/x86_64-linux-gnu/dri/" 2>/dev/null || true

# Mesa GBM library (required by Xorg/Mesa loader)
mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/gbm"
cp -a /usr/lib/x86_64-linux-gnu/gbm/* "$RFS_DIR/usr/lib/x86_64-linux-gnu/gbm/" 2>/dev/null || true

mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0"
cp -a /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/* "$RFS_DIR/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/" 2>/dev/null || true

mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/gtk-3.0"
cp -a /usr/lib/x86_64-linux-gnu/gtk-3.0/* "$RFS_DIR/usr/lib/x86_64-linux-gnu/gtk-3.0/" 2>/dev/null || true

# Gather dependencies for DRI, GBM, and GTK modules
find "$RFS_DIR/usr/lib/x86_64-linux-gnu/dri/" "$RFS_DIR/usr/lib/x86_64-linux-gnu/gbm/" "$RFS_DIR/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/" "$RFS_DIR/usr/lib/x86_64-linux-gnu/gtk-3.0/" -name "*.so" 2>/dev/null | while read -r mod; do
    copy_deps "$mod" "$RFS_DIR"
done

# Set GTK3 Default Theme to Adwaita-dark
echo "Configuring GTK3 default theme (Adwaita-dark)..."
mkdir -p "$RFS_DIR/etc/gtk-3.0"
cat <<GTK_SETTINGS > "$RFS_DIR/etc/gtk-3.0/settings.ini"
[Settings]
gtk-theme-name=Adwaita
gtk-icon-theme-name=Adwaita
gtk-cursor-theme-name=Adwaita
gtk-font-name=Sans 10
gtk-application-prefer-dark-theme=1
GTK_SETTINGS

# Also put it in user home to ensure it is read
mkdir -p "$RFS_DIR/home/live/.config/gtk-3.0"
cp "$RFS_DIR/etc/gtk-3.0/settings.ini" "$RFS_DIR/home/live/.config/gtk-3.0/settings.ini"


# WebKit2GTK helper processes (essential for spawning web views)
echo "Copying WebKit2GTK helper processes..."
if [ -d /usr/lib/x86_64-linux-gnu/webkit2gtk-4.1 ]; then
    mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/webkit2gtk-4.1"
    cp -a /usr/lib/x86_64-linux-gnu/webkit2gtk-4.1/* "$RFS_DIR/usr/lib/x86_64-linux-gnu/webkit2gtk-4.1/"
    find "$RFS_DIR/usr/lib/x86_64-linux-gnu/webkit2gtk-4.1/" -type f -executable -o -name "*.so" 2>/dev/null | while read -r helper; do
        copy_deps "$helper" "$RFS_DIR" 2>/dev/null || true
    done
fi

# Firefox ESR Installation
echo "Copying Firefox ESR..."
if [ -d /usr/lib/firefox-esr ]; then
    mkdir -p "$RFS_DIR/usr/lib/firefox-esr"
    cp -a /usr/lib/firefox-esr/* "$RFS_DIR/usr/lib/firefox-esr/"
    mkdir -p "$RFS_DIR/usr/bin"
    ln -sf /usr/lib/firefox-esr/firefox-esr "$RFS_DIR/usr/bin/firefox-esr"
    ln -sf /usr/lib/firefox-esr/firefox-esr "$RFS_DIR/usr/bin/firefox"
    find "$RFS_DIR/usr/lib/firefox-esr/" -name "*.so" 2>/dev/null | while read -r mod; do
        copy_deps "$mod" "$RFS_DIR" 2>/dev/null || true
    done
fi

# Imlib2 loader plugins (loaded via dlopen at runtime, NOT found by ldd)
# These are essential for the WM to load PNG/JPEG wallpapers via imlib_load_image()
echo "Copying Imlib2 loader and filter plugins..."
if [ -d /usr/lib/x86_64-linux-gnu/imlib2 ]; then
    mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/imlib2"
    cp -a /usr/lib/x86_64-linux-gnu/imlib2/* "$RFS_DIR/usr/lib/x86_64-linux-gnu/imlib2/"
    # Gather dependencies for Imlib2 loader plugins
    find "$RFS_DIR/usr/lib/x86_64-linux-gnu/imlib2/" -name "*.so" 2>/dev/null | while read -r mod; do
        copy_deps "$mod" "$RFS_DIR"
    done
fi

# Copy Xkb configuration for keyboard layouts
mkdir -p "$RFS_DIR/usr/share/X11"
cp -a /usr/share/xkeyboard-config-2 "$RFS_DIR/usr/share/X11/xkb" || cp -a /usr/share/X11/xkb "$RFS_DIR/usr/share/X11/xkb"

# Copy Xorg xorg.conf.d snippets (especially 40-libinput.conf for input device support)
if [ -d /usr/share/X11/xorg.conf.d ]; then
    echo "Copying Xorg xorg.conf.d input configuration..."
    mkdir -p "$RFS_DIR/usr/share/X11/xorg.conf.d"
    cp -a /usr/share/X11/xorg.conf.d/* "$RFS_DIR/usr/share/X11/xorg.conf.d/"
fi

# Copy Glib schemas for settings stability
mkdir -p "$RFS_DIR/usr/share/glib-2.0"
cp -a /usr/share/glib-2.0/schemas "$RFS_DIR/usr/share/glib-2.0/"
# Compile schemas inside rootfs
glib-compile-schemas "$RFS_DIR/usr/share/glib-2.0/schemas/"

echo "=== Copying udev rules and helpers ==="
mkdir -p "$RFS_DIR/lib/udev"
cp -a /lib/udev/* "$RFS_DIR/lib/udev/"
rm -f "$RFS_DIR/lib/udev/hwdb.bin"
rm -rf "$RFS_DIR/lib/udev/hwdb.d"
# Gather dependencies for udev helper binaries
find "$RFS_DIR/lib/udev/" -type f -executable | while read -r helper; do
    copy_deps "$helper" "$RFS_DIR"
done

echo "=== Step 5: Copying themes, wallpapers and assets ==="
# Copy fonts
mkdir -p "$RFS_DIR/usr/share/fonts/truetype"
cp -a /usr/share/fonts/truetype/dejavu "$RFS_DIR/usr/share/fonts/truetype/" 2>/dev/null || true
cp -a /usr/share/fonts/truetype/liberation "$RFS_DIR/usr/share/fonts/truetype/" 2>/dev/null || true

# Copy X11 bitmap fonts (cursor font is essential for mouse pointer)
mkdir -p "$RFS_DIR/usr/share/fonts/X11/misc"
cp -a /usr/share/fonts/X11/misc/* "$RFS_DIR/usr/share/fonts/X11/misc/" 2>/dev/null || true
# Copy font aliases and type1 fonts if present
if [ -d /usr/share/fonts/X11/Type1 ]; then
    mkdir -p "$RFS_DIR/usr/share/fonts/X11/Type1"
    cp -a /usr/share/fonts/X11/Type1/* "$RFS_DIR/usr/share/fonts/X11/Type1/" 2>/dev/null || true
fi
# Copy font tools for runtime font index generation
for tool in mkfontdir mkfontscale; do
    if command -v "$tool" >/dev/null 2>&1; then
        cp "$(which $tool)" "$RFS_DIR/usr/bin/" 2>/dev/null || true
        copy_deps "$(which $tool)" "$RFS_DIR"
    fi
done
# Ensure font index is built at build time too
mkfontdir "$RFS_DIR/usr/share/fonts/X11/misc" 2>/dev/null || true

# Copy emoji font (needed for panel emoji icons like 🔒)
if [ -f /usr/share/fonts/truetype/noto/NotoColorEmoji.ttf ]; then
    echo "Copying Noto Color Emoji font..."
    mkdir -p "$RFS_DIR/usr/share/fonts/truetype/noto"
    cp -a /usr/share/fonts/truetype/noto/NotoColorEmoji.ttf "$RFS_DIR/usr/share/fonts/truetype/noto/"
fi

# Copy Adwaita cursor theme for proper mouse pointer display
if [ -d /usr/share/icons/Adwaita ]; then
    echo "Copying Adwaita cursor theme..."
    mkdir -p "$RFS_DIR/usr/share/icons/Adwaita"
    cp -a /usr/share/icons/Adwaita/* "$RFS_DIR/usr/share/icons/Adwaita/"
fi

# Copy hicolor icon theme
if [ -d /usr/share/icons/hicolor ]; then
    echo "Copying hicolor icon theme..."
    mkdir -p "$RFS_DIR/usr/share/icons/hicolor"
    cp -a /usr/share/icons/hicolor/* "$RFS_DIR/usr/share/icons/hicolor/"
fi

# Copy mime database
if [ -d /usr/share/mime ]; then
    echo "Copying mime database..."
    mkdir -p "$RFS_DIR/usr/share/mime"
    cp -a /usr/share/mime/* "$RFS_DIR/usr/share/mime/"
fi

# Copy Fontconfig configuration
if [ -d /etc/fonts ]; then
    echo "Copying Fontconfig configuration..."
    mkdir -p "$RFS_DIR/etc/fonts"
    cp -a /etc/fonts/* "$RFS_DIR/etc/fonts/"
fi
if [ -d /usr/share/fontconfig ]; then
    echo "Copying Fontconfig shared configuration..."
    mkdir -p "$RFS_DIR/usr/share/fontconfig"
    cp -a /usr/share/fontconfig/* "$RFS_DIR/usr/share/fontconfig/"
fi

# Copy glycin configuration and loaders
echo "Copying glycin configuration and loaders..."
if [ -d /usr/share/glycin-loaders ]; then
    mkdir -p "$RFS_DIR/usr/share/glycin-loaders"
    cp -a /usr/share/glycin-loaders/* "$RFS_DIR/usr/share/glycin-loaders/"
fi
if [ -d /usr/libexec/glycin-loaders ]; then
    mkdir -p "$RFS_DIR/usr/libexec/glycin-loaders"
    cp -a /usr/libexec/glycin-loaders/* "$RFS_DIR/usr/libexec/glycin-loaders/"
    # Gather dependencies for glycin helpers too
    find "$RFS_DIR/usr/libexec/glycin-loaders/" -type f -executable | while read -r helper; do
        copy_deps "$helper" "$RFS_DIR"
    done
fi

# Copy bubblewrap (bwrap) for sandboxing
if command -v bwrap >/dev/null 2>&1; then
    cp "$(which bwrap)" "$RFS_DIR/usr/bin/bwrap"
    copy_deps "$(which bwrap)" "$RFS_DIR"
fi

# Copy gdk-pixbuf-query-loaders
if [ -f /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders ]; then
    mkdir -p "$RFS_DIR/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0"
    cp /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders "$RFS_DIR/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/"
    copy_deps "/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders" "$RFS_DIR"
fi
# Set default cursor theme
mkdir -p "$RFS_DIR/usr/share/icons/default"
cat <<CURSOR_THEME > "$RFS_DIR/usr/share/icons/default/index.theme"
[Icon Theme]
Inherits=Adwaita
CURSOR_THEME

# Copy workspace assets
mkdir -p "$RFS_DIR/usr/share/lide"
cp -a "$WORKSPACE/images" "$RFS_DIR/usr/share/lide/"
cp -a "$WORKSPACE/themes" "$RFS_DIR/usr/share/lide/"

echo "=== Step 6: Configuring rootfs scripts and files ==="
    mkdir -p "$RFS_DIR/proc" "$RFS_DIR/sys" "$RFS_DIR/dev" "$RFS_DIR/tmp" "$RFS_DIR/run" "$RFS_DIR/mnt" "$RFS_DIR/sbin"
    mkdir -p "$RFS_DIR/var/log" "$RFS_DIR/var/lib/dbus" "$RFS_DIR/var/tmp" "$RFS_DIR/var/run"
    mkdir -p "$RFS_DIR/home/live/.config/blackline"
    mkdir -p "$RFS_DIR/etc/X11"

    # Xorg configuration with explicit font paths and input device support
    cat <<XORGCONF > "$RFS_DIR/etc/X11/xorg.conf"
Section "ServerFlags"
    Option "AutoAddDevices" "true"
    Option "AllowMouseOpenFail" "true"
EndSection

Section "Files"
    FontPath "/usr/share/fonts/X11/misc"
    FontPath "/usr/share/fonts/X11/Type1"
    FontPath "/usr/share/fonts/truetype/dejavu"
    FontPath "/usr/share/fonts/truetype/liberation"
    FontPath "/usr/share/fonts/truetype/noto"
EndSection

Section "Device"
    Identifier "Card0"
    Driver "modesetting"
EndSection

Section "Monitor"
    Identifier "Monitor0"
EndSection

Section "Screen"
    Identifier "Screen0"
    Device "Card0"
    Monitor "Monitor0"
    DefaultDepth 24
    SubSection "Display"
        Depth 24
        Modes "1280x720" "1024x768" "800x600"
    EndSubSection
EndSection

Section "ServerLayout"
    Identifier "Default"
    Screen "Screen0" 0 0
EndSection
XORGCONF

    # Copy DBus configurations
    mkdir -p "$RFS_DIR/usr/share/dbus-1"
    cp -a /usr/share/dbus-1/* "$RFS_DIR/usr/share/dbus-1/"
    mkdir -p "$RFS_DIR/etc/dbus-1"
    cp -a /etc/dbus-1/* "$RFS_DIR/etc/dbus-1/"

    # Copy kernel modules to rootfs
    mkdir -p "$RFS_DIR/lib/modules"
    cp -a "/lib/modules/$KERNEL_VERSION" "$RFS_DIR/lib/modules/"

    # Create kmod symlinks in RootFS
    ln -sf kmod "$RFS_DIR/bin/lsmod"
    ln -sf ../bin/kmod "$RFS_DIR/sbin/depmod"
    ln -sf ../bin/kmod "$RFS_DIR/sbin/insmod"
    ln -sf ../bin/kmod "$RFS_DIR/sbin/modprobe"
    ln -sf ../bin/kmod "$RFS_DIR/sbin/rmmod"

    # Run depmod to generate module dependency files for the guest kernel
    depmod -a -b "$RFS_DIR" "$KERNEL_VERSION"

    # Xwrapper.config - allow Xorg to run from console
    cat <<XWRAP > "$RFS_DIR/etc/X11/Xwrapper.config"
allowed_users=anybody
needs_root_rights=yes
XWRAP

# Basic config files
cat <<EOF > "$RFS_DIR/etc/passwd"
root:x:0:0:root:/root:/bin/bash
live:x:1000:1000:live user:/home/live:/bin/bash
messagebus:x:988:988:System Message Bus:/nonexistent:/usr/sbin/nologin
EOF

cat <<EOF > "$RFS_DIR/etc/group"
root:x:0:
live:x:1000:
audio:x:29:live
video:x:44:live
input:x:996:live
messagebus:x:988:
EOF

cat <<EOF > "$RFS_DIR/etc/nsswitch.conf"
passwd:         files
group:          files
shadow:         files
hosts:          files dns
networks:       files
protocols:      db files
services:       db files
ethers:         db files
rpc:            db files
EOF

# Pre-configure wallpaper
cat <<EOF > "$RFS_DIR/home/live/.config/blackline/current_wallpaper.conf"
/usr/share/lide/images/walpapers/wal1.png
EOF

# Set custom terminal prompt
cat <<EOF > "$RFS_DIR/home/live/.bashrc"
PS1="blackline @ \W # "
EOF
cat <<EOF > "$RFS_DIR/etc/bash.bashrc"
PS1="blackline @ \W # "
EOF

chown -R 1000:1000 "$RFS_DIR/home/live" 2>/dev/null || true

# Write rootfs init (this acts as PID 1 inside the running OS)
rm -f "$RFS_DIR/sbin/init"
cat <<'EOF' > "$RFS_DIR/sbin/init"
#!/bin/bash
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
export HOME=/home/live
export GTK_THEME=Adwaita:dark
export XCURSOR_THEME=Adwaita
export XCURSOR_SIZE=24
export XCURSOR_PATH=/usr/share/icons:/usr/share/pixmaps

# Mount system filesystems
mount -t proc proc /proc 2>/dev/null || true
mount -t sysfs sysfs /sys 2>/dev/null || true
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true
mkdir -p /dev/pts
mount -t devpts devpts /dev/pts 2>/dev/null || true
mount -t tmpfs tmpfs /tmp 2>/dev/null || true
mount -t tmpfs tmpfs /run 2>/dev/null || true
mkdir -p /run/dbus

# Populate device nodes and run udev
/lib/systemd/systemd-udevd --daemon
udevadm trigger --action=add
udevadm settle

# Load GPU and input drivers explicitly
modprobe virtio-gpu 2>/dev/null || true
modprobe virtio_input 2>/dev/null || true
modprobe xhci-pci 2>/dev/null || true
modprobe ohci-pci 2>/dev/null || true
modprobe ehci-pci 2>/dev/null || true
modprobe uhci-hcd 2>/dev/null || true
modprobe evdev 2>/dev/null || true
modprobe psmouse 2>/dev/null || true
modprobe usbhid 2>/dev/null || true
modprobe hid 2>/dev/null || true
modprobe hid_generic 2>/dev/null || true

# Wait for /dev/dri/card0 to appear (up to 10 seconds)
echo "Waiting for GPU device..."
for i in $(seq 1 20); do
    if [ -e /dev/dri/card0 ]; then
        echo "GPU device ready: /dev/dri/card0"
        break
    fi
    sleep 0.5
done
if [ ! -e /dev/dri/card0 ]; then
    echo "WARNING: /dev/dri/card0 not found, Xorg may fail"
fi

# Wait for input devices to be ready
echo "Waiting for input devices..."
for i in $(seq 1 20); do
    if [ -e /dev/input/mice ] || [ -e /dev/input/event0 ]; then
        echo "Input devices ready"
        break
    fi
    sleep 0.5
done
chmod 666 /dev/input/mice 2>/dev/null || true
chmod 666 /dev/input/event* 2>/dev/null || true

# Start D-Bus
dbus-uuidgen --ensure
dbus-daemon --system --fork

# Set desktop launcher symlinks or assets in the home directory
mkdir -p /home/live/Desktop/LIDE
ln -sf /usr/share/lide/images /home/live/Desktop/LIDE/images
chown -R 1000:1000 /home/live

# Regenerate gdk-pixbuf loaders cache
if [ -f /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders ]; then
    mkdir -p /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/2.10.0
    /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/gdk-pixbuf-query-loaders > /usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/2.10.0/loaders.cache 2>/dev/null || true
fi

# Build X11 font indexes so Xorg can find the cursor font
if command -v mkfontdir >/dev/null 2>&1; then
    mkfontdir /usr/share/fonts/X11/misc 2>/dev/null || true
fi
if command -v mkfontscale >/dev/null 2>&1; then
    mkfontscale /usr/share/fonts/X11/misc 2>/dev/null || true
fi
# Ensure fc-cache picks up all fonts
fc-cache -f 2>/dev/null || true

# Force desktop size and run X
echo "Starting BlackLine session..."
cd /usr/local/bin
if ! xinit ./blackline-session -- /usr/bin/Xorg -nolisten tcp vt1 >/dev/console 2>&1; then
    echo "ERROR: Xorg or session manager failed to start!" >/dev/console
    echo "=== Xorg log ===" >/dev/console
    cat /var/log/Xorg.0.log 2>/dev/null >/dev/console
    echo "Starting emergency shell..." >/dev/console
    exec /bin/bash </dev/console >/dev/console 2>&1
fi
EOF
chmod +x "$RFS_DIR/sbin/init"

echo "=== Step 7: Creating the Initrd ==="
# Copy busybox and basic tools to initrd
cp -L /usr/bin/busybox "$INITRD_ROOT/bin/busybox"
copy_deps "/usr/bin/busybox" "$INITRD_ROOT"

# Copy required kernel modules for booting
MODULES=(
    "kernel/drivers/cdrom/cdrom.ko.xz"
    "kernel/drivers/scsi/scsi_common.ko.xz"
    "kernel/drivers/scsi/scsi_mod.ko.xz"
    "kernel/drivers/scsi/sr_mod.ko.xz"
    "kernel/drivers/ata/libata.ko.xz"
    "kernel/drivers/ata/libahci.ko.xz"
    "kernel/drivers/ata/ahci.ko.xz"
    "kernel/drivers/ata/ata_piix.ko.xz"
    "kernel/fs/isofs/isofs.ko.xz"
    "kernel/drivers/block/loop.ko.xz"
    "kernel/fs/squashfs/squashfs.ko.xz"
    "kernel/fs/overlayfs/overlay.ko.xz"
)
for mod in "${MODULES[@]}"; do
    src="/lib/modules/$KERNEL_VERSION/$mod"
    if [ -f "$src" ]; then
        xzcat "$src" > "$INITRD_ROOT/lib/modules/$(basename "$mod" .xz)"
    else
        # Try uncompressed
        uncompressed="/lib/modules/$KERNEL_VERSION/${mod%.xz}"
        if [ -f "$uncompressed" ]; then
            cp "$uncompressed" "$INITRD_ROOT/lib/modules/$(basename "$uncompressed")"
        else
            echo "Warning: Module not found: $mod"
        fi
    fi
done

# Write initrd init script
cat <<'EOF' > "$INITRD_ROOT/init"
#!/bin/busybox sh

# Redirect stdin/stdout/stderr to console
exec </dev/console >/dev/console 2>&1

echo "=== BlackLine Initrd Booting ==="

/bin/busybox mount -t devtmpfs devtmpfs /dev
/bin/busybox mount -t proc proc /proc
/bin/busybox mount -t sysfs sysfs /sys

/bin/busybox mkdir -p /mnt/iso /mnt/squashfs /mnt/overlaydir /mnt/newroot

# Load modules
echo "Loading kernel modules..."
/bin/busybox insmod /lib/modules/cdrom.ko
/bin/busybox insmod /lib/modules/scsi_common.ko
/bin/busybox insmod /lib/modules/scsi_mod.ko
/bin/busybox insmod /lib/modules/sr_mod.ko
/bin/busybox insmod /lib/modules/libata.ko
/bin/busybox insmod /lib/modules/ata_piix.ko
/bin/busybox insmod /lib/modules/libahci.ko
/bin/busybox insmod /lib/modules/ahci.ko
/bin/busybox insmod /lib/modules/isofs.ko
/bin/busybox insmod /lib/modules/loop.ko
/bin/busybox insmod /lib/modules/squashfs.ko
/bin/busybox insmod /lib/modules/overlay.ko

# Try to find the ISO live directory with retries to allow devices to settle
found=0
echo "Searching for live filesystem..."
for attempt in 1 2 3 4 5; do
    echo "Search attempt $attempt/5..."
    for dev in /dev/sr* /dev/sd* /dev/vd*; do
        if [ -b "$dev" ]; then
            echo "Testing device: $dev"
            if /bin/busybox mount -o ro "$dev" /mnt/iso 2>/dev/null; then
                if [ -f /mnt/iso/live/filesystem.squashfs ]; then
                    echo "Success: Found live filesystem on $dev"
                    found=1
                    break 2
                else
                    /bin/busybox umount /mnt/iso
                fi
            fi
        fi
    done
    /bin/busybox sleep 2
done

if [ "$found" -ne 1 ]; then
    echo "ERROR: Live filesystem not found!"
    echo "Available block devices:"
    /bin/busybox ls -l /dev/sd* /dev/sr* /dev/vd* 2>/dev/null
    exec /bin/busybox sh
fi

/bin/busybox mount -t squashfs -o loop,ro /mnt/iso/live/filesystem.squashfs /mnt/squashfs
/bin/busybox mount -t tmpfs tmpfs /mnt/overlaydir
/bin/busybox mkdir -p /mnt/overlaydir/upper /mnt/overlaydir/work
if ! /bin/busybox mount -t overlay overlay -o lowerdir=/mnt/squashfs,upperdir=/mnt/overlaydir/upper,workdir=/mnt/overlaydir/work /mnt/newroot; then
    echo "ERROR: Failed to mount overlay root filesystem"
    exec /bin/busybox sh
fi

# Move virtual filesystems to the new root
/bin/busybox mkdir -p /mnt/newroot/mnt/iso
/bin/busybox mount --move /mnt/iso /mnt/newroot/mnt/iso
/bin/busybox mount --move /dev /mnt/newroot/dev
/bin/busybox mount --move /proc /mnt/newroot/proc
/bin/busybox mount --move /sys /mnt/newroot/sys

echo "Switching to rootfs..."
exec /bin/busybox switch_root /mnt/newroot /sbin/init
EOF
chmod +x "$INITRD_ROOT/init"

# Pack the custom initrd
echo "Packing initrd.img..."
cd "$INITRD_ROOT"
find . -print0 | cpio --null -ov --format=newc | gzip -9 > "$BUILD_DIR/boot/initrd.img"
cd "$WORKSPACE"

echo "=== Step 8: Creating SquashFS ==="
mksquashfs "$RFS_DIR" "$BUILD_DIR/live/filesystem.squashfs" -noappend -comp xz -all-root

echo "=== Step 9: Creating ISO Image ==="
cp "$VMLINUZ" "$BUILD_DIR/boot/vmlinuz"

cat <<EOF > "$BUILD_DIR/boot/grub/grub.cfg"
set default=0
set timeout=3

menuentry "BlackLine OS Live" {
    linux /boot/vmlinuz console=tty0 console=ttyS0
    initrd /boot/initrd.img
}
EOF

grub-mkrescue -o "$ISO_DIR/blackline.iso" "$BUILD_DIR"

echo "=== ISO creation complete! ==="
ls -lh "$ISO_DIR/blackline.iso"
