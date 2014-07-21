NULL=
AM_CPPFLAGS=\
	$(PACKAGE_CFLAGS) \
	-g \
	$(NULL)
			

bin_PROGRAMS = camorama

BUILT_SOURCES    = camorama-stock-images.h
camorama_SOURCES = \
	callbacks.c \
	camorama-filter-chain.c \
	camorama-filter-chain.h \
	camorama-filter-laplace.c \
	camorama-filter-mirror.c \
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
camorama_LDADD = $(PACKAGE_LIBS)

DISTCLEANFILES=$(BUILT_SOURCES)

camorama-stock-images.h:
	gdk-pixbuf-csource --raw --build-list \
		camorama_webcam_16 ../pixmaps/camorama-webcam-16.png \
		camorama_webcam_24 ../pixmaps/camorama-webcam-24.png \
	> $@ || (rm -f $@)

