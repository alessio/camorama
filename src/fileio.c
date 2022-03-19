#include "interface.h"
#include "support.h"
#include "fileio.h"
#include "v4l.h"

#include <errno.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <time.h>
#include <stdio.h>

#define CHAR_HEIGHT  11
#define CHAR_WIDTH   6
#define CHAR_START   4
#include "font_6x11.h"

/* add timestamp/text to image - "borrowed" from gspy */
int
add_rgb_text(guchar *image, int width, int height, char *cstring,
             char *format, gboolean str, gboolean date)
{
    time_t t;
    struct tm *tm;
    gchar line[128];
    guchar *ptr;
    int i, x, y, f, len;
    int total;
    gchar *image_label;

    if (str == TRUE && date == TRUE) {
        image_label = g_strdup_printf("%s - %s", cstring, format);
    } else if (str == TRUE && date == FALSE) {
        image_label = g_strdup_printf("%s", cstring);
    } else if (str == FALSE && date == TRUE) {
        image_label = g_strdup_printf("%s", format);
    } else if (str == FALSE && date == FALSE) {
        return 0;
    } else {
        image_label = g_strdup("");
    }

    time(&t);
    tm = localtime(&t);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    len = strftime(line, sizeof(line) - 1, image_label, tm);
#pragma GCC diagnostic pop
    g_free(image_label);

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

void remote_save(cam_t *cam)
{
    GThread *remote_thread;
    char *filename, *error_message;
    gchar *ext;
    gboolean pbs;
    GdkPixbuf *pb;

    /* Don't allow multiple threads to save image at the same time */
    g_mutex_lock(cam->remote_save_mutex);
    if (cam->n_threads) {
        g_mutex_unlock(cam->remote_save_mutex);
        return;
    }
    cam->n_threads++;
    g_mutex_unlock(cam->remote_save_mutex);

    switch (cam->rsavetype) {
    case JPEG:
        ext = g_strdup((gchar *) "jpeg");
        break;
    case PNG:
        ext = g_strdup((gchar *) "png");
        break;
    default:
        ext = g_strdup((gchar *) "jpeg");
    }

    if (chdir("/tmp") != 0) {
        error_dialog(_("Could save temporary image file in /tmp."));
        goto ret;
    }

    g_mutex_lock(cam->pixbuf_mutex);
    if (cam->rtimestamp == TRUE) {
        add_rgb_text(cam->pic_buf, cam->width, cam->height, cam->ts_string,
                     cam->date_format, cam->usestring, cam->usedate);
    }

    pb = gdk_pixbuf_new_from_data(cam->pic_buf, GDK_COLORSPACE_RGB, FALSE, 8,
                                  cam->width, cam->height,
                                  cam->width * cam->bpp / 8, NULL, NULL);
    g_mutex_unlock(cam->pixbuf_mutex);

    filename = g_strdup_printf("camorama.%s", ext);
    if (pb == NULL) {
        error_message = g_strdup_printf(_("Unable to create image '%s'."),
                                        filename);
        error_dialog(error_message);
        g_free(error_message);
        g_free(filename);

        goto ret;
    }

    pbs = gdk_pixbuf_save(pb, filename, ext, NULL, NULL);
    if (pbs == FALSE) {
        error_message = g_strdup_printf(_("Could not save image '%s/%s'."),
                                        cam->pixdir, filename);
        error_dialog(error_message);
        g_free(error_message);
        g_free(filename);

        goto ret;
    }

    remote_thread = g_thread_new("remote", &save_thread, cam);
    if (!remote_thread) {
        error_message = g_strdup_printf(_("Could not create a thread to save image '%s/%s'."),
                                        cam->pixdir, filename);
        error_dialog(error_message);
        g_free(error_message);
    }

    g_free(filename);

ret:
    g_mutex_lock(cam->remote_save_mutex);
    cam->n_threads--;
    g_mutex_unlock(cam->remote_save_mutex);

    g_free(ext);
}

struct mount_params {
    GFile *rdir_file;
    GMountOperation *mop;
    gchar *uri;
};

static void mount_cb(GObject *obj, GAsyncResult *res, gpointer user_data)
{
    cam_t *cam = user_data;
    gboolean ret;
    GError *err = NULL;

    ret = g_file_mount_enclosing_volume_finish(G_FILE(obj), res, &err);

    /* Ignore G_IO_ERROR_ALREADY_MOUNTED */
    if (g_error_matches(err, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED))
        ret = 1;

    if (ret) {
        cam->rdir_ok = TRUE;
        g_settings_set_string(cam->gc, CAM_SETTINGS_HOSTNAME, cam->host);
        g_settings_set_string(cam->gc, CAM_SETTINGS_REMOTE_PROTO, cam->proto);
        g_settings_set_string(cam->gc, CAM_SETTINGS_REMOTE_SAVE_DIR, cam->rdir);
        g_settings_set_string(cam->gc, CAM_SETTINGS_REMOTE_SAVE_FILE, cam->rcapturefile);
    } else {
        gchar *error_message = g_strdup_printf(_("An error occurred mounting %s:%s."),
                                               cam->uri, err->message);

        error_dialog(error_message);
        g_free(error_message);
    }
}

gchar *volume_uri(gchar *host, gchar *proto, gchar *rdir)
{
    return g_strdup_printf("%s://%s/%s", proto, host, rdir);
}

void umount_volume(cam_t *cam)
{
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
    /* Only try to mount if remote capture is enabled */
    if (!cam->rcap)
        return;

    /* Prepare a mount operation */
    cam->rdir_file = g_file_new_for_uri(cam->uri);
    if (cam->rdir_file)
        cam->rdir_mop = gtk_mount_operation_new(NULL);
    else
        cam->rdir_mop = NULL;

    if (!cam->rdir_mop) {
        gchar *error_message = g_strdup_printf(_("An error occurred accessing %s."),
                                               cam->uri);

        error_dialog(error_message);
        g_free(error_message);

        return;
    }

    g_file_mount_enclosing_volume(cam->rdir_file, G_MOUNT_MOUNT_NONE,
                                  cam->rdir_mop, NULL, mount_cb, cam);
}

gpointer save_thread(gpointer data)
{
    cam_t *cam = data;
    char *output_uri_string, *input_uri_string;
    GFile *uri_1;
    GFileOutputStream *fout;
    unsigned char *tmp;
    char *error_message;
    gssize ret;
    FILE *fp;
    int bytes = 0;
    time_t t;
    char timenow[64], *ext;
    struct tm *tm;
    GError *error = NULL;
    int len;

    /* Check if it is ready to mount */
    if (!cam->rdir_ok)
        return NULL;

    switch (cam->rsavetype) {
    case JPEG:
        ext = g_strdup((gchar *) "jpeg");
        break;
    case PNG:
        ext = g_strdup((gchar *) "png");
        break;
    default:
        ext = g_strdup((gchar *) "jpeg");
    }
    input_uri_string = g_strdup_printf("camorama.%s", ext);

    if (chdir("/tmp") != 0) {
        error_dialog(_("Could save temporary image file in /tmp."));
        g_free(ext);
        g_thread_exit(NULL);
    }

    if (!(fp = fopen(input_uri_string, "rb"))) {
        error_message = g_strdup_printf(_("Unable to open temporary image file '%s'.\nCannot upload image."),
                                        input_uri_string);
        error_dialog(error_message);
        g_free(input_uri_string);
        g_free(error_message);
        g_thread_exit(NULL);
        //exit (0);
    }

    tmp = malloc(sizeof(char) * cam->width * cam->height * cam->bpp * 2 / 8);
    while (!feof(fp)) {
        bytes += fread(tmp, 1, cam->width * cam->height * cam->bpp / 8, fp);
    }
    fclose(fp);

    time(&t);
    tm = localtime(&t);
    len = strftime(timenow, sizeof(timenow) - 1, "%Y%m%d-%H:%M:%S", tm);
    if (len < 0)
        timenow[0] = '\0';

    if (cam->rtimefn == TRUE) {
        output_uri_string = g_strdup_printf("%s/%s-%s-%03d.%s", cam->uri,
                                            cam->capturefile,
                                            timenow, cam->frame_number % 1000, ext);
    } else {
        output_uri_string = g_strdup_printf("%s/%s.%s", cam->uri,
                                            cam->capturefile, ext);
    }

    uri_1 = g_file_new_for_uri(output_uri_string);
    if (!uri_1) {
        error_message = g_strdup_printf(_("An error occurred opening %s."),
                                        output_uri_string);
        error_dialog(error_message);
        g_free(error_message);
        g_thread_exit(NULL);
    }

    fout = g_file_replace(uri_1, NULL, FALSE,
                          G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error);
    if (error) {
        error_message =
            g_strdup_printf(_("An error occurred opening %s for write: %s."),
                            output_uri_string, error->message);
        error_dialog(error_message);
        g_free(error_message);
        g_thread_exit(NULL);
    }

    /*  write the data */
    ret = g_output_stream_write(G_OUTPUT_STREAM(fout), tmp, bytes, NULL, &error);
    if (ret < 0) {
        g_output_stream_close(G_OUTPUT_STREAM(fout), NULL, &error);
        error_message = g_strdup_printf(_("An error occurred writing to %s: %s."),
                                        output_uri_string, error->message);
        error_dialog(error_message);
        g_free(error_message);
    }
    if (!g_output_stream_close(G_OUTPUT_STREAM(fout), NULL, &error)) {
        error_message = g_strdup_printf(_("An error occurred closing %s: %s."),
                                        output_uri_string, error->message);
        error_dialog(error_message);
        g_free(error_message);
    }

    g_object_unref(uri_1);
    free(tmp);
    g_thread_exit(NULL);

    return NULL;
}

int local_save(cam_t *cam)
{
    gchar *filename, *ext;
    time_t t;
    struct tm *tm;
    char timenow[64], *error_message;
    int len, mkd;
    gboolean pbs;
    GdkPixbuf *pb;

    /*
     * TODO: run gdk-pixbuf-query-loaders to get available image types
     */

    switch (cam->savetype) {
    case JPEG:
        ext = g_strdup((gchar *) "jpeg");
        break;
    case PNG:
        ext = g_strdup((gchar *) "png");
        break;
    default:
        ext = g_strdup((gchar *) "jpeg");
    }

    time(&t);
    tm = localtime(&t);
    len = strftime(timenow, sizeof(timenow) - 1, "%Y%m%d-%H:%M:%S", tm);
    if (len < 0)
        timenow[0] = '\0';

    if (cam->debug == TRUE)
        fprintf(stderr, "time = %s\n", timenow);

    if (cam->timefn == TRUE)
        filename = g_strdup_printf("%s-%s-%03d.%s",
                                   cam->capturefile, timenow,
                                   cam->frame_number % 1000, ext);
    else
        filename = g_strdup_printf("%s.%s", cam->capturefile, ext);

    if (cam->debug == TRUE)
        fprintf(stderr, "filename = %s\n", filename);

    mkd = mkdir(cam->pixdir, 0777);

    if (cam->debug == TRUE)
        perror("create dir: ");

    if (mkd != 0 && errno != EEXIST) {
        error_message = g_strdup_printf(_("Could not create directory '%s'."),
                                        cam->pixdir);
        error_dialog(error_message);
        g_free(filename);
        g_free(error_message);
        return -1;
    }

    if (chdir(cam->pixdir) != 0) {
        error_message = g_strdup_printf(_("Could not change to directory '%s'."),
                                        cam->pixdir);
        error_dialog(error_message);
        g_free(filename);
        g_free(error_message);
        return -1;
    }

    g_mutex_lock(cam->pixbuf_mutex);
    if (cam->timestamp == TRUE)
        add_rgb_text(cam->pic_buf, cam->width, cam->height, cam->ts_string,
                     cam->date_format, cam->usestring, cam->usedate);

    pb = gdk_pixbuf_new_from_data(cam->pic_buf, GDK_COLORSPACE_RGB, FALSE, 8,
                                  cam->width, cam->height,
                                  (cam->width * cam->bpp / 8), NULL, NULL);
    g_mutex_unlock(cam->pixbuf_mutex);

    pbs = gdk_pixbuf_save(pb, filename, ext, NULL, NULL);
    if (pbs == FALSE) {
        error_message = g_strdup_printf(_("Could not save image '%s/%s'."),
                                        cam->pixdir, filename);
        error_dialog(error_message);
        g_free(filename);
        g_free(error_message);
        return -1;
    }

    g_free(filename);
    return 0;
}
