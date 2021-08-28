@echo off
setlocal ENABLEDELAYEDEXPANSION

if %Arch% equ x64 (
    set CompilerArch=amd64
    set CompilerHostArch=x64
    set Machine=X64
    set ArchPath=x64
    set CompilerFlagsArch=/D "_WIN64"
    set NasmArch=win64
) else (
    set CompilerArch=x86
    set CompilerHostArch=x86
    set Machine=X86
    set ArchPath=x86
    set CompilerFlagsArch=/D "_WIN32"
    set NasmArch=win32
)

set MVS2019Path="C:\Program Files (x86)\Microsoft Visual Studio\2019\"
set ProfessionalPath="Professional\Common7\Tools\"
set CommunityPath="Community\Common7\Tools\"
set CustomPath="UserDefinedVS2019Path"
set BuildPath=.\gen\bin\%ArchPath%\%Configuration%\
set IntPath=.\gen\int\%ArchPath%\%Configuration%\
set SrcPath=.\bit\src\bit\
set IncPath=.\bit\include\

if exist %MVS2019Path%%ProfessionalPath% (
    set VCPath=%MVS2019Path%%ProfessionalPath%
) else if exist %MVS2019Path%%CommunityPath% (
    set VCPath=%MVS2019Path%%CommunityPath%
) else if exist %CustomPath% (
    set VCPath=%CustomPath%
) else (
    echo Couldn't find Microsoft Visual Studio 2019 path. Modify the path on the build.bat file.
    goto :end
)

set CurrPath="%cd%"
cd %VCPath%
call VsDevCmd.bat -host_arch=%CompilerHostArch% -arch=%CompilerArch%
cd %CurrPath%

if not exist %BuildPath% (
    mkdir %BuildPath%
)

if not exist %IntPath% (
    mkdir %IntPath%
)

set ClFlags=/std:c++17 %CompilerFlags% %CompilerFlagsArch% /D "BIT_EXPORTING" /nologo /EHsc /I %IncPath%
set LinkFlags=/out:%BuildPath%bit.dll /DYNAMICBASE "kernel32.lib" "user32.lib" /MACHINE:%Machine% /NOLOGO %LinkerFlags%

for %%i in (%SrcPath%windows\%ArchPath%\*.asm) do (
    echo Assembling %%i
    nasm -Xvc -f %NasmArch% -o %IntPath%%%~ni.obj %%i
)

set CxxList=
for %%i in (%SrcPath%*.cpp) do (
    set CxxList=!CxxList! %%i
)
for %%i in (%SrcPath%windows\*.cpp) do (
    set CxxList=!CxxList! %%i
)

cl /c %ClFlags% %CxxList% /Fo:%IntPath%

set ObjList=
for %%i in (%IntPath%*.obj) do (
    set ObjList=!ObjList! %%i
)

link /dll %LinkFlags% %ObjList% 

:end
