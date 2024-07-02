@echo off
mkdir build
pushd build
gcc ..\code\win32_chip8.c -o win32_chip8.exe -Wall -Werror -Wextra -std=c99 -Wno-unused-parameter -g -luser32 -lgdi32
popd