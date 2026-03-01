#!/bin/bash
cd ~/Desktop/LIDE

echo "Starting BlackLine..."

# Kill existing processes
pkill Xephyr 2>/dev/null
pkill blackline-background 2>/dev/null
pkill blackline-panel 2>/dev/null
pkill blackline-wm 2>/dev/null
sleep 1

# Start Xephyr
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 2

export DISPLAY=:1

# Start background (wallpaper)
./blackline-background &
BACKGROUND_PID=$!

# Start panel
./blackline-panel &
PANEL_PID=$!

# Start window manager
./blackline-wm &
WM_PID=$!

echo "BlackLine is running!"
echo "Press Ctrl+C to stop"

# Wait for Ctrl+C
trap "kill $BACKGROUND_PID $PANEL_PID $WM_PID $XEPHYR_PID 2>/dev/null; echo 'Stopped'; exit" INT
wait
