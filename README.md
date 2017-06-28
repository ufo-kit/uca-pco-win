### pco.sdk Windows plugin for libuca.

To build the plugin install libuca according to the Windows installation
[instructions](http://libuca.readthedocs.io/en/latest/quickstart.html#building-on-windows).
Now download and install the pco development SDK for Windows and keep the
directory location at hand (lets assume for now it's installed at `C:\pco.sdk`).
Because we the UNIX Makefiles generated with CMake do not handle Windows paths
very well, we first link that into the Cygwin environment:

    $ ln -s C:/pco.sdk $HOME/pco.sdk

Now we can configure the plugin:

    $ cd uca-pco-win
    $ mkdir build && cd build
    $ cmake .. -DPCOSDK_INCLUDE_DIR=$HOME/pco.sdk/include -DPCOSDK_LIBRARY_DIR=$HOME/pco.sdk/bin64
    $ make && make install
