@echo off

set Arch=x64
call build\build_windows_debug.bat
call build\build_windows.bat
