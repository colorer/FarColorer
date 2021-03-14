FarColorer
==========
FarColorer is a syntax highlighting plugin for FAR Manager.
[![build](https://github.com/colorer/FarColorer/workflows/build/badge.svg)](https://github.com/colorer/FarColorer/actions?query=workflow%3A%22build%22)

Downloads
=========
To get the latest Colorer, install the latest [FarManager](http://www.farmanager.com/download.php?l=en).
Nightly builds of FarManager have night build of FarColorer.

FarColorer is included in FAR Manager since about build 3200, so just update FAR Manager to get latest releases.
  
Check F9 - Options - Plugins configuration - FarColorer

It only works for (F4)Edit, (F3)View is not supported, as there is no API. 
but you can type in the console `clr:name_of_file` 
This launches the built-in viewer colorer.
  
How to build from source
==========
To build plugin from source, you will need:

  * Visual Studio 2019 or higher
  * git
  * cmake 3.15 or higher

Download the source from git repository:

    git clone https://github.com/colorer/FarColorer.git --recursive

Setup vcpkg

    cd colorer-library
    ./external/vcpkg/bootstrap-vcpkg.bat

Build colorer and dependency, if they are not in the local cache:

    mkdir build
    cd build
    cmake -S .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=../external/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_OVERLAY_PORTS=../external/vcpkg-ports -DVCPKG_FEATURE_FLAGS=versions -DCOLORER_BUILD_ARCH=x64
    colorer.sln

For x86 platform use `-DVCPKG_TARGET_TRIPLET=x86-windows-static` and `-DCOLORER_BUILD_ARCH=x86`.
Once builded, the dependencies will be cached in the local cache.

Links
========================

* Project main page: [http://colorer.sourceforge.net/](http://colorer.sourceforge.net/)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* FarColorer discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=1573](http://forum.farmanager.com/viewtopic.php?f=5&t=1573)
* FarColorer discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=4319](http://forum.farmanager.com/viewtopic.php?f=39&t=4319)
