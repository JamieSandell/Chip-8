@echo off
mkdir build
pushd build
cl -Zi ..\code\win32_chip8.c user32.lib Gdi32.lib
popd