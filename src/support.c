#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <config.h>

GtkWidget *xpm_label_box (gchar * xpm_filename)
{
    GtkWidget *box;
    GtkWidget *image;

    /* Create box for image and label */
    box = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 2);

    /* Now on to the image stuff */
    image = gtk_image_new_from_file (xpm_filename);

    /* Create a label for the button */

    /* Pack the image and label into the box */
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 3);

    gtk_widget_show (image);

    return box;
}
GtkWidget *lookup_widget (GtkWidget * widget, const gchar * widget_name)
{
    GtkWidget *parent, *found_widget;

    for (;;) {
        if (GTK_IS_MENU (widget))
            parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
        else
            parent = widget->parent;
        if (parent == NULL)
            break;
        widget = parent;
    }

    found_widget =
        (GtkWidget *) gtk_object_get_data (GTK_OBJECT (widget), widget_name);
    if (!found_widget)
        g_warning ("Widget not found: %s", widget_name);
    return found_widget;
}
GdkPixbuf *create_pixbuf (const gchar * filename)
{
    gchar *pathname = NULL;
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    if (!filename || !filename[0])
        return NULL;

    pathname =
        gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
                                   filename, TRUE, NULL);

    if (!pathname) {
        g_warning (_("Couldn't find pixmap file: %s"), filename);
        return NULL;
    }
    pixbuf = gdk_pixbuf_new_from_file (pathname, &error);
    if (!pixbuf) {
        fprintf (stderr, "Failed to load pixbuf file: %s: %s\n", pathname,
                 error->message);
        g_error_free (error);
    }
    g_free (pathname);
    return pixbuf;
}

int error_dialog (char *message)
{
    GtkWidget *dialog;
    int test;

    dialog =
        gnome_message_box_new (message, GNOME_MESSAGE_BOX_ERROR,
                               GNOME_STOCK_BUTTON_CLOSE, NULL);
    test = gnome_dialog_run (GNOME_DIALOG (dialog));
    return test;
}
