@echo off

set Arch=x86
call build\build_windows_debug.bat
call build\build_windows.bat
