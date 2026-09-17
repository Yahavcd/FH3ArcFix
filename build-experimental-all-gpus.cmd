@echo off
setlocal
cd /d "%~dp0"

where cl.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: cl.exe not found.
  echo Run this from an x64 Native Tools Command Prompt for Visual Studio Build Tools.
  exit /b 1
)

if not exist build-experimental mkdir build-experimental
cl.exe /nologo /std:c++17 /EHsc /O2 /MD /LD src\fh3arcfix-experimental-all-gpus.cpp /link /DEF:src\d3d12.def /OUT:build-experimental\d3d12.dll
if errorlevel 1 exit /b 1

echo.
echo Built experimental all-GPUs DLL: %CD%\build-experimental\d3d12.dll
endlocal
