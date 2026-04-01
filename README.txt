Nokia's Snake Game

This project is a simple Snake game written in C using raylib.

Features:
- title screen
- snake movement
- food collection
- score system
- snake growth
- game over and restart

How to compile:
clang -std=c11 -Wall -Wextra -pedantic main.c -o snake $(pkg-config --cflags --libs raylib)

How to run:
./snake

Controls:
- W A S D or arrow keys to move
- Enter to start
- Enter to restart after game over
