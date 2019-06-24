# EvntConsistColl


## Introduction

EvntConsistColl library is an extention to GASPI that provides some implementations of 
collectives, including eventually consistent ones, in addition to the GASPI native
collectives like barrier and allreduce. 

The main feature of the eventually consistent collectives is that they operate on a 
fraction of data (e.g. 60%) which makes them appealing for machine/ deep learning
applications. 

Currently, we provide early prototypes of broadcast and reduce.

## Installation

#### Requirements:
- `cmake` version > 3.6 (presently build using `cmake v3.9.3`) 
- `c++ 14` (presently with `gcc-8.2.0`)

#### Building EvntlConsistColl

1. clone the git-repository into `<evnt-consist-coll_root>`

2. edit appropriatelly` <evnt-consist-coll_root>/CMakeFiles.txt` to set the following variables
    - if GPI-2 is not to be loaded as a module, redefine `PKG_CONFIG_PATH` by 
    adding the path to the file `GPI2.pc` (the package-config file for GPI-2)
    - eventually, comment the line with `CMAKE_SHARED_LINKER_FLAGS ...`
    (it has been added due to the relative old g++ system-libraries)

3. in `<evnt-consist-coll_root>` create a subdirectory `build` to compile comprex
    - `cd  <evnt-consist-coll_root>`
    - `mkdir build`
    - `cd build`
    - `cmake .. -DCMAKE_INSTALL_DIR=<target_installation_dir>`
    - `make install`

After building and installing EvntConsistColl, 
- the library `libEvntConsistColl.a` is installed in `<target_installation_dir>/lib`
- the header `evnt.consist.coll.hxx` is in `<target_installation_dir>/include`
- the executable examples are in `<evnt-consist-coll_root>/build/examples`

## Examples
There are two examples
- `bcast` provides two versions of broadcast with plain gaspi_write and binomial tree. Both support regular
as well as eventually consistent collectives
- `reduce` provides implementation based on binomial tree that supports both regular and eventually consistent collectives
