## Development System Setup in Windows
Refer to the following steps in order to setup development environment under Windows to build libuca and uca plugins such as uca-net or uca-pco-win

* Get msys2 from [msys2.github.io](https://msys2.github.io/) (Preferably x86_64 variant)
* Install it to C:\msys64 or any other location
* Fireup mingw64_shell.bat from C:\msys64 folder. This opens up msys2 shell. Use pre installed pacman - package manager to install required packages

* Update core msys packages and restart shell after this command
> `pacman -Syu`

* Install dependent tools and packages `pacman -S <package_name>`. Following is a list of package names
> * `mingw-w64-x86_64-gcc`
> * `mingw-w64-x86_64-gdb`
> * `make`
> * `mingw-w64-x86_64-pkg-config`
> * `perl`
> * `mingw-w64-x86_64-gtk2`

## Building libuca on Windows

* Create an empty `build` directory in libuca root folder and change directory to that folder

* Configure CMAKE using the following command
> `cmake -G "MSYS Makefiles" -D CMAKE_INSTALL_PREFIX="C:\uca" -D CMAKE_BUILD_TYPE=Debug ..`

* make && make install
> This should create and install all binaries and libraries in folder C:\uca

## Building Plugins for libuca on Windows

Note: A package config file(libuca.pc) is generated when libuca is built and is stored in folder pkgconfig in C:\uca\bin. This file is used while building uca plugins to locate shared libraries of libuca.


* Add libuca to Package Config Search Path using environment variable PKG_CONFIG_PATH
> `PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/c/uca/bin/pkgconfig/`

* Create an empty `build` directory in the plugin root folder and change directory to that folder

* Configure CMAKE using the following command
> `cmake -G "MSYS Makefiles" -D CMAKE_INSTALL_PREFIX="C:\uca" -D CMAKE_BUILD_TYPE=Debug ..`

* make && make install

## Executing

`SC2_Cam.dll`, `sc2_cl_me4.dll` (from pco.sdk/bin64) and `fglib5.dll` (from SiliconSoftware Runtime 5.2.3) should be copied into the same directory as the plugin directory (which would be C:\uca\bin i.e `CMAKE_INSTALL_PREFIX`\bin)