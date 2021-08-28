@echo off

echo Building Release...

set Configuration=Release
set CompilerFlags=/O2 /Ob1 /D "NDEBUG" /MD
set LinkerFlags= 
