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
	camorama.appdata.xml.in			\
	data/pixmaps/camorama-webcam-16.png	\
	data/pixmaps/camorama-webcam-24.png


CLEANFILES=\
	camorama.desktop     \
	camorama.appdata.xml \
	po/camorama.pot

pixmapdir   = $(datadir)/pixmaps

install-data-local:
@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	$(mkinstalldirs) $(DESTDIR)$(pixmapdir); \
	$(mkinstalldirs) $(DESTDIR)$(datadir)/camorama/pixmaps; \
	for pixmap in $(srcdir)/pixmaps/*.png; do \
		if test -f $$pixmap; then \
			$(INSTALL_DATA) $$pixmap $(DESTDIR)$(pixmapdir); \
			$(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/camorama/pixmaps; \
		fi \
	done \
fi

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
