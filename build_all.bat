@echo off
setlocal

echo eSpeak-NG SAPI5 - Build
echo.

set BUILD_DIR_X86=build_x86
set BUILD_DIR_X64=build_x64
set OUTPUT_DIR=output

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

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
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

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_LINKER=lld-link -DCMAKE_RC_COMPILER=llvm-rc -DCMAKE_RC_FLAGS="/C 65001" -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -S . -B %BUILD_DIR_X86%
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

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_LINKER=lld-link -DCMAKE_RC_COMPILER=llvm-rc -DCMAKE_RC_FLAGS="/C 65001" -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -S . -B %BUILD_DIR_X64%
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

"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" "installer\espeak-sapi.iss" /Q

if errorlevel 1 (
    echo Warning: InnoSetup compilation failed.
    echo Install InnoSetup 6 from https://jrsoftware.org/isinfo.php
    echo Installer not created, but DLLs are available in output directory.
) else (
    echo Installer created successfully in output directory!
)

echo.
echo Build completed successfully!
echo.

endlocal
