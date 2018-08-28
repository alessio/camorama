#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
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

int error_dialog (char *message)
{
    GtkWidget *dialog;
    int test;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    "%s", message);

    test = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return test;
}
