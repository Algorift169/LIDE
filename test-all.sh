#!/bin/bash
cd ~/Desktop/LIDE

echo "=== Testing BlackLine Components ==="

# Kill existing Xephyr
pkill Xephyr
sleep 1

# Start Xephyr
echo "Starting Xephyr..."
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 3

export DISPLAY=:1

# Test 1: Just xterm (to verify X is working)
echo "Test 1: Opening xterm..."
xterm &
sleep 3
kill %1 2>/dev/null

# Test 2: Wallpaper setter
echo "Test 2: Setting wallpaper..."
./blackline-wallpaper
sleep 2

# Test 3: Panel alone
echo "Test 3: Starting panel..."
./blackline-panel &
PANEL_PID=$!
sleep 3

# Test 4: Window manager alone
echo "Test 4: Starting window manager..."
./blackline-wm &
WM_PID=$!
sleep 3

echo ""
echo "All components started. You should see:"
echo "  - A panel at the top"
echo "  - A terminal window"
echo "  - The wallpaper"
echo ""
echo "Press Enter to open another terminal for testing"
read

DISPLAY=:1 xterm &

echo ""
echo "Press Enter to kill everything and exit"
read

# Cleanup
kill $PANEL_PID $WM_PID $XEPHYR_PID 2>/dev/null
pkill Xephyr
echo "Test complete"