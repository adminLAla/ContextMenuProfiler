@echo off
cd /d "%~dp0.."

echo Building ContextMenuProfiler.Hook (x64) - Modularized...
set "BUILD_DIR=.build\hook"
set "OBJ_DIR=%BUILD_DIR%\obj"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

:: Check if cl.exe is already in the path and configured for x64
:: We use 'where' which is more stable in both CMD and PowerShell
where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    cl 2>&1 | findstr /i "x64" >nul
    if %ERRORLEVEL% EQU 0 (
        echo Environment already configured for x64, skipping vcvars64.bat
        goto :compile
    )
)

:: Try to find VS path using vswhere
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do set "VS_PATH=%%i"

if not exist "%VS_PATH%" (
    :: Fallback to common paths
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
)

if not exist "%VS_PATH%" (
    echo Error: Visual Studio not found. Please install VS 2022 or higher, or edit build_hook.bat to set VS_PATH.
    exit /b 1
)

echo Using VS Path: %VS_PATH%
if exist "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo Error: vcvars64.bat not found in %VS_PATH%
    exit /b 1
)

:compile
:: Build Hook DLL
cl /LD /MT /Zi /EHsc /utf-8 /Fo"%OBJ_DIR%\\" /Fd"%BUILD_DIR%\\vc140.pdb" ^
    ContextMenuProfiler.Hook\src\dllmain.cpp ^
    ContextMenuProfiler.Hook\src\common.cpp ^
    ContextMenuProfiler.Hook\src\ipc_server.cpp ^
    ContextMenuProfiler.Hook\src\handlers\com_handler.cpp ^
    ContextMenuProfiler.Hook\src\handlers\ecmd_handler.cpp ^
    ContextMenuProfiler.Hook\src\hook.c ^
    ContextMenuProfiler.Hook\src\trampoline.c ^
    ContextMenuProfiler.Hook\src\buffer.c ^
    ContextMenuProfiler.Hook\src\hde\hde32.c ^
    ContextMenuProfiler.Hook\src\hde\hde64.c ^
    /I ContextMenuProfiler.Hook\include ^
    /link /DLL /DEBUG /OUT:"%BUILD_DIR%\\ContextMenuProfiler.Hook.dll" /PDB:"%BUILD_DIR%\\ContextMenuProfiler.Hook.pdb" /IMPLIB:"%BUILD_DIR%\\ContextMenuProfiler.Hook.lib" user32.lib ole32.lib shell32.lib shlwapi.lib advapi32.lib gdiplus.lib comctl32.lib gdi32.lib
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo Building Injector...
cl /MT /Zi /EHsc /utf-8 /Fo"%OBJ_DIR%\\" /Fd"%BUILD_DIR%\\vc140.pdb" ^
    ContextMenuProfiler.Hook\src\injector.cpp ^
    /I ContextMenuProfiler.Hook\include ^
    /link /DEBUG /OUT:"%BUILD_DIR%\\ContextMenuProfiler.Injector.exe" /PDB:"%BUILD_DIR%\\ContextMenuProfiler.Injector.pdb" user32.lib kernel32.lib advapi32.lib
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

copy /Y "%BUILD_DIR%\ContextMenuProfiler.Hook.dll" "ContextMenuProfiler.Hook.dll" >nul
copy /Y "%BUILD_DIR%\ContextMenuProfiler.Injector.exe" "ContextMenuProfiler.Injector.exe" >nul
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Done.
exit /b 0
