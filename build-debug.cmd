@echo off
setlocal
cd /d "%~dp0"

where cl.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: cl.exe not found.
  exit /b 1
)

if not exist build-debug mkdir build-debug
cl.exe /nologo /std:c++17 /EHsc /Od /Zi /MD /DFH3ARCFIX_DIAGNOSTICS=1 /LD src\fh3arcfix.cpp /link /DEBUG /DEF:src\d3d12.def /OUT:build-debug\d3d12.dll
if errorlevel 1 exit /b 1

echo.
echo Built diagnostics DLL: %CD%\build-debug\d3d12.dll
endlocal
