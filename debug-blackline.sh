#!/bin/bash
echo "Starting BlackLine Debug Session"
Xephyr :1 -screen 1024x768 -ac &
XEPHYR_PID=$!
sleep 2
export DISPLAY=:1
./blackline-wm > wm.log 2>&1 &
WM_PID=$!
sleep 1
./blackline-panel > panel.log 2>&1 &
PANEL_PID=$!
./blackline-wallpaper
echo "Components started. Check logs:"
echo "  wm.log: window manager"
echo "  panel.log: panel"
echo ""
echo "Press Enter to kill all processes"
read
kill $WM_PID $PANEL_PID $XEPHYR_PID 2>/dev/null
echo "Debug session ended"
