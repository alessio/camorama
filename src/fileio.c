#include"fileio.h"
#include<time.h>
#include<errno.h>
#include "support.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include<stdio.h>
# define CHAR_HEIGHT  11
# define CHAR_WIDTH   6
# define CHAR_START   4
# include "font_6x11.h"
#include <gnome.h>

static int print_error (GnomeVFSResult result, const char *uri_string);

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
    len = strftime (line, 127, image_label, tm);
    
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

void remote_save (cam * cam)
{
    GThread *remote_thread;
    GnomeVFSHandle **write_handle;
    char *output_uri_string, *input_uri_string;
    GnomeVFSFileSize bytes_written;
    GnomeVFSURI *uri_1;
    unsigned char *tmp;
    GnomeVFSResult result = 0;
    gboolean test;
    char *filename, *error_message;
    FILE *fp;
    int bytes = 0, fc;
    time_t t;
    gchar *timenow, *ext;
    struct tm *tm;
    gboolean pbs;
    GdkPixbuf *pb;
    GError *error;

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
    //cam->tmp = NULL;

    if (cam->rtimestamp == TRUE) {
        add_rgb_text (cam->tmp, cam->width, cam->height, cam->ts_string,
                      cam->date_format, cam->usestring, cam->usedate);
    }

    if (chdir ("/tmp") != 0) {
        error_dialog (_("Could save temporary image file in /tmp."));
        g_free (ext);
        //g_thread_exit (NULL);
    }

    time (&t);
    tm = localtime (&t);
    strftime (timenow, sizeof (timenow) - 1, "%s", tm);

    filename = g_strdup_printf ("camorama.%s", ext);
    //g_free(ext);
    pb = gdk_pixbuf_new_from_data (cam->tmp, GDK_COLORSPACE_RGB, FALSE, 8,
                                   cam->width, cam->height,
                                   cam->width * cam->bpp / 8, NULL,
                                   NULL);
 
    if (pb == NULL) {
        error_message =
            g_strdup_printf (_("Unable to create image '%s'."), filename);
        error_dialog (error_message);
        g_free (error_message);
        //g_thread_exit (NULL);
    }

    pbs = gdk_pixbuf_save (pb, filename, ext, NULL, NULL);      //&error);//, NULL);
    if (pbs == FALSE) {
        error_message =
            g_strdup_printf (_("Could not save image '%s/%s'."),
                             cam->pixdir, filename);
        error_dialog (error_message);
        g_free (filename);
        //g_free (error_message);
        //return -1;
    }

    /*if (cam->debug == TRUE)
     * {
     * fprintf (stderr, "bytes to file %s: %d\n", filename, fc);
     * } */

    g_free (filename);
    /* from here :) */
    /* open tmp file and read it */
    /*input_uri_string = g_strdup_printf ("camorama.%s", ext);
     * 
     * if (!(fp = fopen (input_uri_string, "rb")))
     * {
     * error_message =
     * g_strdup_printf (_
     * ("Unable to open temporary image file '%s'."),
     * filename);
     * error_dialog (error_message);
     * g_free (input_uri_string);
     * g_free (error_message);
     * exit (0);
     * }
     * 
     * tmp = malloc (sizeof (char) * cam->width * cam->height * cam->bpp * 2 / 8);
     * while (!feof (fp))
     * {
     * bytes += fread (tmp, 1, cam->width * cam->height * cam->bpp / 8, fp);
     * }
     * fclose (fp);
     * 
     * time (&t);
     * tm = localtime (&t);
     * strftime (timenow, sizeof (timenow) - 1, "%s", tm);
     * if (cam->rtimefn == TRUE)
     * {
     * output_uri_string = g_strdup_printf ("ftp://%s/%s/%s-%s.%s",
     * cam->rhost, cam->rpixdir,
     * cam->rcapturefile,
     * timenow, ext);
     * }
     * else
     * {
     * output_uri_string =
     * g_strdup_printf ("ftp://%s/%s/%s.%s", cam->rhost,
     * cam->rpixdir, cam->rcapturefile,
     * ext);
     * }
     * uri_1 = gnome_vfs_uri_new (output_uri_string);
     * 
     * test = gnome_vfs_uri_exists (uri_1);
     * 
     * gnome_vfs_uri_set_user_name (uri_1, cam->rlogin);
     * gnome_vfs_uri_set_password (uri_1, cam->rpw);
     */
    /* start here? */
    /*result = gnome_vfs_open_uri((GnomeVFSHandle **) & write_handle, uri_1, GNOME_VFS_OPEN_WRITE);
     * if(result != GNOME_VFS_OK) {
     * error_message = g_strdup_printf(_("An error occurred opening %s."), output_uri_string);
     * error_dialog(error_message);
     * g_free(error_message);
     * g_thread_exit(NULL);
     * } */

    /*  write the data */
    /*result = gnome_vfs_write((GnomeVFSHandle *) write_handle, tmp, bytes, &bytes_written);
     * if(result != GNOME_VFS_OK) {
     * error_message = g_strdup_printf(_("An error occurred writing to %s."), output_uri_string);
     * error_dialog(error_message);
     * g_free(error_message);
     * } */

    remote_thread =
        g_thread_new ("remote", (GThreadFunc) save_thread, cam);
    g_free (ext);
    //free (tmp);

}

void save_thread (cam * cam)
{
    GnomeVFSHandle **write_handle;
    char *output_uri_string, *input_uri_string;
    GnomeVFSFileSize bytes_written;
    GnomeVFSURI *uri_1;
    unsigned char *tmp;
    GnomeVFSResult result = 0;
    gboolean test;
    char *filename, *error_message;
    FILE *fp;
    int bytes = 0;
    time_t t;
    gchar *timenow, *ext;
    struct tm *tm;
    gboolean pbs;
    GdkPixbuf *pb;
    /*GnomeVFSResult result = 0;
     * GnomeVFSHandle **write_handle;
     * char *error_message; */

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
        output_uri_string = g_strdup_printf ("ftp://%s/%s/%s-%s.%s",
                                             cam->rhost, cam->rpixdir,
                                             cam->rcapturefile, timenow, ext);
    } else {
        output_uri_string =
            g_strdup_printf ("ftp://%s/%s/%s.%s", cam->rhost,
                             cam->rpixdir, cam->rcapturefile, ext);
    }
    uri_1 = gnome_vfs_uri_new (output_uri_string);

    //test = gnome_vfs_uri_exists (uri_1);
    gnome_vfs_uri_set_user_name (uri_1, cam->rlogin);
    gnome_vfs_uri_set_password (uri_1, cam->rpw);
    

    result = gnome_vfs_open_uri ((GnomeVFSHandle **) & write_handle,
                                 uri_1, GNOME_VFS_OPEN_WRITE);
    if (result != GNOME_VFS_OK) {
        error_message =
            g_strdup_printf (_("An error occurred opening %s."),
                             output_uri_string);
        error_dialog (error_message);
        g_free (error_message);
        g_thread_exit (NULL);
    }

    /*  write the data */
    result = gnome_vfs_write ((GnomeVFSHandle *) write_handle, tmp, bytes,
                              &bytes_written);
    if (result != GNOME_VFS_OK) {
        error_message =
            g_strdup_printf (_("An error occurred writing to %s."),
                             output_uri_string);
        error_dialog (error_message);
        g_free (error_message);
    }
    gnome_vfs_close ((GnomeVFSHandle *) write_handle);
    gnome_vfs_shutdown ();
    free (tmp);
	 g_thread_exit (NULL);
}

static int print_error (GnomeVFSResult result, const char *uri_string)
{
    const char *error_string;
    /* get the string corresponding to this GnomeVFSResult value */
    error_string = gnome_vfs_result_to_string (result);
    printf ("Error %s occurred opening location %s\n", error_string,
            uri_string);
    return 1;
}

int local_save (cam * cam)
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
    //cam->tmp = NULL;
    //memcpy (cam->tmp, cam->pic_buf, cam->width * cam->height * cam->bpp / 8);

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
