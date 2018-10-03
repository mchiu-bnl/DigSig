AC_INIT([V1742PRDF], [1.0])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

AM_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CXX([CC g++])

if test $ac_cv_prog_gcc = yes; then
dnl  CXXFLAGS="$CXXFLAGS -Wall -std=c++11"
  CXXFLAGS="$CXXFLAGS -Wall"
fi

ROOTLIBS=`root-config --libs`
AC_SUBST(ROOTLIBS)

ROOTGLIBS=`root-config --glibs`
AC_SUBST(ROOTGLIBS)

AC_OUTPUT(Makefile)

