#include <errno.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <time.h>
#include <stdio.h>

#include "interface.h"
#include "support.h"
#include "fileio.h"

# define CHAR_HEIGHT  11
# define CHAR_WIDTH   6
# define CHAR_START   4
# include "font_6x11.h"

/* add timestamp/text to image - "borrowed" from gspy */
int
add_rgb_text (guchar *image, int width, int height, char *cstring, char *format,
              gboolean str, gboolean date)
{
    time_t t;
    struct tm *tm;
    gchar line[128];
    guchar *ptr;
    int i, x, y, f, len;
    int total;
    gchar *image_label;

    if (str == TRUE && date == TRUE) {
        image_label = g_strdup_printf ("%s - %s", cstring, format);

    } else if (str == TRUE && date == FALSE) {
        image_label = g_strdup_printf ("%s", cstring);
    } else if (str == FALSE && date == TRUE) {
        image_label = g_strdup_printf ("%s", format);
    } else if (str == FALSE && date == FALSE) {
        return 0;
    }

    time (&t);
    tm = localtime (&t);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    len = strftime (line, sizeof(line) - 1, image_label, tm);
#pragma GCC diagnostic pop

    for (y = 0; y < CHAR_HEIGHT; y++) {
        /* locate text in lower left corner of image */
        ptr = image + 3 * width * (height - CHAR_HEIGHT - 2 + y) + 12;

        /* loop for each character in the string */
        for (x = 0; x < len; x++) {
            /* locate the character in the fontdata array */
            f = fontdata[line[x] * CHAR_HEIGHT + y];

            /* loop for each column of font data */
            for (i = CHAR_WIDTH - 1; i >= 0; i--) {
                /* write a black background under text
                 * comment out the following block to get white letters on picture background */
                /* ptr[0] = 0;
                 * ptr[1] = 0;
                 * ptr[2] = 0; */
                if (f & (CHAR_START << i)) {

                    /* white text */

                    total = ptr[0] + ptr[1] + ptr[2];
                    if (total / 3 < 128) {
                        ptr[0] = 255;
                        ptr[1] = 255;
                        ptr[2] = 255;
                    } else {
                        ptr[0] = 0;
                        ptr[1] = 0;
                        ptr[2] = 0;
                    }
                }
                ptr += 3;
            }
        }
    }
    return 1;
}

void remote_save (cam_t *cam)
{
    GThread *remote_thread;
    char *filename, *error_message;
    gchar *ext;
    gboolean pbs;
    GdkPixbuf *pb;

    switch (cam->rsavetype) {
    case JPEG:
        ext = g_strdup ((gchar *) "jpeg");
        break;
    case PNG:
        ext = g_strdup ((gchar *) "png");
        break;
    default:
        ext = g_strdup ((gchar *) "jpeg");
    }

    if (cam->rtimestamp == TRUE) {
        add_rgb_text (cam->tmp, cam->width, cam->height, cam->ts_string,
                      cam->date_format, cam->usestring, cam->usedate);
    }

    if (chdir ("/tmp") != 0) {
        error_dialog (_("Could save temporary image file in /tmp."));
        g_free (ext);
    }

    filename = g_strdup_printf ("camorama.%s", ext);
    pb = gdk_pixbuf_new_from_data (cam->tmp, GDK_COLORSPACE_RGB, FALSE, 8,
                                   cam->width, cam->height,
                                   cam->width * cam->bpp / 8, NULL,
                                   NULL);
 
    if (pb == NULL) {
        error_message =
            g_strdup_printf (_("Unable to create image '%s'."), filename);
        error_dialog (error_message);
        g_free (error_message);
    }

    pbs = gdk_pixbuf_save (pb, filename, ext, NULL, NULL);
    if (pbs == FALSE) {
        error_message =
            g_strdup_printf (_("Could not save image '%s/%s'."),
                             cam->pixdir, filename);
        error_dialog (error_message);
        g_free (filename);
    }

    g_free (filename);

    remote_thread =
        g_thread_new ("remote", (GThreadFunc) save_thread, cam);
    g_free (ext);
}

struct mount_params {
    GFile *rdir_file;
    GMountOperation *mop;
    gchar *uri;
};

static void mount_cb (GObject * obj, GAsyncResult * res, gpointer user_data)
{
    cam_t *cam = user_data;
    gboolean ret;
    GError *err = NULL;

    ret = g_file_mount_enclosing_volume_finish (G_FILE (obj), res, &err);

    /* Ignore G_IO_ERROR_ALREADY_MOUNTED */
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED))
        ret = 1;

    if (ret) {
        cam->rdir_ok = TRUE;
        gconf_client_set_string(cam->gc, KEY5, cam->host, NULL);
        gconf_client_set_string(cam->gc, KEY6, cam->proto, NULL);
        gconf_client_set_string(cam->gc, KEY8, cam->rdir, NULL);
        gconf_client_set_string(cam->gc, KEY9, cam->rcapturefile, NULL);
    } else {
        gchar *error_message = g_strdup_printf (_("An error occurred mounting %s:%s."),
                                                cam->uri, err->message);
        error_dialog (error_message);
        g_free (error_message);
    }
}

gchar *volume_uri(gchar *host, gchar *proto, gchar *rdir)
{
    return g_strdup_printf ("%s://%s/%s", proto, host, rdir);
}

void umount_volume(cam_t *cam)
{
    struct mount_params mount;

    /* Unmount previous volume */
    if (!cam->rdir_ok)
        return;

    cam->rdir_ok = FALSE;
    g_file_unmount_mountable_with_operation(cam->rdir_file,
                                            G_MOUNT_UNMOUNT_NONE,
                                            cam->rdir_mop, NULL,
                                            NULL, cam);
}

void mount_volume(cam_t *cam)
{
    /* Prepare a mount operation */
    cam->rdir_file = g_file_new_for_uri(cam->uri);
    if (cam->rdir_file)
        cam->rdir_mop = gtk_mount_operation_new(NULL);
    else
        cam->rdir_mop = NULL;

    if (!cam->rdir_mop) {
        gchar *error_message = g_strdup_printf (_("An error occurred accessing %s."),
                                                cam->uri);
        error_dialog (error_message);
        g_free (error_message);

        return;
    }

    g_file_mount_enclosing_volume(cam->rdir_file,  G_MOUNT_MOUNT_NONE,
                                  cam->rdir_mop, NULL, mount_cb, cam);
}

void save_thread (cam_t *cam)
{
    char *output_uri_string, *input_uri_string;
    GFile *uri_1;
    GFileOutputStream *fout;
    unsigned char *tmp;
    gboolean test;
    char *filename, *error_message;
    FILE *fp;
    int bytes = 0;
    time_t t;
    char timenow[64], *ext;
    struct tm *tm;
    gboolean pbs;
    GdkPixbuf *pb;
    GError *error = NULL;

    /* Check if it is ready to mount */
    if (!cam->rdir_ok)
        return;

    switch (cam->rsavetype) {
    case JPEG:
        ext = g_strdup ((gchar *) "jpeg");
        break;
    case PNG:
        ext = g_strdup ((gchar *) "png");
        break;
    default:
        ext = g_strdup ((gchar *) "jpeg");
    }
    input_uri_string = g_strdup_printf ("camorama.%s", ext);

    if (chdir ("/tmp") != 0) {
        error_dialog (_("Could save temporary image file in /tmp."));
        g_free (ext);
        g_thread_exit (NULL);
    }

    if (!(fp = fopen (input_uri_string, "rb"))) {
        error_message =
            g_strdup_printf (_
                             ("Unable to open temporary image file '%s'.\nCannot upload image."),
                             input_uri_string);
        error_dialog (error_message);
        g_free (input_uri_string);
        g_free (error_message);
        g_thread_exit (NULL);
        //exit (0);
    }

    tmp = malloc (sizeof (char) * cam->width * cam->height * cam->bpp * 2 / 8);
    while (!feof (fp)) {
        bytes += fread (tmp, 1, cam->width * cam->height * cam->bpp / 8, fp);
    }
    fclose (fp);

    time (&t);
    tm = localtime (&t);
    strftime (timenow, sizeof (timenow) - 1, "%s", tm);
    if (cam->rtimefn == TRUE) {
        output_uri_string = g_strdup_printf ("%s/%s-%s.%s", cam->uri,
                                             cam->capturefile,
                                             timenow, ext);
    } else {
        output_uri_string = g_strdup_printf ("%s/%s.%s", cam->uri,
                                             cam->capturefile, ext);
    }

    uri_1 = g_file_new_for_uri (output_uri_string);
    if (!uri_1) {
        error_message =
            g_strdup_printf (_("An error occurred opening %s."),
                             output_uri_string);
        error_dialog (error_message);
        g_free (error_message);
        g_thread_exit (NULL);
    }

    fout = g_file_replace (uri_1, NULL, FALSE,
                           G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error);
    if (error) {
        error_message =
            g_strdup_printf (_("An error occurred opening %s for write: %s."),
                             output_uri_string, error->message);
        error_dialog (error_message);
        g_free (error_message);
        g_thread_exit (NULL);
    }

    /*  write the data */
    g_output_stream_write(G_OUTPUT_STREAM(fout), tmp, bytes, NULL, &error);
    if (error) {
        error_message =
            g_strdup_printf (_("An error occurred writing to %s: %s."),
                             output_uri_string, error->message);
        error_dialog (error_message);
        g_free (error_message);
    }
    g_output_stream_close(G_OUTPUT_STREAM(fout), NULL, &error);
    if (error) {
        error_message =
            g_strdup_printf (_("An error occurred closing %s: %s."),
                             output_uri_string, error->message);
        error_dialog (error_message);
        g_free (error_message);
    }

    g_object_unref(uri_1);
    free (tmp);
	 g_thread_exit (NULL);
}

int local_save (cam_t *cam)
{
    int fc;
    gchar *filename, *ext;
    time_t t;
    struct tm *tm;
    char timenow[64], *error_message;
    int len, mkd;
    gboolean pbs;
    GdkPixbuf *pb;

    /* todo:
     *   run gdk-pixbuf-query-loaders to get available image types*/

    switch (cam->savetype) {
    case JPEG:
        ext = g_strdup ((gchar *) "jpeg");
        break;
    case PNG:
        ext = g_strdup ((gchar *) "png");
        break;
    default:
        ext = g_strdup ((gchar *) "jpeg");
    }

    if (cam->timestamp == TRUE) {
        add_rgb_text (cam->tmp, cam->width, cam->height, cam->ts_string,
                      cam->date_format, cam->usestring, cam->usedate);
    }

    time (&t);
    tm = localtime (&t);
    len = strftime (timenow, sizeof (timenow) - 1, "%s", tm);

    if (cam->debug == TRUE) {
        fprintf (stderr, "time = %s\n", timenow);
    }

    if (cam->timefn == TRUE) {
        filename =
            g_strdup_printf ("%s-%s.%s", cam->capturefile, timenow, ext);
    } else {
        filename = g_strdup_printf ("%s.%s", cam->capturefile, ext);
    }

    if (cam->debug == TRUE) {
        fprintf (stderr, "filename = %s\n", filename);
    }
    mkd = mkdir (cam->pixdir, 0777);

    if (cam->debug == TRUE) {
        perror ("create dir: ");
    }

    if (mkd != 0 && errno != EEXIST) {
        error_message =
            g_strdup_printf (_
                             ("Could not create directory '%s'."),
                             cam->pixdir);
        error_dialog (error_message);
        g_free (filename);
        g_free (error_message);
        return -1;
    }

    if (chdir (cam->pixdir) != 0) {
        error_message =
            g_strdup_printf (_
                             ("Could not change to directory '%s'."),
                             cam->pixdir);
        error_dialog (error_message);
        g_free (filename);
        g_free (error_message);
        return -1;
    }

    pb = gdk_pixbuf_new_from_data (cam->tmp, GDK_COLORSPACE_RGB, FALSE, 8,
                                   cam->width, cam->height,
                                   (cam->width * cam->bpp / 8), NULL,
                                   NULL);
    pbs = gdk_pixbuf_save (pb, filename, ext, NULL, NULL);
     if (pbs == FALSE) {
        error_message =
            g_strdup_printf (_("Could not save image '%s/%s'."),
                             cam->pixdir, filename);
        error_dialog (error_message);
        g_free (filename);
        g_free (error_message);
        return -1;
    }

    g_free (filename);
    return 0;
}
