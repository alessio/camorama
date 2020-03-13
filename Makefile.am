desktopdir = $(datadir)/applications
desktop_in_files = camorama.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

man_MANS = camorama.1

appdatadir = $(datadir)/metainfo
appdata_in_files = camorama.appdata.xml.in
nodist_appdata_DATA = $(appdata_in_files:.xml.in=.xml)

SUBDIRS = src data po

EXTRA_DIST =					\
	autogen.sh				\
	camorama.1				\
	camorama.desktop.in			\
	camorama.appdata.xml.in

CLEANFILES=\
	camorama.desktop     \
	camorama.appdata.xml \
	po/camorama.pot

pixmapdir   = $(datadir)/pixmaps

install-icons:
	for i in 16 24 32 48 64 128 256 512; do \
		$(mkinstalldirs) $(DESTDIR)$(datadir)/icons/hicolor/$${i}x$${i}/devices; \
		$(INSTALL_DATA) pixmaps/camorama-$${i}.png $(DESTDIR)$(datadir)/icons/hicolor/$${i}x$${i}/devices/camorama.png; \
	done

install-data-local: install-icons
@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pixmapdir); \
	$(mkinstalldirs) $(DESTDIR)$(datadir)/camorama/pixmaps; \
	for pixmap in $(srcdir)/pixmaps/*.png; do \
		if test -f $$pixmap; then \
			$(INSTALL_DATA) $$pixmap $(DESTDIR)$(pixmapdir); \
			$(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/camorama/pixmaps; \
		fi \
	done

dist-hook:
	if test -d pixmaps; then \
	mkdir $(distdir)/pixmaps; \
	for pixmap in pixmaps/*; do \
	if test -f $$pixmap; then \
	cp -p $$pixmap $(distdir)/pixmaps; \
	fi \
	done \
fi

camorama.desktop: camorama.desktop.in
	msgfmt --desktop -d $(top_srcdir)/po --template $< -o $@

camorama.appdata.xml: camorama.appdata.xml.in
	msgfmt --xml -d $(top_srcdir)/po --template $< -o $@

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES = config.guess config.rpath~ config.sub configure \
		 ./Makefile.in ./data/Makefile.in ./src/Makefile.in \
		 ./po/Makefile.in ./po/Makefile.in
