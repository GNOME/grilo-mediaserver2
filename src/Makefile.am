#
# Makefile.am
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.

INCLUDES = @DEPS_CFLAGS@

rygel-grilo-%-glue.h: $(top_srcdir)/data/%2.xml
	$(DBUSBINDINGTOOL)			\
	--mode=glib-server			\
	--output=$@				\
	--prefix=rygel_grilo_$(subst -,_,$*)	\
	$^

bin_PROGRAMS = rygel-grilo

rygel_grilo_SOURCES =			\
	rygel-grilo.c			\
	rygel-grilo-media-server-glue.h	\
	rygel-grilo-media-server.h	\
	rygel-grilo-media-server.c

rygel_grilo_CFLAGS =	\
	-DPREFIX=$(prefix)

rygel_grilo_LDADD =	\
	-ldl -lpthread	\
	@DEPS_LIBS@

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
