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

#include "camorama-stock-items.h"

static int ver = 0, max = 0, min = 0;
static int half = 0, use_read = 0, buggery = 0;
static gchar *poopoo = NULL;
static int x = -1, y = -1;


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
    { NULL }
};
#pragma GCC diagnostic pop

int
main(int argc, char *argv[]) {
    cam_t cam_object, *cam;
    GConfClient *gc;
    GtkWidget *widget;
    unsigned int bufsize;
    GError *error = NULL;

    cam = &cam_object;

    /* set some default values */
    cam->frame_number = 0;
    cam->size = PICHALF;
    cam->video_dev = NULL;
    cam->read = FALSE;
    cam->width = 0;
    cam->height = 0;
    cam->res = NULL;
    cam->n_res = 0;

    bindtextdomain (PACKAGE_NAME, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (PACKAGE_NAME, "UTF-8");
    textdomain (PACKAGE_NAME);

    if (!gtk_init_with_args(&argc, &argv,_("camorama"), options,
                             PACKAGE_NAME, &error) || error) {
        g_printerr(_("Invalid argument\nRun '%s --help'\n"), argv[0]);
        return 1;
    }

    /* gtk is initialized now */
    camorama_stock_init();
    camorama_filters_init();

    cam->debug = buggery;

	cam->width = x;
	cam->height = y;

    if (ver) {
        fprintf (stderr, _("\n\nCamorama version %s\n\n"), PACKAGE_VERSION);
        exit (0);
    }
    if (max) {
        cam->size = PICMAX;
    }
    if (min) {
        cam->size = PICMIN;
    }
    if (half) {
        cam->size = PICHALF;
    }
    if (use_read) {
        printf ("Forcing read mode\n");
        cam->read = TRUE;
    }
    gc = gconf_client_get_default ();
    cam->gc = gc;

    gconf_client_add_dir (cam->gc, PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
    gconf_client_notify_add (cam->gc, KEY1, (void *) gconf_notify_func,
                             cam->pixdir, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY2, (void *) gconf_notify_func,
                             cam->capturefile, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY3,
                             (void *) gconf_notify_func_int,
                             GINT_TO_POINTER (cam->savetype), NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY4,
                             (void *) gconf_notify_func_bool,
                             &cam->timestamp, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY5, (void *) gconf_notify_func,
                             cam->host, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY6, (void *) gconf_notify_func,
                             cam->proto, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY8, (void *) gconf_notify_func,
                             cam->rdir, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY9, (void *) gconf_notify_func,
                             cam->rcapturefile, NULL, NULL);

    if (!poopoo) {
	gchar const* gconf_device = gconf_client_get_string(cam->gc, KEY_DEVICE, NULL);
	if(gconf_device) {
		cam->video_dev = g_strdup(gconf_device);
	} else {
		cam->video_dev = g_strdup ("/dev/video0");
	}
    } else {
        cam->video_dev = g_strdup (poopoo);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    cam->date_format = "%Y-%m-%d %H:%M:%S";
#pragma GCC diagnostic pop

    cam->pixdir = g_strdup (gconf_client_get_string (cam->gc, KEY1, NULL));
    cam->capturefile =
        g_strdup (gconf_client_get_string (cam->gc, KEY2, NULL));
    cam->savetype = gconf_client_get_int (cam->gc, KEY3, NULL);
    cam->host = g_strdup (gconf_client_get_string (cam->gc, KEY5, NULL));
    cam->proto = g_strdup (gconf_client_get_string (cam->gc, KEY6, NULL));
    cam->rdir = g_strdup (gconf_client_get_string (cam->gc, KEY8, NULL));
    cam->rcapturefile = g_strdup (gconf_client_get_string (cam->gc, KEY9, NULL));
    cam->rsavetype = gconf_client_get_int (cam->gc, KEY10, NULL);
    cam->ts_string =
        g_strdup (gconf_client_get_string (cam->gc, KEY16, NULL));
    cam->timestamp = gconf_client_get_bool (cam->gc, KEY4, NULL);
    cam->rtimestamp = gconf_client_get_bool (cam->gc, KEY11, NULL);

    cam->cap = gconf_client_get_bool (cam->gc, KEY12, NULL);
    cam->rcap = gconf_client_get_bool (cam->gc, KEY13, NULL);
    cam->timefn = gconf_client_get_bool (cam->gc, KEY14, NULL);
    cam->rtimefn = gconf_client_get_bool (cam->gc, KEY15, NULL);
    cam->usestring = gconf_client_get_bool (cam->gc, KEY18, NULL);
    cam->usedate = gconf_client_get_bool (cam->gc, KEY19, NULL);
    cam->acap = gconf_client_get_bool (cam->gc, KEY20, NULL);
    cam->timeout_interval = gconf_client_get_int (cam->gc, KEY21, NULL);
    cam->show_adjustments = gconf_client_get_bool (cam->gc, KEY22, NULL);
    cam->show_effects = gconf_client_get_bool (cam->gc, KEY23, NULL);

    if (use_read)
        cam->dev = v4l2_open (cam->video_dev, O_RDWR);
    else
        cam->dev = v4l2_open (cam->video_dev, O_RDWR | O_NONBLOCK);

    camera_cap (cam);
    get_win_info (cam);

    set_win_info (cam);
    get_win_info (cam);

    /* get picture attributes */
    get_pic_info (cam);

    bufsize = cam->max_width * cam->max_height * cam->bpp / 8;
    cam->pic_buf = malloc (bufsize);
    cam->tmp = malloc (bufsize);

    if (!cam->pic_buf || !cam->tmp) {
       printf("Failed to allocate memory for buffers\n");
       exit(0);
    }

    //cam->read = FALSE;
    /* initialize cam and create the window */

    if (cam->read == FALSE) {
        pt2Function = timeout_func;
        start_streaming (cam);
    } else {
        printf ("using read()\n");
        pt2Function = read_timeout_func;
    }

    cam->xml = gtk_builder_new ();
    if (!gtk_builder_add_from_file (cam->xml,
                                    PACKAGE_DATA_DIR "/camorama/camorama.ui",
                                    NULL)) {
	error_dialog (_("Couldn't load builder file"));
        exit(1);
    }

    /*eggtray */

    /*tray_icon = egg_tray_icon_new ("Our other cool tray icon");
     * button = gtk_button_new_with_label ("This is a another\ncool tray icon");
     * g_signal_connect (button, "clicked",
     * G_CALLBACK (second_button_pressed), tray_icon);
     * 
     * gtk_container_add (GTK_CONTAINER (tray_icon), button);
     * gtk_widget_show_all (GTK_WIDGET (tray_icon)); */
    load_interface (cam);

    widget = GTK_WIDGET (gtk_builder_get_object (cam->xml, "da"));
    gtk_widget_show (widget);

    cam->idle_id = g_idle_add ((GSourceFunc) pt2Function, (gpointer) cam);

    g_timeout_add (2000, (GSourceFunc) fps, cam->status);

    if (cam->debug == TRUE)
       print_cam(cam);

    gtk_main ();
    if (cam->read == FALSE) {
       stop_streaming(cam);
    }
    v4l2_close(cam->dev);

    return 0;
}
