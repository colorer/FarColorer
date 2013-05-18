@echo off
set PROJECT_ROOT=%~dp0..
set PROJECT_CONFIG=Release

set PROJECT_CONF=x86

:build
set PROJECT_BUIILDDIR=%PROJECT_ROOT%\build\%PROJECT_CONFIG%\%PROJECT_CONF%
if not exist %PROJECT_BUIILDDIR% ( mkdir %PROJECT_BUIILDDIR% > NUL )
pushd %PROJECT_BUIILDDIR%

@call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" %PROJECT_CONF%
cmake.exe -G "NMake Makefiles" -D PROJECT_ROOT=%PROJECT_ROOT% -D CMAKE_BUILD_TYPE=%PROJECT_CONFIG% -D CONF=%PROJECT_CONF% %PROJECT_ROOT%\src
nmake

:: Create temp directory
set PKGDIR=%PROJECT_ROOT%\build\FarColorer
set PKGDIRARCH=%PKGDIR%\%PROJECT_CONF%
set PKGDIRBIN=%PKGDIRARCH%\bin
set SDIR=%PROJECT_ROOT%\build\%PROJECT_CONFIG%\%PROJECT_CONF%
if exist %PKGDIRARCH% rmdir /S /Q %PKGDIRARCH%

:: Copy files
if not exist %PKGDIR% ( mkdir %PKGDIR% > NUL )
mkdir %PKGDIRARCH% > NUL
mkdir %PKGDIRBIN% > NUL

copy %PROJECT_ROOT%\misc\*.* %PKGDIRBIN% > NUL
copy %PROJECT_ROOT%\LICENSE %PKGDIRARCH% > NUL
copy %PROJECT_ROOT%\docs\history.ru.txt %PKGDIRARCH% > NUL

copy %SDIR%\*.dll %PKGDIRBIN% > NUL
copy %SDIR%\*.map %PKGDIRBIN% > NUL

popd

if "%PROJECT_CONF%" == "x86" goto x64
goto exit

:x64
set PROJECT_CONF=x64
goto build

:exit
