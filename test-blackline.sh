#!/bin/bash
echo "=== BlackLine Test Script ==="

# Kill any existing Xephyr instances
pkill Xephyr
sleep 1

# Start Xephyr
echo "Starting Xephyr on :1"
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 3

export DISPLAY=:1

# Test if X server is responding
echo "Testing X server connection..."
xeyes &
EYES_PID=$!
sleep 2
kill $EYES_PID 2>/dev/null

# Try to start just the panel first
echo "Starting panel only..."
./blackline-panel &
PANEL_PID=$!
sleep 3

echo ""
echo "Panel should be visible. If you see a panel, press Enter to continue with WM"
echo "If not, press Ctrl+C to abort"
read

# Kill panel
kill $PANEL_PID 2>/dev/null

# Now try WM alone
echo "Starting window manager only..."
./blackline-wm &
WM_PID=$!
sleep 2

echo ""
echo "WM is running. Try opening xterm manually in another terminal:"
echo "  DISPLAY=:1 xterm"
echo ""
echo "Press Enter to kill everything and exit"
read

# Cleanup
kill $WM_PID $XEPHYR_PID 2>/dev/null
echo "Test complete"