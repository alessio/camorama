NULL=
AM_CPPFLAGS=\
	$(PACKAGE_CFLAGS) \
	-g \
	$(NULL)
			

bin_PROGRAMS = camorama

camorama_SOURCES = \
	callbacks.c \
	camorama-filter-chain.c \
	camorama-filter-chain.h \
	camorama-filter-laplace.c \
	camorama-filter-mirror.c \
	camorama-filter-reichardt.c \
	camorama-globals.h \
	camorama-stock-items.c \
	camorama-stock-items.h \
	camorama-window.c \
	camorama-window.h \
	filter.c \
	glib-helpers.h \
	main.c \
	support.c \
	v4l.c \
	fileio.c	\
	v4l.h		\
	callbacks.h	\
	support.h	\
	interface.h	\
	fileio.h	\
	font_6x11.h	\
	filter.h	\
	$(BUILT_SOURCES)\
	$(NULL)
camorama_LDADD = $(PACKAGE_LIBS) -lv4l2 -lm

DISTCLEANFILES=$(BUILT_SOURCES)

