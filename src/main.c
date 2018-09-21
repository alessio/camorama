#include "interface.h"

#include "callbacks.h"
#include "filter.h"
#include "camorama-window.h"
#include "camorama-globals.h"
#include "support.h"
#include <config.h>

#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <libv4l2.h>
#include <stdlib.h>

static int ver = 0, max = 0, min;
static int half = 0, use_read = 0, debug = 0;
static gchar *video_dev = NULL;
static int x = 0, y = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

static GOptionEntry options[] = {
    {"version", 'V', 0, G_OPTION_ARG_NONE, &ver,
     N_("show version and exit"), NULL},
    {"device", 'd', 0, G_OPTION_ARG_STRING, &video_dev,
     N_("v4l device to use"), NULL},
    {"debug", 'D', 0, G_OPTION_ARG_NONE, &debug,
     N_("enable debugging code"), NULL},
    {"width", 'x', 0, G_OPTION_ARG_INT, &x,
     N_("capture width"), NULL},
    {"height", 'y', 0, G_OPTION_ARG_INT, &y,
     N_("capture height"), NULL},
    {"max", 'M', 0, G_OPTION_ARG_NONE, &max,
     N_("maximum capture size"), NULL},
    {"min", 'm', 0, G_OPTION_ARG_NONE, &min,
     N_("minimum capture size"), NULL},
    {"half", 'H', 0, G_OPTION_ARG_NONE, &half,
     N_("middle capture size"), NULL},
    {"read", 'R', 0, G_OPTION_ARG_NONE, &use_read,
     N_("use read() rather than mmap()"), NULL},
    {NULL}
};

#pragma GCC diagnostic pop

static void get_geometry(cam_t *cam)
{
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 22
    GdkRectangle geo;

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor(display, 0);
    gdk_monitor_get_geometry(monitor, &geo);

    cam->screen_width  = geo.width;
    cam->screen_height = geo.height;
#else
    cam->screen_width  = gdk_screen_width();
    cam->screen_height = gdk_screen_height();
#endif
}

static void close_app(GtkWidget* widget, cam_t *cam)
{
    if (cam->idle_id)
        g_source_remove(cam->idle_id);

    if (cam->read == FALSE)
        stop_streaming(cam);

    v4l2_close(cam->dev);

    if (cam->timeout_id)
        g_source_remove(cam->timeout_id);

    if (cam->timeout_fps_id)
        g_source_remove(cam->timeout_fps_id);

    gtk_widget_destroy(widget);

    g_free(cam->video_dev);
    g_free(cam->pixdir);
    g_free(cam->host);
    g_free(cam->proto);
    g_free(cam->rdir);
    g_free(cam->rcapturefile);
    g_free(cam->ts_string);
    free(cam->pic_buf);
    free(cam->tmp);

    g_object_unref (G_OBJECT (cam->xml));
    g_object_unref (G_OBJECT (cam->gc));

#if GTK_MAJOR_VERSION < 3
    gtk_main_quit();
#endif
}

static cam_t cam_object = { 0 };

#if GTK_MAJOR_VERSION < 3
static void activate(void)
#else
static void activate(GtkApplication *app)
#endif
{
    cam_t *cam = &cam_object;
    GtkWidget *widget, *window;
    unsigned int bufsize, i;

    /* set some default values */
    cam->frame_number = 0;
    cam->size = PICHALF;
    cam->video_dev = NULL;
    cam->read = FALSE;
    cam->width = 0;
    cam->height = 0;
    cam->res = NULL;
    cam->n_res = 0;
    cam->scale = 1.f;
#if GTK_MAJOR_VERSION >= 3
    cam->app = app;
#endif

    /* gtk is initialized now */
    camorama_filters_init();

    cam->debug = debug;

    get_geometry(cam);

    if (ver) {
        fprintf(stderr, _("\n\nCamorama version %s\n\n"), PACKAGE_VERSION);
        exit(0);
    }
    if (max)
        cam->size = PICMAX;

    if (min)
        cam->size = PICMIN;

    if (half)
        cam->size = PICHALF;

    if (use_read) {
        printf("Forcing read mode\n");
        cam->read = TRUE;
    }

    cam->xml = gtk_builder_new();

    if (!gtk_builder_add_from_file(cam->xml,
                                   PACKAGE_DATA_DIR "/camorama/" CAMORAMA_UI,
                                   NULL)) {
        error_dialog(_("Couldn't load builder file"));
        exit(1);
    }

    retrieve_video_dev(cam);

    cam->gc = g_settings_new(CAM_SETTINGS_SCHEMA);

    if (!video_dev) {
        gchar const *gconf_device = g_settings_get_string(cam->gc,
                                                          CAM_SETTINGS_DEVICE);
        if (gconf_device)
            cam->video_dev = g_strdup(gconf_device);
        else
            cam->video_dev = NULL;
    } else {
        cam->video_dev = g_strdup(video_dev);
    }

    if (cam->video_dev) {
        for (i = 0; i < n_devices; i++)
            if (!strcmp(cam->video_dev, devices[i].fname) && devices[i].is_valid)
                break;

        /* Not found, or doesn't work. Falling back to the first device */
        if (i == n_devices) {
            char *msg;

            if (n_valid_devices == 1)
                msg = g_strdup_printf(_("%s not found. Falling back to %s"),
                                     cam->video_dev, devices[0].fname);
            else
                msg = g_strdup_printf(_("%s not found."),
                                     cam->video_dev);
            error_dialog(msg);
            g_free(msg);

            /* Ask user or get the only one device, if it is the case */
            select_video_dev(cam);
        }
    } else {
        cam->video_dev = devices[0].fname;
    }

    if (cam->debug)
        printf("Using videodev: %s\n", cam->video_dev);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    cam->date_format = "%Y-%m-%d %H:%M:%S";
#pragma GCC diagnostic pop

    cam->pixdir = g_settings_get_string(cam->gc, CAM_SETTINGS_SAVE_DIR);
    cam->capturefile = g_settings_get_string(cam->gc, CAM_SETTINGS_SAVE_FILE);
    cam->savetype = g_settings_get_int(cam->gc, CAM_SETTINGS_FILE_TYPE);
    cam->host = g_settings_get_string(cam->gc, CAM_SETTINGS_HOSTNAME);
    cam->proto = g_settings_get_string(cam->gc, CAM_SETTINGS_REMOTE_PROTO);
    cam->rdir = g_settings_get_string(cam->gc, CAM_SETTINGS_REMOTE_SAVE_DIR);
    cam->rcapturefile = g_settings_get_string(cam->gc,
                                              CAM_SETTINGS_REMOTE_SAVE_FILE);
    cam->rsavetype = g_settings_get_int(cam->gc,
                                        CAM_SETTINGS_REMOTE_FILE_TYPE);
    cam->ts_string = g_settings_get_string(cam->gc,
                                           CAM_SETTINGS_TIMESTAMP_STRING);
    cam->timestamp = g_settings_get_boolean(cam->gc, CAM_SETTINGS_TIMESTAMP);
    cam->rtimestamp = g_settings_get_boolean(cam->gc, CAM_SETTINGS_REMOTE_TIMESTAMP);

    cam->cap = g_settings_get_boolean(cam->gc, CAM_SETTINGS_LOCAL_CAPTURE);
    cam->rcap = g_settings_get_boolean(cam->gc, CAM_SETTINGS_REMOTE_CAPTURE);
    cam->timefn = g_settings_get_boolean(cam->gc, CAM_SETTINGS_LOCAL_APPEND_TS);
    cam->rtimefn = g_settings_get_boolean(cam->gc,
                                         CAM_SETTINGS_REMOTE_APPEND_TS);
    cam->usestring = g_settings_get_boolean(cam->gc,
                                           CAM_SETTINGS_USE_CUSTOM_STRING);
    cam->usedate = g_settings_get_boolean(cam->gc, CAM_SETTINGS_DRAWDATE);
    cam->acap = g_settings_get_boolean(cam->gc, CAM_SETTINGS_AUTO_CAPTURE);
    cam->timeout_interval = g_settings_get_int(cam->gc,
                                               CAM_SETTINGS_AUTO_CAPTURE_INTERVAL);
    cam->show_adjustments = g_settings_get_boolean(cam->gc,
                                                   CAM_SETTINGS_SHOW_ADJUSTMENTS);
    cam->show_effects = g_settings_get_boolean(cam->gc,
                                               CAM_SETTINGS_SHOW_EFFECTS);
    if (x)
        cam->width = x;
    else
        cam->width = g_settings_get_int(cam->gc, CAM_SETTINGS_WIDTH);

    if (y)
        cam->height = y;
    else
        cam->height = g_settings_get_int(cam->gc, CAM_SETTINGS_HEIGHT);

    if (use_read)
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

    load_interface(cam);

    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "da"));

#if GTK_MAJOR_VERSION >= 3
    g_signal_connect(G_OBJECT(widget), "draw", G_CALLBACK(draw_callback), cam);
#endif

    window = GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"));

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK (close_app), cam);

    gtk_widget_show(widget);

    cam->idle_id = g_idle_add((GSourceFunc) timeout_func, (gpointer) cam);

    cam->timeout_fps_id = g_timeout_add(2000, (GSourceFunc) fps, cam->status);

    if (cam->debug == TRUE)
        print_cam(cam);
}

int main(int argc, char *argv[])
{
#if GTK_MAJOR_VERSION >= 3
    GtkApplication *app;
    gint status;
#else
    GError *error = NULL;
#endif

    bindtextdomain(PACKAGE_NAME, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    textdomain(PACKAGE_NAME);

#if GTK_MAJOR_VERSION >= 3
    app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
    g_application_add_main_option_entries(G_APPLICATION(app), options);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
#else
    if (!gtk_init_with_args(&argc, &argv, _("camorama"), options,
                            PACKAGE_NAME, &error) || error) {
        g_printerr(_("Invalid argument\nRun '%s --help'\n"), argv[0]);
        return 1;
    }
    activate();

    gtk_main();

    return 0;
#endif
}
