AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -g -O2 -Wall -Werror -Wextra -Wno-unused-parameter

AM_CPPFLAGS = -I$(includedir) -I$(ROOTSYS)/include 

lib_LTLIBRARIES = libdigsig.la   

AM_LDFLAGS = -L$(libdir)

libdigsig_la_LIBADD = @ROOTLIBS@ @ROOTGLIBS@

noinst_HEADERS = RunningStats.h digsigLinkDef.h

include_HEADERS = DigSig.h \
		  DigAna.h

libdigsig_la_SOURCES = RunningStats.cc \
		       DigSig.cc \
		       DigAna.cc

nodist_libdigsig_la_SOURCES = digsigDict.C

digsigDict.C: DigSig.h DigAna.h digsigLinkDef.h
	rootcint -f $@ -c $(RINCLUDES) $^


clean-local:
	rm -f $(BUILT_SOURCES) *Dict*


