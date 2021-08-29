@echo off

echo Building Debug...

set Configuration=Debug
set CompilerFlags=/Od /D "_DEBUG" /MDd
set LinkerFlags=/DEBUG:FULL /PDB:gen/bin/%Arch%/Debug/bit.pdb

