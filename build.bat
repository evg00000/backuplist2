@echo off
rem Build on Windows: w64devkit (i686) or MSYS2 MINGW32 in PATH.
rem Needs a 32-bit toolchain with msvcrt (not UCRT), otherwise the exe will not run on XP.
mingw32-make CROSS= %*
if errorlevel 1 pause
