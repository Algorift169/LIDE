#!/bin/bash
cd ~/Desktop/LIDE

echo "=== Testing Individual Components ==="

# Kill existing Xephyr
pkill Xephyr
sleep 1

# Start Xephyr
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 3

export DISPLAY=:1

# Test 1: Simple xterm (should work)
echo "Test 1: Opening xterm..."
xterm &
sleep 3
kill %1 2>/dev/null

# Test 2: Solid color background
echo "Test 2: Setting solid background..."
xsetroot -solid '#0b0f14'
sleep 2

# Test 3: Panel alone
echo "Test 3: Starting panel alone..."
if [ -f ./blackline-panel ]; then
    ./blackline-panel &
    PANEL_PID=$!
    sleep 5
    kill $PANEL_PID 2>/dev/null
else
    echo "Panel not found!"
fi

# Test 4: Window manager alone
echo "Test 4: Starting window manager alone..."
if [ -f ./blackline-wm ]; then
    ./blackline-wm &
    WM_PID=$!
    sleep 3
    
    # Try to open a window while WM is running
    echo "Opening xterm with WM running..."
    xterm &
    sleep 5
    
    kill $WM_PID 2>/dev/null
else
    echo "WM not found!"
fi

# Test 5: Launcher alone
echo "Test 5: Testing launcher..."
if [ -f ./blackline-launcher ]; then
    ./blackline-launcher &
    LAUNCHER_PID=$!
    sleep 5
    kill $LAUNCHER_PID 2>/dev/null
else
    echo "Launcher not found!"
fi

# Test 6: Tools alone
echo "Test 6: Testing tools..."
if [ -f ./blackline-tools ]; then
    ./blackline-tools &
    TOOLS_PID=$!
    sleep 5
    kill $TOOLS_PID 2>/dev/null
else
    echo "Tools not found!"
fi

# Cleanup
kill $XEPHYR_PID 2>/dev/null
pkill Xephyr
echo "Tests complete"
