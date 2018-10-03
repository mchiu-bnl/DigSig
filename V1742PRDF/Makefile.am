AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4 

AM_CXXFLAGS = -g -Wall -Werror -Wextra -Wno-unused-parameter

AM_CPPFLAGS = -I$(includedir) -I$(ONLINE_MAIN)/include -I$(ROOTSYS)/include 

lib_LTLIBRARIES = libv1742prdf.la   

AM_LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib -lpmonitor -lEvent -lNoRootEvent -lmessage

libv1742prdf_la_LIBADD = @ROOTLIBS@ @ROOTGLIBS@

noinst_HEADERS = v1742prdf.h

libv1742prdf_la_SOURCES = \
        v1742prdfDict.C \
	v1742prdf.cc

v1742prdfDict.C: v1742prdf.h v1742prdfLinkDef.h
	rootcint -f $@ -c $(AM_CPPFLAGS) $^

clean-local:
	rm -f $(BUILT_SOURCES) *Dict*

