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

static int ver = 0, max = 0, min;
static int half = 0, use_read = 0, buggery = 0;
static gchar *poopoo = NULL;
static int x = 0, y = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

static GOptionEntry options[] = {
    {"version", 'V', 0, G_OPTION_ARG_NONE, &ver,
     N_("show version and exit"), NULL},
    {"device", 'd', 0, G_OPTION_ARG_STRING, &poopoo,
     N_("v4l device to use"), NULL},
    {"debug", 'D', 0, G_OPTION_ARG_NONE, &buggery,
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
    GConfClient *gc;
    GtkWidget *widget, *window;
    unsigned int bufsize;

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

    cam->debug = buggery;

    cam->width = x;
    cam->height = y;

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
    gc = gconf_client_get_default();
    cam->gc = gc;

    gconf_client_add_dir(cam->gc, PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
    gconf_client_notify_add(cam->gc, KEY1, (void *)gconf_notify_func,
                            &cam->pixdir, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY2, (void *)gconf_notify_func,
                            &cam->capturefile, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY3,
                            (void *)gconf_notify_func_int,
                            &cam->savetype, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY4,
                            (void *)gconf_notify_func_bool,
                            &cam->timestamp, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY5, (void *)gconf_notify_func,
                            &cam->host, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY6, (void *)gconf_notify_func,
                            &cam->proto, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY8, (void *)gconf_notify_func,
                            &cam->rdir, NULL, NULL);
    gconf_client_notify_add(cam->gc, KEY9, (void *)gconf_notify_func,
                            &cam->rcapturefile, NULL, NULL);

    if (!poopoo) {
        gchar const *gconf_device = gconf_client_get_string(cam->gc, KEY_DEVICE,
                                                            NULL);
        if (gconf_device)
            cam->video_dev = g_strdup(gconf_device);
        else
            cam->video_dev = g_strdup("/dev/video0");
    } else {
        cam->video_dev = g_strdup(poopoo);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    cam->date_format = "%Y-%m-%d %H:%M:%S";
#pragma GCC diagnostic pop

    cam->pixdir = gconf_client_get_string(cam->gc, KEY1, NULL);
    cam->capturefile = gconf_client_get_string(cam->gc, KEY2, NULL);
    cam->savetype = gconf_client_get_int(cam->gc, KEY3, NULL);
    cam->host = gconf_client_get_string(cam->gc, KEY5, NULL);
    cam->proto = gconf_client_get_string(cam->gc, KEY6, NULL);
    cam->rdir = gconf_client_get_string(cam->gc, KEY8, NULL);
    cam->rcapturefile = gconf_client_get_string(cam->gc, KEY9, NULL);
    cam->rsavetype = gconf_client_get_int(cam->gc, KEY10, NULL);
    cam->ts_string = gconf_client_get_string(cam->gc, KEY16, NULL);
    cam->timestamp = gconf_client_get_bool(cam->gc, KEY4, NULL);
    cam->rtimestamp = gconf_client_get_bool(cam->gc, KEY11, NULL);

    cam->cap = gconf_client_get_bool(cam->gc, KEY12, NULL);
    cam->rcap = gconf_client_get_bool(cam->gc, KEY13, NULL);
    cam->timefn = gconf_client_get_bool(cam->gc, KEY14, NULL);
    cam->rtimefn = gconf_client_get_bool(cam->gc, KEY15, NULL);
    cam->usestring = gconf_client_get_bool(cam->gc, KEY18, NULL);
    cam->usedate = gconf_client_get_bool(cam->gc, KEY19, NULL);
    cam->acap = gconf_client_get_bool(cam->gc, KEY20, NULL);
    cam->timeout_interval = gconf_client_get_int(cam->gc, KEY21, NULL);
    cam->show_adjustments = gconf_client_get_bool(cam->gc, KEY22, NULL);
    cam->show_effects = gconf_client_get_bool(cam->gc, KEY23, NULL);

    if (use_read)
        cam->dev = v4l2_open(cam->video_dev, O_RDWR);
    else
        cam->dev = v4l2_open(cam->video_dev, O_RDWR | O_NONBLOCK);

    camera_cap(cam);
    get_win_info(cam);

    set_win_info(cam);
    get_win_info(cam);

    /* get picture attributes */
    get_pic_info(cam);

    bufsize = cam->max_width * cam->max_height * cam->bpp / 8;
    cam->pic_buf = malloc(bufsize);
    cam->tmp = malloc(bufsize);

    if (!cam->pic_buf || !cam->tmp) {
        printf("Failed to allocate memory for buffers\n");
        exit(0);
    }
    //cam->read = FALSE;
    /* initialize cam and create the window */

    if (cam->read == FALSE) {
        pt2Function = timeout_func;
        start_streaming(cam);
    } else {
        printf("using read()\n");
        pt2Function = read_timeout_func;
    }

    cam->xml = gtk_builder_new();

    if (!gtk_builder_add_from_file(cam->xml,
                                   PACKAGE_DATA_DIR "/camorama/" CAMORAMA_UI,
                                   NULL)) {
        error_dialog(_("Couldn't load builder file"));
        exit(1);
    }

    load_interface(cam);

    widget = GTK_WIDGET(gtk_builder_get_object(cam->xml, "da"));

#if GTK_MAJOR_VERSION >= 3
    g_signal_connect(G_OBJECT(widget), "draw", G_CALLBACK(draw_callback), cam);
#endif

    window = GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window"));

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK (close_app), cam);

    gtk_widget_show(widget);

    cam->idle_id = g_idle_add((GSourceFunc) pt2Function, (gpointer) cam);

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
