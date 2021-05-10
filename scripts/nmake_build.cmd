rem Example script for build FarColorer
@echo off
set PROJECT_ROOT=%~dp0..
set PROJECT_CONFIG=Release

set PROJECT_ARCH=x64

:build
set PROJECT_BUIILDDIR=%PROJECT_ROOT%\build\%PROJECT_CONFIG%\%PROJECT_ARCH%
set INSTALL_DIR=%PROJECT_ROOT%\build\FarColorer\%PROJECT_ARCH%

if exist %PROJECT_BUIILDDIR% rmdir /S /Q %PROJECT_BUIILDDIR%
if exist %INSTALL_DIR% rmdir /S /Q %INSTALL_DIR%
mkdir %PROJECT_BUIILDDIR% > NUL
mkdir %INSTALL_DIR% > NUL

pushd %PROJECT_BUIILDDIR%
@call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" %PROJECT_ARCH%
cmake.exe %PROJECT_ROOT% -G "NMake Makefiles" -DCOLORER_BUILD_ARCH=%PROJECT_ARCH% -DCMAKE_BUILD_TYPE=%PROJECT_CONFIG% -DCMAKE_TOOLCHAIN_FILE=%PROJECT_ROOT%/external/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%PROJECT_ARCH%-windows-static -DVCPKG_OVERLAY_PORTS=%PROJECT_ROOT%/external/vcpkg-ports -DVCPKG_FEATURE_FLAGS=versions -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build . --config %PROJECT_CONFIG%
cmake --install . --config %PROJECT_CONFIG%
echo See result in directory %INSTALL_DIR%
popd

if "%PROJECT_ARCH%" == "x64" goto x86
goto exit

:x86
set PROJECT_ARCH=x86
goto build

:exit
