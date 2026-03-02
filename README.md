BlackLine Desktop Environment

A lightweight, modular desktop environment written in C, featuring a custom window manager using Xlib and GTK3-based components. BlackLine (formerly LIDE) is designed for simplicity, performance, and a clean terminal-inspired aesthetic.
Features

    Custom Window Manager (blackline-wm): Xlib-based window manager with:

        Window decorations with title bars

        Draggable windows by title bar

        Keyboard shortcuts (Super+Enter, Super+Q, Super+Space)

        Proper window focus and stacking

        Client window closing support

    Top Panel (blackline-panel): GTK3-based panel with:

        Dark theme (#0b0f14 background, #00ff88 accent)

        Live clock display

        "BlackLine" button to launch application launcher

        "Tools" button to open tools container

        Full-width dock-style window

    Application Launcher (blackline-launcher): GTK3-based app launcher that:

        Reads .desktop files from /usr/share/applications

        Search-as-you-type functionality

        Dark themed interface

        Launches applications with a single click

    Tools Container (blackline-tools): Utility window with:

        Quick access to common applications (Terminal, File Manager, Text Editor, Calculator, System Monitor)

        Close button and focus-out auto-close

        Dark theme matching the desktop aesthetic

    Wallpaper Service (blackline-background): Continuous background setter that:

        Uses feh to set wallpaper from ~/Desktop/LIDE/images/wal1.png

        Falls back to solid color (#0b0f14) if wallpaper not found

        Runs continuously to ensure wallpaper persists

    Session Manager (blackline-session): Starts all components in the correct order:

        Wallpaper service → Panel → Window Manager

Keyboard Shortcuts
Shortcut	Action
Super+Enter	Launch terminal (xterm)
Super+Q	Close focused window
Super+Space	Open application launcher
Project Structure

BlackLine/
├── wm/                    # Window manager
│   ├── wm.c              # Main WM event loop
│   ├── keybinds.c        # Keyboard shortcut handling
│   └── wm.h              # WM headers
├── panel/                 # Top panel
│   ├── panel.c           # Panel GUI and logic
│   └── clock.c           # Clock update function
├── launcher/              # Application launcher
│   ├── launcher.c        # Launcher GUI and .desktop parsing
│   └── launcher.h        # Launcher headers
├── tools/                 # Tools container and utilities
│   ├── tools_container.c # Tools window
│   ├── wallpaper.c       # Wallpaper setter
│   └── background.c      # Continuous wallpaper service
├── session/               # Session manager
│   └── session.c         # Process launcher
├── themes/                # Theme files
│   ├── Xresources        # xterm configuration
│   └── lide.css          # GTK CSS styling
├── images/                # Wallpaper images
│   └── wal1.png          # Default wallpaper
├── include/               # Header files (future use)
├── Makefile               # Build configuration
└── README.md              # This file

Building from Source
Dependencies
# Debian/Ubuntu/Kali
sudo apt install libx11-dev libgtk-3-dev pkg-config feh xterm

# For wallpaper support
sudo apt install feh

Build Commands
# Clone the repository
git clone https://github.com/Algorift169/LIDE.git
cd LIDE

# Build all components
make clean
make

# Install to ~/.local/bin/
make install

# Or install system-wide
sudo make install

Running BlackLine
In Xephyr (for testing)
# Run the test script
./run.sh

# Or manually
Xephyr :1 -screen 1024x768 -ac &
export DISPLAY=:1
./blackline-background &
./blackline-panel &
./blackline-wm

===================================================

Just the Beginning...

This is just the start of BlackLine Desktop Environment. The foundation has been laid, but many more tools and features are planned for the future.
