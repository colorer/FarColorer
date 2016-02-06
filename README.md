FarColorer
==========
  FarColorer is a syntax highlighting plugin for FAR Manager.
  
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

  * Visual Studio 2015 or higher
  * git
  * cmake 2.8.9 or higher

Download the source from git repository:

    cd src
    git clone https://github.com/colorer/FarColorer.git --recursive

or update git repository:

    git pull
    git submodule update --recursive
    
From src/FarColorer/scripts call
    
    nmake_build.cmd

The built binaries will be in build/FarColorer/x86 or x64

Links
========================

* Project main page: [http://colorer.sourceforge.net/](http://colorer.sourceforge.net/)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* FarColorer discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=1573](http://forum.farmanager.com/viewtopic.php?f=5&t=1573)
* FarColorer discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=4319](http://forum.farmanager.com/viewtopic.php?f=39&t=4319)
