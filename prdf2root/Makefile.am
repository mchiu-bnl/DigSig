AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4 

AM_CXXFLAGS = -g -Wall -Werror -Wextra -Wno-unused-parameter

AM_CPPFLAGS = -I$(includedir) -I$(ONLINE_MAIN)/include -I$(ROOTSYS)/include 

lib_LTLIBRARIES = libdigsig_prdf.la

AM_LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib -lpmonitor -lEvent -lNoRootEvent -lmessage

libdigsig_prdf_la_LIBADD = @ROOTLIBS@ @ROOTGLIBS@

include_HEADERS = \
 digsig_prdf.h 

libdigsig_prdf_la_SOURCES = \
  digsig_prdfDict.C \
  digsig_prdf.cc

digsig_prdfDict.C: digsig_prdf.h digsig_prdfLinkDef.h
	rootcint -f $@ -c $(AM_CPPFLAGS) $^

clean-local:
	rm -f $(BUILT_SOURCES) *Dict*

