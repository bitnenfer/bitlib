@echo off

set Arch=x64
call build\build_windows_release.bat
call build\build_windows.bat
