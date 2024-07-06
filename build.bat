@echo off
mkdir build
pushd build
cl -FC -Zi ..\code\win32_chip8.c user32.lib gdi32.lib
popd