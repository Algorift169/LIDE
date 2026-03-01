#!/bin/bash
cd ~/Desktop/LIDE

echo "=== BlackLine Diagnostic ==="
echo "Current directory: $(pwd)"
echo ""

echo "=== Checking binaries ==="
for bin in blackline-wm blackline-panel blackline-launcher blackline-tools blackline-wallpaper; do
    if [ -f "./$bin" ]; then
        echo "✓ $bin exists"
        ls -la "./$bin"
    else
        echo "✗ $bin MISSING"
    fi
done
echo ""

echo "=== Checking wallpaper ==="
if [ -f "./images/wal1.png" ]; then
    echo "✓ Wallpaper found at ./images/wal1.png"
    file "./images/wal1.png"
else
    echo "✗ Wallpaper not found at ./images/wal1.png"
fi
echo ""

echo "=== Checking Xephyr ==="
if command -v Xephyr &> /dev/null; then
    echo "✓ Xephyr installed: $(which Xephyr)"
else
    echo "✗ Xephyr not found"
fi
echo ""

echo "=== Checking wallpaper tools ==="
for tool in feh nitrogen; do
    if command -v $tool &> /dev/null; then
        echo "✓ $tool installed"
    else
        echo "✗ $tool not installed"
    fi
done
echo ""

echo "=== Current directory contents ==="
ls -la
echo ""

echo "=== Running test ==="
echo "Starting Xephyr..."
pkill Xephyr 2>/dev/null
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 3

export DISPLAY=:1

echo "Testing wallpaper setter..."
if [ -f "./blackline-wallpaper" ]; then
    ./blackline-wallpaper
else
    echo "Skipping wallpaper test (binary missing)"
fi

echo ""
echo "Panel should appear. Press Ctrl+C to exit"
if [ -f "./blackline-panel" ]; then
    ./blackline-panel
else
    echo "Panel binary missing, using test launcher instead"
    ./test-launcher.sh &
    sleep 2
    ./test-tools.sh
fi

# Cleanup on exit
trap "kill $XEPHYR_PID 2>/dev/null" EXIT
