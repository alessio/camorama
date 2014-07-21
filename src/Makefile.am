INCLUDES = \
   -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DDATADIR=\""$(datadir)"\"    \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
		   @PACKAGE_CFLAGS@
			

bin_PROGRAMS = camorama

camorama_SOURCES = main.c v4l.c filter.c callbacks.c support.c fileio.c eggtrayicon.c v4l.h callbacks.h support.h interface.h fileio.h font_6x11.h filter.h eggtrayicon.h
camorama_LDADD = @PACKAGE_LIBS@ 
