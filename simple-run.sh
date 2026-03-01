#!/bin/bash
cd ~/Desktop/LIDE

pkill Xephyr
sleep 1

Xephyr :1 -screen 1024x768 -ac &
sleep 2
export DISPLAY=:1

./blackline-wallpaper
./blackline-panel &
./blackline-wm &

echo "BlackLine running. Press Enter to kill"
read
pkill Xephyr
