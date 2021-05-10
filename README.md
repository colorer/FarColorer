FarColorer
==========
FarColorer is a syntax highlighting plugin for Far Manager.
[![build](https://github.com/colorer/FarColorer/workflows/build/badge.svg)](https://github.com/colorer/FarColorer/actions?query=workflow%3A%22build%22)

It only works for (F4)Edit file. (F3)View is not supported, as there is no API.
But you can type in the console `clr:name_of_file`. This launches the built-in viewer colorer.

Downloads
=========
FarColorer is included in Far Manager since 2013 year (about Far build 3.0.3200). So just update [Far Manager](http://www.farmanager.com/download.php?l=en) to get latest stable release of plugin.

Other version you must find in [Releases](https://github.com/colorer/FarColorer/releases) on github. New release is auto-created after create tag in git. 

FarColorer has two main verions:
 * 1.3.x - version with Windows XP support. Latest version included in Far Manager 1.3.26 in Far Manager 3.0.5796 (2021.05.09). This version has partial support. See in branch v1.3.x.
 * 1.4.x and higher - current fully supported version, worked only on actual platforms. It included in Far Manager since 3.0.5797 (2021.05.10).

For checking installed version press F9 - Options - Plugins configuration, select FarColorer and press F3.

How to build from source
==========
To build plugin from source, you will need:

  * Visual Studio 2019 or higher
  * git
  * cmake 3.15 or higher

Download the source from git repository:

    git clone https://github.com/colorer/FarColorer.git --recursive

Setup vcpkg

    cd FarColorer
    ./external/vcpkg/bootstrap-vcpkg.bat

Build colorer and dependency, if they are not in the local cache:

    mkdir build
    cd build
    cmake -S .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=../external/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_OVERLAY_PORTS=../external/vcpkg-ports -DVCPKG_FEATURE_FLAGS=versions -DCOLORER_BUILD_ARCH=x64
    farcolorer.sln

For x86 platform use `-DVCPKG_TARGET_TRIPLET=x86-windows-static` and `-DCOLORER_BUILD_ARCH=x86`.
Once builded, the dependencies will be cached in the local cache.

Links
========================

* Project main page: [http://colorer.sourceforge.net/](http://colorer.sourceforge.net/)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* FarColorer discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=1573](http://forum.farmanager.com/viewtopic.php?f=5&t=1573)
* FarColorer discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=4319](http://forum.farmanager.com/viewtopic.php?f=39&t=4319)
