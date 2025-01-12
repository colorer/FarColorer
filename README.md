FarColorer
==========
FarColorer is a syntax highlighting plugin for Far Manager.
[![Build](https://github.com/colorer/FarColorer/actions/workflows/farcolorer-ci.yml/badge.svg)](https://github.com/colorer/FarColorer/actions/workflows/farcolorer-ci.yml)

It only works for (F4)Edit file. (F3)View is not supported, as there is no API.
But you can type in the console `clr:name_of_file`. This launches the built-in viewer colorer.

Downloads
=========
FarColorer is included in Far Manager since 2013 year (about Far build 3.0.3200). So just update [Far Manager](http://www.farmanager.com/download.php?l=en) to get latest stable release of plugin.

Other version you must find in [Releases](https://github.com/colorer/FarColorer/releases) on github. New release is auto-created after create tag in git. 

FarColorer has two main versions - legacy and modern. On [release page](https://github.com/colorer/FarColorer/releases) files of the modern version has suffix `icu`.
For example, FarColorer.x64.icu.v1.6.1.7z - modern , FarColorer.x64.v1.6.1.7z - legacy.

### Legacy version

 * Included in the Far Manager distribution
 * Support Windows XP (broken between 1.4.x and 1.5.x)
 * Used legacy Unicode string which the library was originally built on
 * Used ghc::filesystem instead of std::filesystem

### Modern version

 * Installed manually
 * Do not support older platform, like as Windows XP
 * Used ICU Unicode string
 * Used std::filesystem

For checking installed version press F9 - Options - Plugins configuration, select FarColorer and press F3.

How to build from source
==========
To build plugin from source, you will need:

  * Visual Studio 2019 or higher
  * git
  * cmake 3.15 or higher
  * vcpkg

Download the source from git repository:

```bash
git clone https://github.com/colorer/FarColorer.git --recursive
```

Setup vcpkg, or use the preset (set env VCPKG_ROOT)

```bash
git clone https://github.com/microsoft/vcpkg.git
set VCPKG_ROOT=<path_to_vcpkg>
%VCPKG_ROOT%\bootstrap-vcpkg.bat -disableMetrics
```

#### IDE

Open folder with Colorer-library from IDE like as Clion, VSCode, VisualStudio.
In the CMakePresets file.json configurations have been created for standard builds.

#### Console

Setup Visual Studio environment (x64 or x86/arm64 for platforms)

```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
```

Build colorer and dependency, if they are not in the local cache:

```bash

mkdir build
cd build
cmake -S .. -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCOLORER_BUILD_ARCH=x64
cmake --build .
```

For x86 platform use `-DVCPKG_TARGET_TRIPLET=x86-windows-static -DCOLORER_BUILD_ARCH=x86`, arm64 - `-DVCPKG_TARGET_TRIPLET=arm64-windows-static -DCOLORER_BUILD_ARCH=arm64`.

Once built, the dependencies will be cached in the local cache.

Links
========================

* Project main page: [http://colorer.sourceforge.net/](http://colorer.sourceforge.net/)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* FarColorer discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=1573](http://forum.farmanager.com/viewtopic.php?f=5&t=1573)
* FarColorer discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=4319](http://forum.farmanager.com/viewtopic.php?f=39&t=4319)
