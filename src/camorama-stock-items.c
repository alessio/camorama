#include "camorama-stock-items.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static GtkStockItem camorama_items[] = {
	{CAMORAMA_STOCK_WEBCAM, N_("Webcam"), 0, 0, "camorama"}
};

static void
add_default_image(const gchar* stock_id, gint size, gchar *pixfilename)
{
	GdkPixbuf* buf = gdk_pixbuf_new_from_file(pixfilename, NULL);
	g_return_if_fail(buf);

	gtk_icon_theme_add_builtin_icon(stock_id, size, buf);
	g_object_unref(buf);
}

void
camorama_stock_init(void) {
	gchar *pixfilename;
	GtkIconFactory* factory = gtk_icon_factory_new();
	GtkIconSet    * set = gtk_icon_set_new ();
	GtkIconSource * source = gtk_icon_source_new();

	gtk_stock_add_static(camorama_items, G_N_ELEMENTS(camorama_items));

	gtk_icon_source_set_size_wildcarded(source, TRUE);
	gtk_icon_source_set_direction_wildcarded(source, TRUE);
	gtk_icon_source_set_state_wildcarded(source, TRUE);

	gtk_icon_source_set_icon_name(source, CAMORAMA_STOCK_WEBCAM);
	gtk_icon_set_add_source(set, source);

	gtk_icon_factory_add(factory, CAMORAMA_STOCK_WEBCAM, set);

	add_default_image(CAMORAMA_STOCK_WEBCAM, 16, PACKAGE_DATA_DIR "/pixmaps/camorama-webcam-16.png");
	add_default_image(CAMORAMA_STOCK_WEBCAM, 16, PACKAGE_DATA_DIR "/pixmaps/camorama-webcam-24.png");

	gtk_icon_factory_add_default(factory);

	gtk_icon_set_unref(set);
	gtk_icon_source_free(source);
}

