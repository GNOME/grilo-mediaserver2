#
# Makefile.am
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.

INCLUDES = @DEPS_CFLAGS@

lib_LTLIBRARIES = libmediaserver1.la

libmediaserver1_la_CFLAGS =	\
	$(DEPS_CFLAGS)		\
	-I$(srcdir)

libmediaserver1_la_LIBADD =	\
	$(DEPS_LIBS)

libmediaserver1_la_SOURCES =		\
	media-server1-introspection.h	\
	media-server1-server-table.c	\
	media-server1-server.c		\
	media-server1-client.c		\
	media-server1-observer.c

libmediaserver1inc_HEADERS =	\
	media-server1-common.h	\
	media-server1-server.h	\
	media-server1-client.h	\
	media-server1-observer.h

libmediaserver1incdir =	$(includedir)

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
