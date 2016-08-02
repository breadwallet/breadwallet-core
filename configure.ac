dnl * Copyright 2012-2016 Luke Dashjr
dnl *
dnl * This program is free software; you can redistribute it and/or modify it
dnl * under the terms of the standard MIT license.  See COPYING for more details.

AC_INIT(
	[libbreadwallet-core],
	[0.1.0],
	[https://github.com/breadwallet/breadwallet-core/issues],
	[libbreadwallet-core])
AC_CONFIG_AUX_DIR([.])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.11 -Wall dist-xz foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC_C99
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT([disable-static])

# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
AC_SUBST([LIBBREADWALLET_CORE_SO_VERSION], [0:0:0])

AC_CONFIG_FILES([Makefile
	libbreadwallet-core.pc:libbreadwallet-core.pc.in
])

m4_define([BFG_PTHREAD_FLAG_CHECK],
	AC_MSG_CHECKING([for $1])
    save_CFLAGS="${CFLAGS}"
    save_LIBS="${LIBS}"
    found_pthread=false
	for cflag in ' -pthread' ''; do
		for lib in ' -lpthread' ' -lwinpthread' ''; do
			CFLAGS="${save_CFLAGS}${cflag}"
			LIBS="${save_LIBS}${lib}"
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([
					#include <pthread.h>
				], [
					void *f = $1;
				])
			], [
				found_pthread=true
				PTHREAD_FLAGS="${cflag}"
				PTHREAD_LIBS="${lib}"
				if test "x${cflag}${lib}" = "x"; then
					AC_MSG_RESULT([yes])
				else
					AC_MSG_RESULT([with${cflag}${lib}])
				fi
				$2
				break 2
			])
		done
	done
    CFLAGS="${save_CFLAGS}"
    LIBS="${save_LIBS}"
	if test "x${found_pthread}" = "xfalse"; then
		AC_MSG_RESULT([no])
	fi
	AC_SUBST([PTHREAD_FLAGS])
	AC_SUBST([PTHREAD_LIBS])
)
BFG_PTHREAD_FLAG_CHECK([pthread_create])

AC_SEARCH_LIBS([log],[m])

PKG_CHECK_MODULES([libsecp256k1],[libsecp256k1])

AC_OUTPUT
