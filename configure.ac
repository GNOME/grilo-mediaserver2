#
# configure.ac
# 
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.. All rights reserved.

m4_define([prj_name],    [rygel-grilo])
m4_define([prj_version], [0.1])

AC_INIT([prj_name], [prj_version])

AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_SRCDIR([src])

AM_CONFIG_HEADER(src/config.h)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AM_PROG_CC_C_O

AC_HEADER_STDC

AM_MAINTAINER_MODE

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

CFLAGS="$CFLAGS -Wall -Werror -Wmissing-prototypes -Wmissing-declarations -std=c99"

# ----------------------------------------------------------
# DEBUG SUPPORT
# ----------------------------------------------------------

AC_ARG_ENABLE(debug,
        [  --enable-debug include debug symbols],,
        enable_debug=yes)
if test "x$enable_debug" = "xyes"; then
        AC_MSG_WARN("Configuring with debugging options enabled!")
	CFLAGS="$CFLAGS -g3 -O0"
fi

AM_CONDITIONAL([DEBUG], [test "x$enable_debug" = "xyes"])

# ----------------------------------------------------------
# UNINSTALLED SUPPORT
# ----------------------------------------------------------

AC_ARG_ENABLE(uninstalled,
        AC_HELP_STRING([--enable-uninstalled],
                [run uninstalled (default: no)]),,
        [enable_uninstalled=no])

if test "x$enable_uninstalled" = "xyes"; then
        AC_MSG_WARN("Configuring for running uninstalled!")
        GRL_API_VERSION=uninstalled
else
        GRL_API_VERSION=0.1
fi

# ----------------------------------------------------------
# DEPENDENCIES
# ----------------------------------------------------------

PKG_CHECK_MODULES(DEPS, 		\
			glib-2.0	\
			gobject-2.0	\
                        gio-2.0         \
			dbus-glib-1)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

PKG_CHECK_MODULES(GRL_DEP, grilo-${GRL_API_VERSION} >= 0.1.13)

AC_SUBST(GRL_DEP_CFLAGS)
AC_SUBST(GRL_DEP_LIBS)

# ----------------------------------------------------------
# DBUS BINDING TOOL
# ----------------------------------------------------------

AC_PATH_PROG(DBUSBINDINGTOOL, dbus-binding-tool)
if test -z $DBUSBINDINGTOOL; then
   AC_MSG_ERROR([Could not find 'dbus-binding-tool'])
fi

# ----------------------------------------------------------
# WORKAROUNDS
# ----------------------------------------------------------

AC_SUBST([abs_top_builddir])
AC_SUBST([builddir])
AC_SUBST([sysconfdir])

# ----------------------------------------------------------
# OUTPUT
# ----------------------------------------------------------

AC_CONFIG_FILES([
  Makefile
  mediaserver2.pc
  lib/Makefile
  src/Makefile
  data//Makefile
])

AC_OUTPUT
