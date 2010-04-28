#
# Makefile.am
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.

INCLUDES = @DEPS_CFLAGS@

lib_LTLIBRARIES = libmediaserver2.la

libmediaserver2_la_CFLAGS =	\
	$(DEPS_CFLAGS)

libmediaserver2_la_LIBADD =	\
	$(DEPS_LIBS)

libmediaserver2_la_SOURCES =		\
	media-server2-introspection.h	\
	media-server2-common.h		\
	media-server2-server.h		\
	media-server2-server-table.c	\
	media-server2-server.c		\
	media-server2-client.h		\
	media-server2-client.c		\
	media-server2-observer.h	\
	media-server2-observer.c

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
