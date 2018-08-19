desktopdir = $(datadir)/applications
desktop_in_files = camorama.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

SUBDIRS = src pixmaps data po

EXTRA_DIST =   autogen.sh     \
	camorama.desktop.in  \
	intltool-extract.in     \
	intltool-merge.in    \
	intltool-update.in

CLEANFILES=\
	camorama.desktop     \
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
