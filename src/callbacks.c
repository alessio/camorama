#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "filter.h"
#include <glib/gi18n.h>
#include <config.h>
#include <pthread.h>
#include <libv4l2.h>

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

extern GtkWidget *main_window, *prefswindow;
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
    gconf_client_set_bool(cam->gc, KEY4, cam->timestamp, NULL);
}

void customstring_func(GtkWidget *rb, cam_t *cam)
{
    cam->usestring = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    gconf_client_set_bool(cam->gc, KEY18, cam->usestring, NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(cam->xml, "string_entry")),
                             cam->usestring);
}

void drawdate_func(GtkWidget *rb, cam_t *cam)
{
    cam->usedate = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    gconf_client_set_bool(cam->gc, KEY19, cam->usedate, NULL);
}

void append_func(GtkWidget *rb, cam_t *cam)
{
    cam->timefn = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    gconf_client_set_bool(cam->gc, KEY14, cam->timefn, NULL);

}

void rappend_func(GtkWidget *rb, cam_t *cam)
{
    cam->rtimefn = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    gconf_client_set_bool(cam->gc, KEY15, cam->rtimefn, NULL);

}

void jpg_func(GtkWidget *rb, cam_t *cam)
{
    cam->savetype = JPEG;
    gconf_client_set_int(cam->gc, KEY3, cam->savetype, NULL);

}

void png_func(GtkWidget *rb, cam_t *cam)
{
    cam->savetype = PNG;
    gconf_client_set_int(cam->gc, KEY3, cam->savetype, NULL);
}

void ppm_func(GtkWidget *rb, cam_t *cam)
{
    cam->savetype = PPM;
    gconf_client_set_int(cam->gc, KEY3, cam->savetype, NULL);
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

    gconf_client_set_bool(cam->gc, KEY12, cam->cap, NULL);
    set_sensitive(cam);
}

void rcap_func(GtkWidget *rb, cam_t *cam)
{
    cam->rcap = gtk_toggle_button_get_active((GtkToggleButton *) rb);

    gconf_client_set_bool(cam->gc, KEY13, cam->rcap, NULL);
    set_sensitive(cam);

}

void acap_func(GtkWidget *rb, cam_t *cam)
{
    cam->acap = gtk_toggle_button_get_active((GtkToggleButton *) rb);

    gconf_client_set_bool(cam->gc, KEY20, cam->acap, NULL);

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
    gconf_client_set_int(cam->gc, KEY21, cam->timeout_interval, NULL);
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
    gconf_client_set_int(cam->gc, KEY10, cam->rsavetype, NULL);

}

void rpng_func(GtkWidget *rb, cam_t *cam)
{
    cam->rsavetype = PNG;
    gconf_client_set_int(cam->gc, KEY10, cam->rsavetype, NULL);
}

void rppm_func(GtkWidget *rb, cam_t *cam)
{
    cam->rsavetype = PPM;
    gconf_client_set_int(cam->gc, KEY10, cam->rsavetype, NULL);
}

void rts_func(GtkWidget *rb, cam_t *cam)
{
    cam->rtimestamp = gtk_toggle_button_get_active((GtkToggleButton *) rb);
    gconf_client_set_bool(cam->gc, KEY11, cam->rtimestamp, NULL);

}

void gconf_notify_func(GConfClient *client, guint cnxn_id, GConfEntry *entry,
                       char **str)
{
    GConfValue *value;

    value = gconf_entry_get_value(entry);
    *str = g_strdup(gconf_value_get_string(value));

}

void gconf_notify_func_bool(GConfClient *client, guint cnxn_id,
                            GConfEntry *entry, gboolean *val)
{
    GConfValue *value;

    value = gconf_entry_get_value(entry);
    *val = gconf_value_get_bool(value);

}

void gconf_notify_func_int(GConfClient *client, guint cnxn_id,
                           GConfEntry *entry, int *val)
{
    GConfValue *value;

    value = gconf_entry_get_value(entry);
    *val = gconf_value_get_int(value);

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
        cam->pixdir = g_strdup(gtk_file_chooser_get_current_folder((GtkFileChooser *) dentry));
        gconf_client_set_string(cam->gc, KEY1, cam->pixdir, NULL);
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
        cam->capturefile = g_strdup(gtk_entry_get_text((GtkEntry *) entry2));
        gconf_client_set_string(cam->gc, KEY2, cam->capturefile, NULL);
    }

    if (strlen(gtk_entry_get_text((GtkEntry *) string_entry)) > 0) {
        cam->ts_string = g_strdup(gtk_entry_get_text((GtkEntry *) string_entry));
        gconf_client_set_string(cam->gc, KEY16, cam->ts_string, NULL);
    }
    if (cam->debug == TRUE) {
        fprintf(stderr, "dir now = %s\nfile now = %s\n", cam->pixdir,
                cam->capturefile);
    }
    gtk_widget_hide(prefswindow);

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
    gconf_client_set_bool(cam->gc, KEY22, cam->show_adjustments, NULL);
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

    gconf_client_set_bool(cam->gc, KEY23, cam->show_effects, NULL);
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
gint read_timeout_func(cam_t *cam)
{
    unsigned char *pic_buf = cam->pic_buf;

    v4l2_read(cam->dev, cam->pic_buf,
              (cam->width * cam->height * cam->bpp / 8));

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

gint timeout_func(cam_t *cam)
{
    unsigned char *pic_buf = cam->pic_buf;

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
        pt2Function(cam);
        pt2Function(cam);
        pt2Function(cam);
        pt2Function(cam);

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
