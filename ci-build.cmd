@echo off
setlocal
cd /d "%~dp0"

where cl.exe >nul 2>nul
if errorlevel 1 (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found.
    exit /b 1
  )
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
  if not defined VSINSTALL (
    echo ERROR: Visual C++ Build Tools not found.
    exit /b 1
  )
  call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
)

call build.cmd
