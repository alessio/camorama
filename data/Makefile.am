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


gsettings_SCHEMAS = org.gnome.camorama.gschema.xml

@GSETTINGS_RULES@

# For the sake of some users
user-install: $(app_DATA) $(keys_DATA)
	mkdir -p ~/.gnome/application-registry ~/.gnome/mime-info/
	cp $(app_DATA) ~/.gnome/application-registry
	cp $(keys_DATA) ~/.gnome/mime-info/

EXTRA_DIST = $(glade_DATA) $(stuff_DATA)

