FarColorer
==========
FarColorer is a syntax highlighting plugin for FAR Manager.

It only works for (F4)Edit file. (F3)View is not supported, as there is no API.
But you can type in the console `clr:name_of_file`. This launches the built-in viewer colorer.

Downloads
=========
!!! Current branch (v1.3.x) contains FarColorer version, which support Windows XP. !!!

Latest version included in Far Manager is 1.3.26 (FarManager 3.0.5796). Since Far Manager 3.0.5797 it`s include version not support Windows XP.
Latest version of 1.3.x you must find in [Releases](https://github.com/colorer/FarColorer/releases) on github. But this version has partial support.

For checking installed version press F9 - Options - Plugins configuration, select FarColorer and press F3.

  
How to build from source
==========
To build plugin from source, you will need:

  * Visual Studio 2017 or higher
  * git
  * cmake 3.15 or higher

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
