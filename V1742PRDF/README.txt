This code converts v1742 pDAQ output to the root format for DigSig.  You'll need
to be in the sPHENIX environment since this code relies on pmonitor.

To compile, type something like

  export MYINSTALL=$HOME/install
  ./autogen.sh --prefix=$MYINSTALL
  make install

MYINSTALL just sets the installation directory and you can set it to whatever you want.
The output will be a libv1742prdf.so in the $MYINSTALL/lib directory. You'll need to
set your LD_LIBRARY_PATH to include $MYINSTALL/lib.

