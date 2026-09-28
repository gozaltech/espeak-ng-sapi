@echo off
setlocal

echo eSpeak-NG SAPI5 - Build
echo.

set BUILD_DIR_X86=build_x86
set BUILD_DIR_X64=build_x64
set OUTPUT_DIR=output

rem Optional overrides for the espeak-ng sources, e.g. to build a local checkout:
rem   set ESPEAK_NG_GIT_REPOSITORY=C:\path\to\espeak-ng
rem   set ESPEAK_NG_GIT_TAG=my-branch
set ESPEAK_NG_ARGS=
if defined ESPEAK_NG_GIT_REPOSITORY set ESPEAK_NG_ARGS=%ESPEAK_NG_ARGS% "-DESPEAK_NG_GIT_REPOSITORY=%ESPEAK_NG_GIT_REPOSITORY%"
if defined ESPEAK_NG_GIT_TAG set ESPEAK_NG_ARGS=%ESPEAK_NG_ARGS% "-DESPEAK_NG_GIT_TAG=%ESPEAK_NG_GIT_TAG%"

if not exist %BUILD_DIR_X86% mkdir %BUILD_DIR_X86%
if not exist %BUILD_DIR_X64% mkdir %BUILD_DIR_X64%
if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%
if not exist %OUTPUT_DIR%\x86 mkdir %OUTPUT_DIR%\x86
if not exist %OUTPUT_DIR%\x64 mkdir %OUTPUT_DIR%\x64
if not exist %OUTPUT_DIR%\espeak-ng-data mkdir %OUTPUT_DIR%\espeak-ng-data

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Please install Visual Studio 2022 or later.
    exit /b 1
)

rem -products * is needed to also find the standalone Build Tools edition.
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSINSTALLDIR=%%i\"
)

if not defined VSINSTALLDIR (
    echo ERROR: Visual Studio installation not found.
    exit /b 1
)

echo Found Visual Studio at: %VSINSTALLDIR%
echo.

echo Building x86 version...
echo.

call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 (
    echo ERROR: Failed to setup x86 environment.
    exit /b 1
)

call :select_compiler

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release %COMPILER_ARGS% -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded %ESPEAK_NG_ARGS% -S . -B %BUILD_DIR_X86%
if errorlevel 1 (
    echo ERROR: CMake x86 configuration failed.
    exit /b 1
)

cmake --build %BUILD_DIR_X86%
if errorlevel 1 (
    echo ERROR: x86 build failed.
    exit /b 1
)

echo.

call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo ERROR: Failed to setup x64 environment.
    exit /b 1
)

call :select_compiler

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release %COMPILER_ARGS% -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded %ESPEAK_NG_ARGS% -S . -B %BUILD_DIR_X64%
if errorlevel 1 (
    echo ERROR: CMake x64 configuration failed.
    exit /b 1
)

cmake --build %BUILD_DIR_X64%
if errorlevel 1 (
    echo ERROR: x64 build failed.
    exit /b 1
)

echo.
echo Copying results to output directory...
echo.

echo Copying x86 SAPI DLL...
copy /Y "%BUILD_DIR_X86%\bin\EspeakSAPI.dll" "%OUTPUT_DIR%\x86\"

echo Copying x86 Configurator...
copy /Y "%BUILD_DIR_X86%\bin\EspeakSAPIConfig.exe" "%OUTPUT_DIR%\x86\"

echo Copying x86 espeak-ng DLL...
copy /Y "%BUILD_DIR_X86%\bin\espeak-ng.dll" "%OUTPUT_DIR%\x86\"

echo Copying x64 SAPI DLL...
copy /Y "%BUILD_DIR_X64%\bin\EspeakSAPI.dll" "%OUTPUT_DIR%\x64\"

echo Copying x64 Configurator...
copy /Y "%BUILD_DIR_X64%\bin\EspeakSAPIConfig.exe" "%OUTPUT_DIR%\x64\"

echo Copying x64 espeak-ng DLL...
copy /Y "%BUILD_DIR_X64%\bin\espeak-ng.dll" "%OUTPUT_DIR%\x64\"

echo Copying espeak-ng data from x86 build...
if exist "%BUILD_DIR_X86%\_deps\espeak-ng-build\espeak-ng-data" (
    xcopy /E /I /Y "%BUILD_DIR_X86%\_deps\espeak-ng-build\espeak-ng-data" "%OUTPUT_DIR%\espeak-ng-data"
    echo Successfully copied espeak-ng-data from x86 build
) else (
    echo ERROR: espeak-ng-data not found at %BUILD_DIR_X86%\_deps\espeak-ng-build\espeak-ng-data
    echo The x86 build must complete successfully before copying data.
    exit /b 1
)

echo.
echo Building installer with InnoSetup...
echo.

rem Look for the Inno Setup compiler: %ISCC% override, then the per-machine
rem and per-user install locations.
if not defined ISCC (
    for %%p in (
        "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
        "%ProgramFiles%\Inno Setup 6\ISCC.exe"
        "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
    ) do (
        if not defined ISCC if exist %%p set "ISCC=%%~p"
    )
)

if not defined ISCC (
    echo Warning: Inno Setup 6 compiler ^(ISCC.exe^) not found.
    echo Install InnoSetup 6 from https://jrsoftware.org/isinfo.php or set ISCC to its path.
    echo Installer not created, but DLLs are available in output directory.
    goto done
)

"%ISCC%" "installer\espeak-sapi.iss" /Q

if errorlevel 1 (
    echo Warning: InnoSetup compilation failed.
    echo Installer not created, but DLLs are available in output directory.
) else (
    echo Installer created successfully in output directory!
)

:done
echo.
echo Build completed successfully!
echo.

endlocal
exit /b 0

rem Prefer clang-cl when the LLVM toolset is installed, otherwise use MSVC.
:select_compiler
where clang-cl >nul 2>nul
if errorlevel 1 (
    echo Using MSVC compiler
    set COMPILER_ARGS=-DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl "-DCMAKE_RC_FLAGS=/c65001"
) else (
    echo Using clang-cl compiler
    set COMPILER_ARGS=-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_LINKER=lld-link -DCMAKE_RC_COMPILER=llvm-rc "-DCMAKE_RC_FLAGS=/C 65001"
)
exit /b 0
