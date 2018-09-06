stuffdir = $(datadir)/camorama
stuff_DATA =				\
	pixmaps/snap-icon.xpm \
	pixmaps/snap-icon-small.xpm \
	pixmaps/contrast.png \
	pixmaps/colourness.png \
	pixmaps/brightness.png \
	pixmaps/whiteness.png \
	pixmaps/stock-tool-hue-saturation-16.png

gladedir = $(datadir)/camorama
glade_DATA= $(CAMORAMA_UI)

schemadir =  @GCONF_SCHEMA_FILE_DIR@
schema_in_files = camorama.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

# For the sake of some users
user-install: $(app_DATA) $(keys_DATA)
	mkdir -p ~/.gnome/application-registry ~/.gnome/mime-info/
	cp $(app_DATA) ~/.gnome/application-registry
	cp $(keys_DATA) ~/.gnome/mime-info/

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA)
	$(GCONFTOOL) --shutdown

EXTRA_DIST = $(glade_DATA) $(schema_in_files) $(stuff_DATA)

CLEANFILES=\
	$(schema_DATA)

