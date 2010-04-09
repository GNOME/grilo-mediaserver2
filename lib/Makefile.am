#
# Makefile.am
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.

INCLUDES = @DEPS_CFLAGS@

%-server-glue.h: $(top_srcdir)/data/%.xml
	$(AM_V_GEN) $(DBUSBINDINGTOOL)	\
	--mode=glib-server		\
	--output=$@			\
	--prefix=$(subst -,_,$*)_server	\
	$^

%-client-glue.h: $(top_srcdir)/data/%.xml
	$(AM_V_GEN) $(DBUSBINDINGTOOL)	\
	--mode=glib-client		\
	--output=$@			\
	--prefix=$(subst -,_,$*)_client	\
	$^

lib_LTLIBRARIES = libmediaserver2.la

libmediaserver2_la_CFLAGS =	\
	$(DEPS_CFLAGS)

libmediaserver2_la_LIBADD =	\
	$(DEPS_LIBS)

libmediaserver2_la_SOURCES =		\
	media-server2-common.h		\
	media-server2-server-glue.h	\
	media-server2-server-private.h	\
	media-server2-server.h		\
	media-server2-client-glue.h	\
	media-server2.c

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
