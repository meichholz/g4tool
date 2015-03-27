AC_PREREQ([2.69])
AC_INIT([g4tool], [1.0], [eichholz@computer.org])
AC_CONFIG_SRCDIR([src/global.h])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.9.6])
LT_INIT

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

##CHECK: Is this the way to select all the flags?
if test "$GCC" = yes
then
  CFLAGS="${CFLAGS} ${EXTRA_GCFLAGS}-pipe -Wno-unused -Wbad-function-cast -Wmissing-prototypes -Wstrict-prototypes -Wcast-align -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -Wundef -pedantic -fno-common -std=gnu99"
  # -Wshadow is TODO
  LDFLAGS="${LDFLAGS} ${EXTRA_GLDFLAGS} -g"
else
  CFLAGS="${CFLAGS} ${EXTRA_CFLAGS}"
  LDFLAGS="${LDFLAGS} ${EXTRA_LDFLAGS}"
fi

AC_HEADER_STDC
AH_TEMPLATE([HAVE_LIBTIFF],[Is libtiff available?])
AC_CHECK_HEADER([tiffio.h],[],[AC_MSG_ERROR([tiffio.h not found])])
AC_CHECK_LIB([tiff],
			 [TIFFOpen],
			 [
AC_DEFINE([HAVE_LIBTIFF],[1])
TIFF_LIB="-ltiff"
],[
AC_MSG_ERROR([You need libtiff])])
AC_SUBST([TIFF_LIB])

# Output

AC_CONFIG_FILES([Makefile
                 src/Makefile
				 man/Makefile
				 test/Makefile
				 ])
AC_OUTPUT

