#
# Makefile.am
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2010, 2011 Igalia S.L.

INCLUDES = @DEPS_CFLAGS@

grilo-ms-%-glue.h: $(top_srcdir)/data/%1.xml
	$(DBUSBINDINGTOOL)			\
	--mode=glib-server			\
	--output=$@				\
	--prefix=grilo_ms_$(subst -,_,$*)	\
	$^

bin_PROGRAMS = grilo-ms

grilo_ms_SOURCES =				\
	grilo-ms.c				\
	grilo-ms-media-object-glue.h		\
	grilo-ms-media-object.h			\
	grilo-ms-media-object.c			\
	grilo-ms-media-container-glue.h		\
	grilo-ms-media-container.h		\
	grilo-ms-media-container.c		\
	grilo-ms-media-item-glue.h		\
	grilo-ms-media-item.h			\
	grilo-ms-media-item.c

grilo_ms_CFLAGS =	\
	-DPREFIX=$(prefix)

grilo_ms_LDADD =	\
	-ldl -lpthread	\
	@DEPS_LIBS@

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
