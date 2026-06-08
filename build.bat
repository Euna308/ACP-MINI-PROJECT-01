@echo off
echo Compiling 2D Graphics Editor (ncurses)...
gcc -Wall -Wextra -std=c99 graphics.c main.c -o editor.exe -lncursesw
if %ERRORLEVEL% equ 0 (
    echo Compilation successful! Running editor...
    editor.exe
) else (
    echo Compilation failed!
    pause
)
