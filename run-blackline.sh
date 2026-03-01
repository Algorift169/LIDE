#!/bin/bash
cd /home/israfil/Desktop/LIDE

echo "Starting BlackLine in Xephyr..."

# Kill existing Xephyr
pkill Xephyr 2>/dev/null
sleep 1

# Start Xephyr
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 2

export DISPLAY=:1

# Set wallpaper (run once)
echo "Setting wallpaper..."
./blackline-wallpaper

# Start panel
echo "Starting panel..."
./blackline-panel &
PANEL_PID=$!
sleep 1

# Start window manager LAST
echo "Starting window manager..."
./blackline-wm &
WM_PID=$!

echo ""
echo "BlackLine is running. Click buttons to test:"
echo "- BlackLine button → launcher"
echo "- Tools button → tools container"
echo "- Super+Enter → terminal"
echo "- Super+Space → launcher"
echo ""
echo "Press Ctrl+C to kill everything"
echo ""

# Wait for Ctrl+C
trap "kill $PANEL_PID $WM_PID $XEPHYR_PID 2>/dev/null; pkill Xephyr; echo 'Cleanup complete'; exit" INT
wait
