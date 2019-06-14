# EvntlConsistColl


1. INTRODUCTION
===============

EvntlConsistColl library is an extention to GASPI that provides some implementations of 
collectives, including eventually consistent ones, in addition to the GASPI native
collectives like barrier and allreduce. 

The main feature of eventually consistent collectives is that they operate on a 
fraction of data (e.g. 60%) which makes them appealing for machine/ deep learning
applications. 

Currently, we provide early prototypes of broadcast and reduce.

2. INSTALLATION
===============

Requirements:
-------------
 (i)  cmake-version > 3.6 ( presently build using 3.9.3) 
 (ii) c++ 14 (presently with gcc-5.2.0 on seislab)

Building EvntlConsistColl
----------------

2.1. clone the git-repository into <evntl-consist-coll_root>

2.2. edit appropriatelly <evntl-consist-coll_root>/CMakeFiles.txt to set there the variables
 -> if GPI-2 is not to be loaded as a module, redefine PKG_CONFIG_PATH by 
    adding the path to the file GPI2.pc (the package-config file for GPI-2)
 -> eventually, comment the line set (CMAKE_SHARED_LINKER_FLAGS ..)
    ( it has been added due to the relative old g++ system-libraries )

2.3. in <evntl-consist-coll_root> create a subdirectory "build" to compile comprex

  $ cd  <evntl-consist-coll_root>
  $ mkdir build
  $ cd build
  $ cmake .. -DCMAKE_INSTALL_DIR=<target_installation_dir>
  $ make install

After building and installing comprex, 

-> the library "libEvntlConsistColl.a" is installed in <target_installation_dir>/lib
-> the header "evntl.consist.coll.hxx" is in <target_installation_dir>/include
-> the executable "example" is in <evntl-consist-coll_root>/build/examples


