This code converts PRDF files to the root format for DigSig.  You'll need
to be in the sPHENIX environment, or have Martin Purschke's PHENIX Event/Packet library with
the the decoder for your packet hit format installed, as well as Martin's pmonitor package.

We use the standard PHENIX and sPHENIX automake system for compilation.
You can find more details on this at the PHENIX or sPHENIX wiki.

To compile and use, first set up your environment.  Type something like

  export MYINSTALL=$HOME/install
  export LD_LIBRARY_PATH=${MYINSTALL}/lib:${LD_LIBRARY_PATH}

MYINSTALL just sets the installation directory and you can set it to whatever you want.
LD_LIBRARY_PATH is the standard loader path for finding libraries to load. Of course, you
only need to set these variables once per shell session.

Then, to create the Makefile, compile, and install the compiled code, do

  cd <SOURCE_DIR>
  mkdir BUILD
  cd BUILD

  ./autogen.sh --prefix=$MYINSTALL
  make install

You should replace <SOURCE_DIR> with your source code directory.

The output will be placed in the $MYINSTALL directory, either under the lib or bin
subdirectories there. 


To use this code to convert a PRDF File, you can follow the example of the prdf2root.sh script in the macros
directory. 

