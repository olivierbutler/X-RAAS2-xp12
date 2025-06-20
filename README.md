# X-RAAS
This is a simulation of the Honeywell Runway Awareness and Advisory System
(RAAS) for the X-Plane 11+ simulator by Laminar Research. To get the latest
release, see:

* https://github.com/olivierbutler/X-RAAS2-xp12/releases

The installation zip package includes a full user manual with
installation instructions in Documentation/User Manual.pdf. Refer to
COPYING for the license.

### BUILDING

To build X-RAAS, check to see you have the pre-requisites installed. The
Linux and Windows versions are built in one step on an Ubuntu 16.04 (or
compatible) machine and the Mac version is obviously built on macOS (10.9
or later).

>Note: __on macOS only__ , by using the option ```-f```, the script will build also the linux and windows versions. see ```README-docker.md```.  

For the Linux and Mac build pre-requisites, see ```build_xpl.sh```

The global build script is located here and is called '```build_release```'.
Once you have the pre-requisite build packages installed, simply run:
***
```
$ ./build_release [-f]
```
This builds the dependencies (FreeType) and then proceeds to build X-RAAS
for the appropriate target platforms. Please note that this builds a
stand-alone version of the plugin that is to be installed into the global
Resources/plugins directory in X-Plane. If you wish to embed this build
into an aircraft model, run it with option ```-e```:
***
```
$ ./build_release [-f] -e
```
See Documentation/Avionics Integration Guide.pdf for more information on
how to properly interface with an embedded version of X-RAAS.
***
```
$ ./build_xpl_sh [-f] [-e]
```
This build only the .xpl file. (options described above can be used)
***
```
$ ./install_xplane.sh
```
Copy the .xpl files to the x-plane and change the quarantine attribute of the ```mac.xpl``` file.  
In the script, just set ```XPLANE_PLUGIN_DIR``` accordingly. 
***

### CREDIT

Original version by skiselkov: https://github.com/skiselkov/X-RAAS2

### DISCLAIMER

X-RAAS is *NOT* meant for flight training or use in real avionics. Its
performance can seriously deviate from the real world system, so *DO NOT*
rely on it for anything critical. It was created solely for entertainment
use. This project has *no* ties to Honeywell or Laminar Research.
