@echo off

echo Building Debug...

set Configuration=Debug
set CompilerFlags=/Od /Ob0 /D "_DEBUG" /MDd /ZI
set LinkerFlags=/DEBUG 

