#
# Makefile.am
#
# Author: Iago Toral <itoral@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.

INCLUDES = @DEPS_CFLAGS@

dgrilo-%-glue.h: $(top_srcdir)/data/%.xml
	$(DBUSBINDINGTOOL)		\
	--mode=glib-server		\
	--output=$@			\
	--prefix=dgrilo_$(subst -,_,$*)	\
	$^

bin_PROGRAMS = test

test_SOURCES =				\
	main.c				\
	dgrilo-media-object-glue.h	\
	dgrilo-media-object.h		\
	dgrilo-media-object.c		\
	dgrilo-media-container-glue.h	\
	dgrilo-media-container.h	\
	dgrilo-media-container.c	\
	dgrilo-media-item-glue.h	\
	dgrilo-media-item.h		\
	dgrilo-media-item.c

test_CFLAGS =	\
	-DPREFIX=$(prefix)

test_LDADD =		\
	-ldl -lpthread	\
	@DEPS_LIBS@

MAINTAINERCLEANFILES =	\
	*.in

DISTCLEANFILES = $(MAINTAINERCLEANFILES)
