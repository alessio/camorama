#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "filter.h"

#include <assert.h>
#include <ftw.h>
#include <glib/gi18n.h>
#include <config.h>
#include <pthread.h>
#include <libv4l2.h>
#include <sys/sysmacros.h>

#define GPL_LICENSE \
    "GPL version 2.\n\n"                                                      \
    "This program is free software; you can redistribute it and/or modify it" \
    " under the terms of the GNU General Public License as published by the " \
    "Free Software Foundation version 2 of the License.\n\n"                  \
    "This program is distributed in the hope that it will be useful, but "    \
    "WITHOUT ANY WARRANTY; without even the implied warranty of "             \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "      \
    "General Public License for more details."

static GtkWidget *about = NULL;

extern GtkWidget *prefswindow;
//extern state func_state;
//extern gint effect_mask;
extern int frames;
extern int frames2;
extern int seconds;
extern GtkWidget *dentry, *entry2, *string_entry;
extern GtkWidget *host_entry, *protocol, *rdir_entry, *filename_entry;
extern const gchar *const protos[];

/*
 * pref callbacks
 */

void ts_func(GtkWidget *rb, cam_t *cam)
{
    cam->timestamp = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_TIMESTAMP, cam->timestamp);
}

void customstring_func(GtkWidget *rb, cam_t *cam)
{
    cam->usestring = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_USE_CUSTOM_STRING,
                           cam->usestring);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "string_entry")),
                             cam->usestring);
}

void drawdate_func(GtkWidget *rb, cam_t *cam)
{
    cam->usedate = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_DRAWDATE, cam->usedate);
}

void append_func(GtkWidget *rb, cam_t *cam)
{
    cam->timefn = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_LOCAL_APPEND_TS, cam->timefn);

}

void rappend_func(GtkWidget *rb, cam_t *cam)
{
    cam->rtimefn = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_REMOTE_APPEND_TS, cam->rtimefn);

}

void jpg_func(GtkWidget *rb, cam_t *cam)
{
    cam->savetype = JPEG;
    g_settings_set_int(cam->gc, CAM_SETTINGS_FILE_TYPE, cam->savetype);

}

void png_func(GtkWidget *rb, cam_t *cam)
{
    cam->savetype = PNG;
    g_settings_set_int(cam->gc, CAM_SETTINGS_FILE_TYPE, cam->savetype);
}

void ppm_func(GtkWidget *rb, cam_t *cam)
{
    cam->savetype = PPM;
    g_settings_set_int(cam->gc, CAM_SETTINGS_FILE_TYPE, cam->savetype);
}

void set_sensitive(cam_t *cam)
{
    gtk_widget_set_sensitive(GTK_WIDGET
                             (gtk_builder_get_object(cam->xml, "table4")),
                             cam->cap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "appendbutton")),
                             cam->cap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object
                              (cam->xml, "tsbutton")), cam->cap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "jpgb")),
                             cam->cap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "pngb")),
                             cam->cap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "table5")),
                             cam->rcap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "timecb")),
                             cam->rcap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "tsbutton2")),
                             cam->rcap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "fjpgb")),
                             cam->rcap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "fpngb")),
                             cam->rcap);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "hbox20")),
                             cam->acap);
}

void cap_func(GtkWidget *rb, cam_t *cam)
{
    cam->cap = gtk_toggle_button_get_active((GtkToggleButton *) rb);

    g_settings_set_boolean(cam->gc, CAM_SETTINGS_LOCAL_CAPTURE, cam->cap);
    set_sensitive(cam);
}

void rcap_func(GtkWidget *rb, cam_t *cam)
{
    cam->rcap = gtk_toggle_button_get_active((GtkToggleButton *) rb);

    g_settings_set_boolean(cam->gc, CAM_SETTINGS_REMOTE_CAPTURE, cam->rcap);
    set_sensitive(cam);

}

void acap_func(GtkWidget *rb, cam_t *cam)
{
    cam->acap = gtk_toggle_button_get_active((GtkToggleButton *) rb);

    g_settings_set_boolean(cam->gc, CAM_SETTINGS_AUTO_CAPTURE, cam->acap);

    if (cam->acap == TRUE) {
        cam->timeout_id = g_timeout_add(cam->timeout_interval,
                                        (GSourceFunc) timeout_capture_func, cam);
        if (cam->debug == TRUE) {
            printf("add autocap - %d -  timeout_interval = %d\n",
                   cam->timeout_id, cam->timeout_interval);
        }
    } else {
        if (cam->debug == TRUE) {
            printf("remove autocap - %d -  timeout_interval = %d\n",
                   cam->timeout_id, cam->timeout_interval);
        }
        g_source_remove(cam->timeout_id);
    }
    set_sensitive(cam);
}

void interval_change(GtkWidget *sb, cam_t *cam)
{
    cam->timeout_interval = gtk_spin_button_get_value((GtkSpinButton *) sb) * 60000;
    g_settings_set_int(cam->gc, CAM_SETTINGS_AUTO_CAPTURE_INTERVAL,
                       cam->timeout_interval);
    if (cam->acap == TRUE) {
        if (cam->debug == TRUE) {
            printf("interval_change; old timeout_id = %d old interval = %d\n",
                   cam->timeout_id, cam->timeout_interval);
        }
        g_source_remove(cam->timeout_id);
        cam->timeout_id = g_timeout_add(cam->timeout_interval,
                                        (GSourceFunc) timeout_capture_func, cam);
        if (cam->debug == TRUE) {
            printf("new timeout_id = %d, new interval = %d\n",
                   cam->timeout_id, cam->timeout_interval);
        }
    }
}

void rjpg_func(GtkWidget *rb, cam_t *cam)
{
    cam->rsavetype = JPEG;
    g_settings_set_int(cam->gc, CAM_SETTINGS_REMOTE_FILE_TYPE, cam->rsavetype);

}

void rpng_func(GtkWidget *rb, cam_t *cam)
{
    cam->rsavetype = PNG;
    g_settings_set_int(cam->gc, CAM_SETTINGS_REMOTE_FILE_TYPE, cam->rsavetype);
}

void rppm_func(GtkWidget *rb, cam_t *cam)
{
    cam->rsavetype = PPM;
    g_settings_set_int(cam->gc, CAM_SETTINGS_REMOTE_FILE_TYPE, cam->rsavetype);
}

void rts_func(GtkWidget *rb, cam_t *cam)
{
    cam->rtimestamp = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_REMOTE_TIMESTAMP,
                           cam->rtimestamp);
}

static int apply_remote_pref(cam_t *cam)
{
    int index;
    gchar *host, *rdir, *proto, *rfile, *uri;

    if (!strlen(gtk_entry_get_text((GtkEntry *) host_entry)))
        return 0;

    index = gtk_combo_box_get_active(GTK_COMBO_BOX(protocol));

    host = g_strdup(gtk_entry_get_text((GtkEntry *) host_entry));
    rdir = g_strdup(gtk_entry_get_text((GtkEntry *) rdir_entry));
    proto = g_strdup(protos[index]);
    rfile = g_strdup(gtk_entry_get_text((GtkEntry *) filename_entry));

    if (!host || !proto || !rdir || !rfile) {
        if (host)
            g_free(host);
        if (proto)
            g_free(proto);
        if (rdir)
            g_free(rdir);
        if (rfile)
            g_free(rfile);
        return 0;
    }

    uri = volume_uri(host, proto, rdir);

    if (cam->rdir_ok) {
        /* unmount/mount can spend time. Do only if URI changed */
        if (strcmp(uri, cam->uri)) {
            umount_volume(cam);
        } else {
            g_free(host);
            g_free(proto);
            g_free(rdir);
            g_free(rfile);
            g_free(uri);

            return 0;
        }
    }

    if (cam->host)
        g_free(cam->host);
    if (cam->proto)
        g_free(cam->proto);
    if (cam->rdir)
        g_free(cam->rdir);
    if (cam->rcapturefile)
        g_free(cam->rcapturefile);
    if (cam->uri)
        g_free(cam->uri);

    cam->host = host;
    cam->rdir = rdir;
    cam->proto = proto;
    cam->rcapturefile = rfile;
    cam->uri = uri;

    mount_volume(cam);

    return 1;
}

/*
 * apply preferences
 */
void prefs_func(GtkWidget *okbutton, cam_t *cam)
{
    if (gtk_file_chooser_get_current_folder((GtkFileChooser *) dentry)) {
        if (cam->pixdir)
            g_free(cam->pixdir);
        cam->pixdir = gtk_file_chooser_get_current_folder((GtkFileChooser *) dentry);
        g_settings_set_string(cam->gc, CAM_SETTINGS_SAVE_DIR, cam->pixdir);
    } else {
        if (cam->debug == TRUE)
            fprintf(stderr, "null directory\ndirectory unchanged.");
    }

    if (!apply_remote_pref(cam) && cam->debug == TRUE)
        fprintf(stderr,
                "remote directory params wrong\ndirectory unchanged.");

    /*
     * this is stupid, even if the string is empty, it will not return NULL
     */
    if (strlen(gtk_entry_get_text((GtkEntry *) entry2)) > 0) {
        if(cam->capturefile)
            g_free(cam->capturefile);
        cam->capturefile = g_strdup(gtk_entry_get_text((GtkEntry *) entry2));
        g_settings_set_string(cam->gc, CAM_SETTINGS_SAVE_FILE,
                              cam->capturefile);
    }

    if (strlen(gtk_entry_get_text((GtkEntry *) string_entry)) > 0) {
        if (cam->ts_string)
            g_free(cam->ts_string);
        cam->ts_string = g_strdup(gtk_entry_get_text((GtkEntry *)string_entry));
        g_settings_set_string(cam->gc, CAM_SETTINGS_TIMESTAMP_STRING,
                              cam->ts_string);
    }
    if (cam->debug == TRUE) {
        fprintf(stderr, "dir now = %s\nfile now = %s\n", cam->pixdir,
                cam->capturefile);
    }
    gtk_widget_hide(prefswindow);
}

gboolean delete_event_prefs_window(GtkWidget *widget, GdkEvent *event,
                                   cam_t *cam)
{
    prefs_func(widget, cam);
    return TRUE;
}

void on_quit_activate(GtkMenuItem *menuitem, cam_t *cam)
{
#if GTK_MAJOR_VERSION >= 3
    g_application_quit(G_APPLICATION(cam->app));
#else
    gtk_main_quit();
#endif
}

void on_preferences1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    gtk_widget_show(prefswindow);
}

void set_image_scale(cam_t *cam)
{
    gchar *title;
    float f;

    if (cam->width > 0.66 * cam->screen_width || cam->height > 0.66 * cam->screen_height) {
        cam->scale = (0.66 * cam->screen_width) / cam->width;
        f = (0.66 * cam->screen_height) / cam->height;

        if (f < cam->scale)
            cam->scale = f;
        if (cam->scale < 0.1f)
          cam->scale = 0.1f;
    } else {
        cam->scale = 1.f;
    }

    gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(cam->xml, "da")),
                                cam->width * cam->scale,
                                cam->height * cam->scale);

    gtk_window_resize(GTK_WINDOW(GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"))),
                      320, 240);

    if (cam->scale == (float)1.)
        title = g_strdup_printf("Camorama - %s - %dx%d", cam->name,
                                cam->width, cam->height);
    else
        title = g_strdup_printf("Camorama - %s - %dx%d (scale: %d%%)", cam->name,
                                cam->width, cam->height, (int)(cam->scale * 100.f));
    gtk_window_set_title(GTK_WINDOW(GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"))),
                         title);
    g_free(title);

    g_settings_set_int(cam->gc, CAM_SETTINGS_WIDTH, cam->width);
    g_settings_set_int(cam->gc, CAM_SETTINGS_HEIGHT, cam->height);
}

void on_change_size_activate(GtkWidget *widget, cam_t *cam)
{
    gchar const *name;
    unsigned int width = 0, height = 0;

    name = gtk_widget_get_name(widget);

    if (strcmp(name, "small") == 0) {
        width = cam->min_width;
        height = cam->min_height;
    } else if (strcmp(name, "medium") == 0) {
        width = cam->max_width / 2;
        height = cam->max_height / 2;
    } else if (strcmp(name, "large") == 0) {
        width = cam->max_width;
        height = cam->max_height;
    } else {
        sscanf(name, "%dx%d", &width, &height);
    }

    try_set_win_info(cam, &width, &height);

    /* Nothing to do, so just return */
    if (width == cam->width && height == cam->height)
        return;

    cam->width = width;
    cam->height = height;

    if (cam->debug == TRUE)
        printf("name = %s\n", name);

    if (cam->read == FALSE)
        stop_streaming(cam);
    set_win_info(cam);
    if (cam->read == FALSE)
        start_streaming(cam);

    set_image_scale(cam);
}

void on_show_adjustments_activate(GtkToggleButton *button, cam_t *cam)
{
    if (gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(cam->xml, "adjustments_table")))) {
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "adjustments_table")));
        gtk_window_resize(GTK_WINDOW(GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"))),
                          320, 240);
        cam->show_adjustments = FALSE;

    } else {
        gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(cam->xml, "adjustments_table")));
        cam->show_adjustments = TRUE;
    }
    g_settings_set_boolean(cam->gc, CAM_SETTINGS_SHOW_ADJUSTMENTS,
                           cam->show_adjustments);
}

void on_show_effects_activate(GtkMenuItem *menuitem, cam_t *cam)
{
    GtkWidget *effects = GTK_WIDGET(gtk_builder_get_object(cam->xml,
                                                           "scrolledwindow_effects"));
    cam->show_effects = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));

    if (!cam->show_effects) {
        gtk_widget_hide(effects);
        gtk_window_resize(GTK_WINDOW(GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"))),
                          320, 240);
    } else {
        gtk_widget_show(effects);
    }

    g_settings_set_boolean(cam->gc, CAM_SETTINGS_SHOW_EFFECTS, cam->show_effects);
}

static void about_widget_destroy(GtkWidget *widget)
{
    gtk_widget_destroy(about);
    about = NULL;
}


void on_about_activate(GtkMenuItem *menuitem, cam_t *cam)
{
    const gchar *authors[] = {
        "Greg Jones  <greg@fixedgear.org>",
        "Jens Knutson  <tempest@magusbooks.com>",
        NULL
    };
    const gchar *comments = _("View, alter and save images from a webcam");
    const gchar *translators = _("translator_credits");
    GdkPixbuf *logo = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR
                                               "/pixmaps/camorama.png",
                                               NULL);

    if (!strcmp(translators, "translator_credits"))
        translators = NULL;
    if (about != NULL) {
        gtk_window_present(GTK_WINDOW(about));
        return;
    }
    about = g_object_new(GTK_TYPE_ABOUT_DIALOG,
                         "name", "Camorama",
                         "version", PACKAGE_VERSION,
                         "copyright", "Copyright \xc2\xa9 2002 Greg Jones",
                         "comments", comments,
                         "authors", authors,
                         "translator-credits", translators,
                         "logo", logo,
                         "license", GPL_LICENSE,
                         "wrap-license", TRUE,
                         NULL);
    gtk_window_set_transient_for(GTK_WINDOW(about),
                                 GTK_WINDOW(GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"))));
    g_signal_connect(about, "response",
                     G_CALLBACK(about_widget_destroy), NULL);
    gtk_widget_show(about);
}

static void apply_filters(cam_t *cam, unsigned char *pic_buf)
{
    /* v4l has reverse rgb order from what camora expect so call the color
       filter to fix things up before running the user selected filters */
    camorama_filter_color_filter(NULL, pic_buf, cam->width,
                                 cam->height, cam->bpp / 8);
    camorama_filter_chain_apply(cam->filter_chain, pic_buf,
                                cam->width, cam->height, cam->bpp / 8);
}

#define MULT(d, c, a, t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

/*
 * As GTK+2 doesn't have gdk_cairo_surface_create_from_pixbuf, we
 * Borrowed its implementation from
 *    https://github.com/bratsche/gtk-/blob/master/gdk/gdkcairo.c
 * With a small backport.
 */
#if GTK_MAJOR_VERSION < 3
static cairo_surface_t *create_from_pixbuf(const GdkPixbuf *pixbuf,
                                           GdkWindow *for_window)
{
    gint width = gdk_pixbuf_get_width(pixbuf);
    gint height = gdk_pixbuf_get_height(pixbuf);
    guchar *gdk_pixels = gdk_pixbuf_get_pixels(pixbuf);
    int gdk_rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int cairo_stride;
    guchar *cairo_pixels;
    cairo_surface_t *surface;
    int j;

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                         width, height);
    cairo_stride = cairo_image_surface_get_stride(surface);
    cairo_pixels = cairo_image_surface_get_data(surface);

    for (j = height; j; j--) {
        guchar *p = gdk_pixels;
        guchar *q = cairo_pixels;

        if (n_channels == 3) {
            guchar *end = p + 3 * width;

            while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                q[0] = p[2];
                q[1] = p[1];
                q[2] = p[0];
#else
                q[1] = p[0];
                q[2] = p[1];
                q[3] = p[2];
#endif
                p += 3;
                q += 4;
            }
        } else {
            guchar *end = p + 4 * width;
            guint t1, t2, t3;

            while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                MULT(q[0], p[2], p[3], t1);
                MULT(q[1], p[1], p[3], t2);
                MULT(q[2], p[0], p[3], t3);
                q[3] = p[3];
#else
                q[0] = p[3];
                MULT(q[1], p[0], p[3], t1);
                MULT(q[2], p[1], p[3], t2);
                MULT(q[3], p[2], p[3], t3);
#endif

                p += 4;
                q += 4;
            }
#undef MULT
        }

        gdk_pixels += gdk_rowstride;
        cairo_pixels += cairo_stride;
    }

    cairo_surface_mark_dirty(surface);
    return surface;
}

static void show_buffer(cam_t *cam)
{
    GtkWidget *widget;
    GdkWindow *window;
    cairo_surface_t *surface;
    cairo_t *cr;
    const GdkRectangle rect = {
        .x = 0, .y = 0,
        .width = cam->width, .height = cam->height
    };

    if (!cam->pb)
        return;

    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "da"));
    window = gtk_widget_get_window(widget);

    surface = create_from_pixbuf(cam->pb, window);
    cr = gdk_cairo_create(window);
    cairo_set_source_surface(cr, surface, 0, 0);

    gdk_cairo_rectangle(cr, &rect);

    if (cam->scale != 1.f)
        cairo_scale(cr, cam->scale, cam->scale);

    cairo_fill(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    frames++;
    frames2++;
}
#else
/*
 * GTK 3 way: use a drawing callback
 */
void draw_callback(GtkWidget *widget, cairo_t *cr, cam_t *cam)
{
    GdkWindow *window;
    cairo_surface_t *surface;
    const GdkRectangle rect = {
        .x = 0, .y = 0,
        .width = cam->width, .height = cam->height
    };

    if (!cam->pb)
        return;

    window = gtk_widget_get_window(widget);
    surface = gdk_cairo_surface_create_from_pixbuf(cam->pb, 1, window);
    cairo_set_source_surface(cr, surface, 0, 0);
    gdk_cairo_rectangle(cr, &rect);
    cairo_fill(cr);
    cairo_surface_destroy(surface);

    frames++;
    frames2++;
}

static inline void show_buffer(cam_t *cam)
{
    gtk_widget_queue_draw(GTK_WIDGET(gtk_builder_get_object(cam->xml, "da")));
}
#endif

/*
* get image from cam - does all the work ;)
*/
gint timeout_func(cam_t *cam)
{
    unsigned char *pic_buf = cam->pic_buf;

    if (cam->read)
        v4l2_read(cam->dev, cam->pic_buf,
                 (cam->width * cam->height * cam->bpp / 8));
    else
        capture_buffers(cam, cam->pic_buf,
                        cam->width * cam->height * cam->bytesperline);

    if (cam->pixformat == V4L2_PIX_FMT_YUV420) {
        yuv420p_to_rgb(cam->pic_buf, cam->tmp, cam->width, cam->height,
                       cam->bpp / 8);
        pic_buf = cam->tmp;
    }
    apply_filters(cam, pic_buf);
    cam->pb = gdk_pixbuf_new_from_data(pic_buf, GDK_COLORSPACE_RGB, FALSE, 8,
                                       cam->width, cam->height,
                                       (cam->width * cam->bpp / 8),
                                       NULL, NULL);

    show_buffer(cam);

    return TRUE;
}

gint fps(GtkWidget *sb)
{
    gchar *stat;
    guint cont = gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), "context");

    seconds++;
    stat = g_strdup_printf(_("%.2f fps - current     %.2f fps - average"),
                           frames / 2., frames2 / (seconds * 2.));
    frames = 0;
    gtk_statusbar_push(GTK_STATUSBAR(sb), cont, stat);
    g_free(stat);
    return 1;
}

void on_status_show(GtkWidget *sb, cam_t *cam)
{
    cam->status = sb;
}

void capture_func(GtkWidget *widget, cam_t *cam)
{
    if (cam->debug == TRUE)
        printf("capture_func\nx = %d, y = %d, depth = %d, realloc size = %d\n",
               cam->width, cam->height, cam->bpp,
               (cam->width * cam->height * cam->bpp / 8));

    memcpy(cam->tmp, cam->pic_buf,
           cam->width * cam->height * cam->bpp / 8);

    if (cam->rcap == TRUE)
        remote_save(cam);

    if (cam->cap == TRUE)
        local_save(cam);
}

gint timeout_capture_func(cam_t *cam)
{
    /* need to return true, or the timeout will be destroyed - don't forget! :) */
    if (cam->hidden == TRUE) {
        /*
         * call timeout_func to get a new picture.   stupid, but it works.
         * also need to add this to capture_func
         * maybe add a "window_state_event" handler to do the same when
         * window is iconified
         */
        timeout_func(cam);
        timeout_func(cam);
        timeout_func(cam);
        timeout_func(cam);

    }
    memcpy(cam->tmp, cam->pic_buf,
           cam->width * cam->height * cam->bpp / 8);

    if (cam->cap == TRUE)
        local_save(cam);

    if (cam->rcap == TRUE)
        remote_save(cam);

    return 1;
}

void contrast_change(GtkScale *sc1, cam_t *cam)
{

    cam->contrast = 256 * (int)gtk_range_get_value((GtkRange *) sc1);
    v4l2_set_control(cam->dev, V4L2_CID_CONTRAST, cam->contrast);
}

void brightness_change(GtkScale *sc1, cam_t *cam)
{

    cam->brightness = 256 * (int)gtk_range_get_value((GtkRange *) sc1);
    v4l2_set_control(cam->dev, V4L2_CID_BRIGHTNESS, cam->brightness);
}

void colour_change(GtkScale *sc1, cam_t *cam)
{

    cam->colour = 256 * (int)gtk_range_get_value((GtkRange *) sc1);
    v4l2_set_control(cam->dev, V4L2_CID_SATURATION, cam->colour);
}

void hue_change(GtkScale *sc1, cam_t *cam)
{

    cam->hue = 256 * (int)gtk_range_get_value((GtkRange *) sc1);
    v4l2_set_control(cam->dev, V4L2_CID_HUE, cam->hue);
}

void wb_change(GtkScale *sc1, cam_t *cam)
{

    cam->whiteness = 256 * (int)gtk_range_get_value((GtkRange *) sc1);
    v4l2_set_control(cam->dev, V4L2_CID_WHITENESS, cam->whiteness);
}

/*
 * Video device selection
 */

unsigned int n_devices = 0, n_valid_devices = 0;
struct devnodes *devices = NULL;

static int handle_video_devs(const char *file,
                             const struct stat *st,
                             int flag)
{
    int dev_minor, first_device = -1, fd;
    unsigned int i;
    struct v4l2_capability vid_cap = { 0 };


    /* Discard  devices that can't be a videodev */
    if (!S_ISCHR(st->st_mode) || major(st->st_rdev) != 81)
        return 0;

    dev_minor = minor(st->st_rdev);

    /* check if it is an already existing device */
    if (devices) {
        for (i = 0; i < n_devices; i++) {
            if (dev_minor == devices[i].minor) {
                first_device = i;
                break;
            }
        }
    }

    devices = realloc(devices, (n_devices + 1) * sizeof(struct devnodes));
    if (!devices) {
        char *msg = g_strdup_printf(_("Can't allocate memory to store devices"));
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }
    memset(&devices[n_devices], 0, sizeof(struct devnodes));

    if (first_device < 0) {
        fd = v4l2_open(file, O_RDWR);
        if (fd < 0) {
            devices[n_devices].is_valid = FALSE;
        } else {
            if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &vid_cap) == -1) {
                devices[n_devices].is_valid = FALSE;
            } else if (!(vid_cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
                devices[n_devices].is_valid = FALSE;
            } else {
                n_valid_devices++;
                devices[n_devices].is_valid = TRUE;
            }
        }

        v4l2_close(fd);
    } else {
        devices[n_devices].is_valid = devices[first_device].is_valid;
    }

    devices[n_devices].fname = g_strdup(file);
    devices[n_devices].minor = dev_minor;
    n_devices++;

    return(0);
}

/*
 * Sort order:
 *
 *  - Valid devices comes first
 *  - Lowest minors comes first
 *
 * For devnode names, it sorts on this order:
 *  - custom udev given names
 *  - /dev/v4l/by-id/
 *  - /dev/v4l/by-path/
 *  - /dev/video
 *  - /dev/char/
 *
 *  - Device name is sorted alphabetically if follows same pattern
 */
static int sort_devices(const void *__a, const void *__b)
{
    const struct devnodes *a = __a;
    const struct devnodes *b = __b;
    int val_a, val_b;

    if (a->is_valid != b->is_valid)
        return !a->is_valid - !b->is_valid;

    if (a->minor != b->minor)
        return a->minor - b->minor;

    /* Ensure that /dev/video* devices will stay at the top */

    if (strstr(a->fname, "by-id"))
        val_a = 1;
    if (strstr(a->fname, "by-path"))
        val_a = 2;
    else if (strstr(a->fname, "/dev/video"))
        val_a = 3;
    else if (strstr(a->fname, "char"))
        val_a = 4;
    else    /* Customized names comes first */
        val_a = 0;

    if (strstr(b->fname, "by-id"))
        val_b = 1;
    if (strstr(b->fname, "by-path"))
        val_b = 2;
    else if (strstr(b->fname, "/dev/video"))
        val_b = 3;
    else if (strstr(b->fname, "char"))
        val_b = 4;
    else   /* Customized names comes first */
        val_b = 0;

    if (val_a != val_b)
        return val_a - val_b;

    /* Finally, just use alphabetic order */
    return strcmp(a->fname, b->fname);
}

static void videodev_response(GtkDialog *dialog,
                              cam_t *cam)
{
    GtkWidget *widget;

    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "videodev_window"));
    gtk_widget_hide(widget);
}

void retrieve_video_dev(cam_t *cam)
{
    int last_minor = -1;
    unsigned int i;
    GtkWidget *widget;

    /* This function is not meant to be called more than once */
    assert (n_devices == 0);

    /* Get all video devices */
    if (ftw("/dev", handle_video_devs, 4) || !n_devices) {
        char *msg = g_strdup_printf(_("Didn't find any camera"));
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }
    qsort(devices, n_devices, sizeof(struct devnodes), sort_devices);

    if (cam->debug)
        for (i = 0; i < n_devices; i++)
            printf("Device[%d]: %s (%s)\n", i, devices[i].fname,
                   devices[i].is_valid ? "OK" : "NOK");

    /* While we have Gtk 2 support, should be aligned with select_video_dev() */
    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "videodev_combo"));
    for (i = 0; i < n_devices; i++) {
        if (devices[i].is_valid) {
            if (devices[i].minor == last_minor)
                continue;
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget),
                                           devices[i].fname);
            last_minor = devices[i].minor;
        }
    }

    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "videodev_ok"));
    g_signal_connect(G_OBJECT(widget), "clicked",
                     G_CALLBACK(videodev_response), cam);
}

int select_video_dev(cam_t *cam)
{
    GtkWidget *window, *widget;
    int ret;
#if GTK_MAJOR_VERSION < 3
    unsigned int i;
    int index, last_minor = -1, p = 0;
#endif

    /* Only ask if there are multiple cameras */
    if (n_valid_devices == 1) {
        cam->video_dev = devices[0].fname;
        return 0;
    }

    /* There are multiple devices. Ask the user */

    window = GTK_WIDGET(gtk_builder_get_object(cam->xml, "videodev_window"));
    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "videodev_combo"));

    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);

    gtk_widget_show(window);

    ret = gtk_dialog_run(GTK_DIALOG(window));

#if GTK_MAJOR_VERSION >= 3
    cam->video_dev = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
#else
    /*
     * For whatever weird reason, gtk_combo_box_text_get_active_text()
     * is not work with Gtk 2.24. We might implement a map, but, as we'll
     * probably drop support for it in the future, better to use the more
     * optimized way for Gtk 3 and Gtk 4.
     */

    index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

    /* Should be aligned with the similar logic at retrieve_video_dev() */
    for (i = 0; i < n_devices; i++) {
        if (devices[i].is_valid) {
            if (devices[i].minor == last_minor)
                continue;
            if (p == index)
                break;
            last_minor = devices[i].minor;
            p++;
        }
    }
    if (i < n_devices)
        cam->video_dev = devices[i].fname;
#endif

    gtk_widget_hide(window);
    return ret;
}

void on_change_camera(GtkWidget *widget, cam_t *cam)
{
    gchar *old_cam;

    old_cam = g_strdup(cam->video_dev);
    select_video_dev(cam);

    /* Trivial case: nothing changed */
    if (!strcmp(cam->video_dev, old_cam)) {
        g_free(old_cam);
        return;
    }
    g_free(old_cam);

    start_camera(cam);
}

static void add_gtk_view_resolutions(cam_t *cam)
{
    GtkWidget *small_res, *new_res;
    unsigned int i;

    /*
     * Dynamically generate the resolutions based on what the camera
     * actually supports. Provide a fallback method, if the camera driver
     * is too old and doesn't support formats enumeration.
     */

    small_res = GTK_WIDGET(gtk_builder_get_object(cam->xml, "small"));

    /* Get all supported resolutions by cam->pixformat */
    get_supported_resolutions(cam);

    if (cam->n_res > 0) {
        for (i = 0; i < cam->n_res; i++) {
            char name[80];

            if (cam->res[i].max_fps > 0)
                    sprintf(name, _("%dx%d (max %.1f fps)"),
                            cam->res[i].x, cam->res[i].y,
                            (double)cam->res[i].max_fps);
                else
                    sprintf(name, _("%dx%d"), cam->res[i].x, cam->res[i].y);

            new_res = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(small_res), name);
            gtk_container_add(GTK_CONTAINER(GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"))),
                              new_res);
            gtk_widget_show(new_res);
            g_signal_connect(new_res, "activate",
                             G_CALLBACK(on_change_size_activate), cam);
            gtk_widget_set_name(new_res, name);

            if (cam->width == cam->res[i].x && cam->height == cam->res[i].y)
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(new_res),
                                               TRUE);
            else
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(new_res),
                                               FALSE);
        }

        /* We won't actually use the small res */
        gtk_widget_hide(small_res);
    } else {
        g_signal_connect(gtk_builder_get_object(cam->xml, "small"),
                         "activate", G_CALLBACK(on_change_size_activate),
                         cam);

        new_res = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(small_res),
                                                                 "Medium");
        gtk_container_add(GTK_CONTAINER(GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"))),
                          new_res);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(new_res),
                                       FALSE);
        gtk_widget_show(new_res);
        g_signal_connect(new_res, "activate",
                         G_CALLBACK(on_change_size_activate), cam);
        gtk_widget_set_name(new_res, "medium");

        new_res = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(small_res),
                                                                 "Large");
        gtk_container_add(GTK_CONTAINER(GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"))),
                          new_res);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(new_res),
                                       FALSE);
        gtk_widget_show(new_res);
        g_signal_connect(new_res, "activate",
                         G_CALLBACK(on_change_size_activate), cam);
        gtk_widget_set_name(new_res, "large");
    }
}

void start_camera(cam_t *cam)
{
    unsigned int bufsize;
    GList *children, *iter;
    GtkWidget *widget, *container;

    /* First step: free used resources, if any */

    if (cam->idle_id) {
        g_source_remove(cam->idle_id);
        cam->idle_id = 0;
    }

    if (cam->dev >= 0) {
        if (cam->read == FALSE)
            stop_streaming(cam);

        cam->pb = NULL;

        v4l2_close(cam->dev);
    }

    if (cam->timeout_id) {
        g_source_remove(cam->timeout_id);
        cam->timeout_id = 0;
    }

    if (cam->pic_buf) {
        free(cam->pic_buf);
        cam->pic_buf = NULL;
    }

    if (cam->tmp) {
        free(cam->tmp);
        cam->tmp = NULL;
    }

    /* Second step: clean-up all resolutions */

    container = GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"));
    children = gtk_container_get_children(GTK_CONTAINER(container));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        widget = GTK_WIDGET(iter->data);
        if (strstr(gtk_widget_get_name(widget), "x"))
            gtk_widget_destroy(widget);
    }

    /* Third step: allocate them again */

    if (cam->read)
        cam->dev = v4l2_open(cam->video_dev, O_RDWR);
    else
        cam->dev = v4l2_open(cam->video_dev, O_RDWR | O_NONBLOCK);

    if (camera_cap(cam))
        exit(-1);

    get_win_info(cam);

    set_win_info(cam);
    get_win_info(cam);

    /* get picture attributes */
    get_pic_info(cam);

    /* Only store the device name after being able to successfully use it */
    g_settings_set_string(cam->gc, CAM_SETTINGS_DEVICE, cam->video_dev);

    bufsize = cam->max_width * cam->max_height * cam->bpp / 8;
    cam->pic_buf = malloc(bufsize);
    cam->tmp = malloc(bufsize);

    if (!cam->pic_buf || !cam->tmp) {
        printf("Failed to allocate memory for buffers\n");
        exit(0);
    }

    if (cam->read == FALSE)
        start_streaming(cam);
    else
        printf("using read()\n");

    cam->idle_id = g_idle_add((GSourceFunc) timeout_func, (gpointer) cam);

    if (cam->debug == TRUE)
        print_cam(cam);

    /* Add resolutions */

    add_gtk_view_resolutions(cam);
}
