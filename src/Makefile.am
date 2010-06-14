#
# Makefile.am
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.

bin_PROGRAMS = rygel-grilo test-client

rygel_grilo_SOURCES =	\
	rygel-grilo.c


rygel_grilo_CFLAGS =			\
	$(DEPS_CFLAGS)			\
	$(GRL_DEP_CFLAGS)		\
	-DPREFIX=$(prefix)		\
	-DSYSCONFDIR=\"$(sysconfdir)\"	\
        -I$(top_srcdir)/lib

rygel_grilo_LDADD =	\
	-ldl -lpthread	\
	$(DEPS_LIBS)	\
	$(GRL_DEP_LIBS)	\
	$(top_builddir)/lib/libmediaserver2.la

test_client_SOURCES =	\
	test-client.c

test_client_CFLAGS =		\
	$(DEPS_CFLAGS)		\
	-DPREFIX=$(prefix)	\
	-I$(top_srcdir)/lib

test_client_LDADD =	\
	-ldl -lpthread	\
	$(DEPS_LIBS)	\
	$(top_builddir)/lib/libmediaserver2.la

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
