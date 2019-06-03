# ConsistColl


1. INTRODUCTION
===============

ConsistColl library provides some implementations of eventually consistent collectives such as reduce.

Eventually consistent collectives operate with a fration of data (e.g. 50%). They are based 
on GASPI and extend its API to cover more than the classic allreduce.


2. INSTALLATION
===============

Requirements:
-------------
 (i)  cmake-version > 3.6 ( presently build using 3.9.3) 
 (ii) c++ 14 (presently with gcc-5.2.0 on seislab)

Building ConsistColl
----------------

2.1. clone the git-repository into <consistcoll_root>

2.2. edit appropriatelly <consistcoll_root>/CMakeFiles.txt to set there the variables
 -> if GPI-2 is not to be loaded as a module, redefine PKG_CONFIG_PATH by 
    adding the path to the file GPI2.pc (the package-config file for GPI-2)
 -> eventually, comment the line set (CMAKE_SHARED_LINKER_FLAGS ..)
    ( it has been added due to the relative old g++ system-libraries )

2.3. in <consistcoll_root> create a subdirectory "build" to compile comprex

  $ cd  <consistcoll_root>
  $ mkdir build
  $ cd build
  $ cmake .. -DCMAKE_INSTALL_DIR=<target_installation_dir>
  $ make install

After building and installing comprex, 

-> the library "libConsistColl.a" is installed in <target_installation_dir>/lib
-> the header "consistent.collectives.hxx" is in <target_installation_dir>/include
-> the executable "example" is in <consistcoll_root>/build/examples


