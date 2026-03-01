# LIDE - Lightweight Integrated Desktop Environment

LIDE is a minimal desktop environment written in C, using Xlib for window management and GTK3 for the panel and launcher. It follows a modular architecture and aims for simplicity and performance.

## Components

- **lide-wm**: Basic window manager with keyboard shortcuts (Super+Enter: terminal, Super+q: close, Super+Space: launcher).
- **lide-panel**: Top panel with clock and application launcher button.
- **lide-launcher**: Application launcher that reads .desktop files.
- **lide-session**: Session manager that starts the WM and panel.

## Building

Ensure you have development packages for X11 and GTK3 installed. On Debian/Ubuntu:

```bash
sudo apt install libx11-dev libgtk-3-dev