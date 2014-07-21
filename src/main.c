#include <gnome.h>
#include "interface.h"

#include "callbacks.h"
#include "support.h"
#include <config.h>

#include <gdk/gdkx.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlibrgb.h>
#include <locale.h>

GtkWidget *main_window, *prefswindow;
state func_state;
int frames, frames2, seconds;
GtkWidget *dentry, *entry2, *string_entry, *format_selection;
GtkWidget *host_entry, *directory_entry, *filename_entry, *login_entry,
    *pw_entry;

static gboolean ver = FALSE, max = FALSE, min = FALSE, half =
    FALSE, use_read = FALSE;

/*static gint tray_icon_destroyed (GtkWidget *tray, gpointer data) 
{
  GConfClient *client = gconf_client_get_default ();

    //Somehow the delete_event never got called, so we use "destroy" 
  if (tray != MyApp->GetMainWindow ()->docklet)
    return true;
  GtkWidget *new_tray = gnomemeeting_init_tray ();

  if (gconf_client_get_bool 
      (client, GENERAL_KEY "do_not_disturb", 0)) 
    gnomemeeting_tray_set_content (new_tray, 2);

  
  MyApp->GetMainWindow ()->docklet = GTK_WIDGET (new_tray);
  gtk_widget_show (gm);

  return true;
}*/

static gint
tray_clicked_callback (GtkWidget * widget, GdkEventButton * event, cam * cam)
{
    GdkEventButton *event_button = NULL;

    if (event->type == GDK_BUTTON_PRESS) {

        event_button = (GdkEventButton *) event;

        //change to switch
        if (event_button->button == 1) {
            if (GTK_WIDGET_VISIBLE
                (glade_xml_get_widget (cam->xml, "window2"))) {
                cam->hidden = TRUE;
				    gtk_idle_remove(cam->idle_id);
						 gtk_widget_hide (glade_xml_get_widget (cam->xml, "window2"));
					
            } else {
                cam->idle_id = gtk_idle_add ((GSourceFunc) pt2Function, (gpointer) cam);
					 gtk_widget_show (glade_xml_get_widget (cam->xml, "window2"));
					 cam->hidden = FALSE;
					
            }
            return TRUE;
        } else if (event_button->button == 3) {

            //gw = MyApp->GetMainWindow ();

            //gnomemeeting_component_view (NULL, (gpointer) gw->ldap_window);

            return TRUE;
        }
    }

    return FALSE;
}

void load_interface (cam * cam)
{
    gchar *title;
    GdkPixbuf *logo = NULL, *scaled = NULL;
    GtkWidget *eventbox = NULL, *image = NULL;
    gint width, height;

    (GtkTooltips *)cam->tooltips = gtk_tooltips_new ();
    logo = (GdkPixbuf *) create_pixbuf (DATADIR "/pixmaps/camorama.png");
    if (logo == NULL) {
        printf ("\n\nLOGO NO GO\n\n");
    }

    gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
    scaled =
        gdk_pixbuf_scale_simple (logo, width, height, GDK_INTERP_BILINEAR);

    image = gtk_image_new_from_pixbuf (scaled);

    //image = gtk_image_new_from_stock (GNOME_STOCK_TRASH, GTK_ICON_SIZE_MENU);

    if (cam->show_adjustments == FALSE) {
        gtk_widget_hide (glade_xml_get_widget (cam->xml, "table6"));
        gtk_widget_hide (glade_xml_get_widget (cam->xml, "vbox37"));
        gtk_window_resize (GTK_WINDOW
                           (glade_xml_get_widget
                            (cam->xml, "window2")), 320, 240);

    }

    eventbox = gtk_event_box_new ();

    gtk_widget_show (image);

    gtk_widget_show (eventbox);

    cam->tray_icon = egg_tray_icon_new ("Our cool tray icon");
    update_tooltip (cam);
    /* add the status to the plug */
    g_object_set_data (G_OBJECT (cam->tray_icon), "image", image);
    g_object_set_data (G_OBJECT (cam->tray_icon), "available",
                       GINT_TO_POINTER (1));
    g_object_set_data (G_OBJECT (cam->tray_icon), "embedded",
                       GINT_TO_POINTER (0));
    gtk_container_add (GTK_CONTAINER (eventbox), image);
    gtk_container_add ((GtkContainer *)cam->tray_icon, eventbox);

    g_signal_connect (G_OBJECT (eventbox), "button_press_event",
                      G_CALLBACK (tray_clicked_callback), cam);

    //button = gtk_button_new_with_label ("This is a cool\ntray icon");
    /*g_signal_connect (button, "clicked",
     * G_CALLBACK (capture_func), cam);
     * gtk_container_add (GTK_CONTAINER (cam->tray_icon), button); */
    gtk_widget_show_all (GTK_WIDGET (cam->tray_icon));

    /* connect the signals in the interface 
     * glade_xml_signal_autoconnect(xml);
     * this won't work, can't pass data to callbacks.  have to do it individually :(*/

    title = g_strdup_printf ("Camorama - %s - %dx%d", cam->vid_cap.name,
                             cam->x, cam->y);
    gtk_window_set_title (GTK_WINDOW
                          (glade_xml_get_widget (cam->xml, "window2")),
                          title);
    g_free (title);

    gtk_window_set_icon (GTK_WINDOW
                         (glade_xml_get_widget (cam->xml, "window2")), logo);
    gtk_window_set_icon (GTK_WINDOW
                         (glade_xml_get_widget (cam->xml, "prefswindow")),
                         logo);

    glade_xml_signal_connect_data (cam->xml, "on_show_effects_activate",
                                   G_CALLBACK (on_show_effects_activate),
                                   cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "togglebutton1"),
                                  cam->show_adjustments);
    glade_xml_signal_connect_data (cam->xml,
                                   "on_show_adjustments1_activate",
                                   G_CALLBACK
                                   (on_show_adjustments1_activate), cam);

    glade_xml_signal_connect_data (cam->xml, "on_large1_activate",
                                   G_CALLBACK (on_change_size_activate), cam);
    glade_xml_signal_connect_data (cam->xml, "on_medium1_activate",
                                   G_CALLBACK (on_change_size_activate), cam);
    glade_xml_signal_connect_data (cam->xml, "on_small1_activate",
                                   G_CALLBACK (on_change_size_activate), cam);

    //glade_xml_signal_connect_data(cam->xml, "capture_func", G_CALLBACK(on_change_size_activate), cam);
    glade_xml_signal_connect_data (cam->xml, "capture_func",
                                   G_CALLBACK (capture_func), cam);
    glade_xml_signal_connect_data (cam->xml, "gtk_main_quit",
                                   G_CALLBACK (delete_event), NULL);

    /* sliders */
    glade_xml_signal_connect_data (cam->xml,
                                   "on_scale1_drag_data_received",
                                   G_CALLBACK
                                   (on_scale1_drag_data_received), cam);
    gtk_range_set_value ((GtkRange *)
                         glade_xml_get_widget (cam->xml, "slider1"), 128);
    glade_xml_signal_connect_data (cam->xml, "contrast_change",
                                   G_CALLBACK (contrast_change), cam);
    gtk_range_set_value ((GtkRange *)
                         glade_xml_get_widget (cam->xml, "slider2"),
                         (int) (cam->contrast / 256));
    glade_xml_signal_connect_data (cam->xml, "brightness_change",
                                   G_CALLBACK (brightness_change), cam);
    gtk_range_set_value ((GtkRange *)
                         glade_xml_get_widget (cam->xml, "slider3"),
                         (int) (cam->brightness / 256));
    glade_xml_signal_connect_data (cam->xml, "colour_change",
                                   G_CALLBACK (colour_change), cam);
    gtk_range_set_value ((GtkRange *)
                         glade_xml_get_widget (cam->xml, "slider4"),
                         (int) (cam->colour / 256));
    glade_xml_signal_connect_data (cam->xml, "hue_change",
                                   G_CALLBACK (hue_change), cam);
    gtk_range_set_value ((GtkRange *)
                         glade_xml_get_widget (cam->xml, "slider5"),
                         (int) (cam->hue / 256));
    glade_xml_signal_connect_data (cam->xml, "wb_change",
                                   G_CALLBACK (wb_change), cam);
    gtk_range_set_value ((GtkRange *)
                         glade_xml_get_widget (cam->xml, "slider6"),
                         (int) (cam->wb / 256));

    /* buttons */
    glade_xml_signal_connect_data (cam->xml, "fix_colour_func",
                                   G_CALLBACK (fix_colour_func),
                                   (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "threshold_func1",
                                   G_CALLBACK (threshold_func1),
                                   (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "threshold_ch_func",
                                   G_CALLBACK (threshold_ch_func),
                                   (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "edge_func1",
                                   G_CALLBACK (edge_func1), (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "sobel_func",
                                   G_CALLBACK (sobel_func), (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "edge_func3",
                                   G_CALLBACK (edge_func3), (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "negative_func",
                                   G_CALLBACK (negative_func),
                                   (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "mirror_func",
                                   G_CALLBACK (mirror_func), (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "colour_func",
                                   G_CALLBACK (colour_func), (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "smooth_func",
                                   G_CALLBACK (smooth_func), (gpointer) NULL);
    glade_xml_signal_connect_data (cam->xml, "edge_func1",
                                   G_CALLBACK (edge_func1), (gpointer) NULL);

    glade_xml_signal_connect_data (cam->xml,
                                   "on_drawingarea1_expose_event",
                                   G_CALLBACK
                                   (on_drawingarea1_expose_event),
                                   (gpointer) cam);
    glade_xml_signal_connect_data (cam->xml, "on_status_show",
                                   G_CALLBACK (on_status_show),
                                   (gpointer) cam);
    glade_xml_signal_connect_data (cam->xml, "on_quit1_activate",
                                   G_CALLBACK (on_quit1_activate),
                                   (gpointer) cam);
    glade_xml_signal_connect_data (cam->xml, "on_preferences1_activate",
                                   G_CALLBACK (on_preferences1_activate),
                                   (gpointer) cam);
    glade_xml_signal_connect_data (cam->xml, "on_about1_activate",
                                   G_CALLBACK (on_about1_activate),
                                   (gpointer) cam);

    /* prefs */
    glade_xml_signal_connect_data (cam->xml, "prefs_func",
                                   G_CALLBACK (prefs_func), cam);

    /* general */
    glade_xml_signal_connect_data (cam->xml, "cap_func",
                                   G_CALLBACK (cap_func), cam);

    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "captured_cb"),
                                  cam->cap);

    glade_xml_signal_connect_data (cam->xml, "rcap_func",
                                   G_CALLBACK (rcap_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "rcapture"),
                                  cam->rcap);

    glade_xml_signal_connect_data (cam->xml, "acap_func",
                                   G_CALLBACK (acap_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "acapture"),
                                  cam->acap);

    glade_xml_signal_connect_data (cam->xml, "interval_change",
                                   G_CALLBACK (interval_change), cam);

    gtk_spin_button_set_value ((GtkSpinButton *)
                               glade_xml_get_widget (cam->xml,
                                                     "interval_entry"),
                               (cam->timeout_interval / 60000));

    /* local */
    dentry = glade_xml_get_widget (cam->xml, "dentry");
    entry2 = glade_xml_get_widget (cam->xml, "entry2");
    gtk_entry_set_text (GTK_ENTRY
                        (gnome_file_entry_gtk_entry
                         (GNOME_FILE_ENTRY (dentry))), cam->pixdir);

    gtk_entry_set_text (GTK_ENTRY (entry2), cam->capturefile);

    glade_xml_signal_connect_data (cam->xml, "append_func",
                                   G_CALLBACK (append_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "appendbutton"),
                                  cam->timefn);

    glade_xml_signal_connect_data (cam->xml, "jpg_func",
                                   G_CALLBACK (jpg_func), cam);
    if (cam->savetype == JPEG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      glade_xml_get_widget (cam->xml,
                                                            "jpgb"), TRUE);
    }
    glade_xml_signal_connect_data (cam->xml, "png_func",
                                   G_CALLBACK (png_func), cam);
    if (cam->savetype == PNG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      glade_xml_get_widget (cam->xml,
                                                            "pngb"), TRUE);
    }

    glade_xml_signal_connect_data (cam->xml, "ts_func",
                                   G_CALLBACK (ts_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "tsbutton"),
                                  cam->timestamp);

    /* remote */
    login_entry = glade_xml_get_widget (cam->xml, "login_entry");
    host_entry = glade_xml_get_widget (cam->xml, "host_entry");
    pw_entry = glade_xml_get_widget (cam->xml, "pw_entry");
    directory_entry = glade_xml_get_widget (cam->xml, "directory_entry");
    filename_entry = glade_xml_get_widget (cam->xml, "filename_entry");
    gtk_entry_set_text (GTK_ENTRY (host_entry), cam->rhost);
    gtk_entry_set_text (GTK_ENTRY (login_entry), cam->rlogin);
    gtk_entry_set_text (GTK_ENTRY (pw_entry), cam->rpw);
    gtk_entry_set_text (GTK_ENTRY (directory_entry), cam->rpixdir);
    gtk_entry_set_text (GTK_ENTRY (filename_entry), cam->rcapturefile);

    glade_xml_signal_connect_data (cam->xml, "rappend_func",
                                   G_CALLBACK (rappend_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "timecb"),
                                  cam->rtimefn);

    glade_xml_signal_connect_data (cam->xml, "rjpg_func",
                                   G_CALLBACK (rjpg_func), cam);
    if (cam->rsavetype == JPEG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      glade_xml_get_widget (cam->xml,
                                                            "fjpgb"), TRUE);
    }
    glade_xml_signal_connect_data (cam->xml, "rpng_func",
                                   G_CALLBACK (rpng_func), cam);
    if (cam->rsavetype == PNG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      glade_xml_get_widget (cam->xml,
                                                            "fpngb"), TRUE);
    }

    glade_xml_signal_connect_data (cam->xml, "rts_func",
                                   G_CALLBACK (rts_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml,
                                                        "tsbutton2"),
                                  cam->rtimestamp);

    /* timestamp */
    glade_xml_signal_connect_data (cam->xml, "customstring_func",
                                   G_CALLBACK (customstring_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml, "cscb"),
                                  cam->usestring);

    string_entry = glade_xml_get_widget (cam->xml, "string_entry");
    gtk_entry_set_text (GTK_ENTRY (string_entry), cam->ts_string);

    glade_xml_signal_connect_data (cam->xml, "drawdate_func",
                                   G_CALLBACK (drawdate_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  glade_xml_get_widget (cam->xml, "tscb"),
                                  cam->usedate);

    cam->status = glade_xml_get_widget (cam->xml, "status");
    set_sensitive (cam);
    gtk_widget_set_sensitive (glade_xml_get_widget
                              (cam->xml, "string_entry"), cam->usestring);

    gtk_widget_set_size_request (glade_xml_get_widget (cam->xml, "da"),
                                 cam->x, cam->y);

    prefswindow = glade_xml_get_widget (cam->xml, "prefswindow");
}

int main (int argc, char *argv[])
{
    cam cam_object, *cam;
    Display *display;
    Screen *screen_num;
    gchar *poopoo = NULL;
    gchar *pixfilename = "camorama/camorama.png";
    gchar *filename;            //= "/usr/opt/garnome/share/camorama/camorama.glade";
    int x = -1, y = -1;
    gboolean buggery = FALSE;
    GtkWidget *button;
    GConfClient *gc;

    const struct poptOption popt_options[] = {
        {"version", 'V', POPT_ARG_NONE, &ver, 0,
         N_("show version and exit"), NULL},
        {"device", 'd', POPT_ARG_STRING, &poopoo, 0,
         N_("v4l device to use"), NULL},
        {"debug", 'D', POPT_ARG_NONE, &buggery, 0,
         N_("enable debuging code"), NULL},
        {"width", 'x', POPT_ARG_INT, &x, 0, N_("capture width"),
         NULL},
        {"height", 'y', POPT_ARG_INT, &y, 0, N_("capture height"),
         NULL},
        {"max", 'M', POPT_ARG_NONE, &max, 0,
         N_("maximum capture size"), NULL},
        {"min", 'm', POPT_ARG_NONE, &min, 0,
         N_("minimum capture size"), NULL},
        {"half", 'H', POPT_ARG_NONE, &half, 0,
         N_("middle capture size"), NULL},
        {"read", 'R', POPT_ARG_NONE, &use_read, 0,
         N_("use read() rather than mmap()"), NULL},
        POPT_TABLEEND
    };

    cam = &cam_object;
    /* set some default values */
    cam->frame_number = 0;
    cam->pic = NULL;
    cam->pixmap = NULL;
    cam->size = PICHALF;
    cam->video_dev = NULL;
    cam->read = FALSE;

    bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    setlocale (LC_ALL, "");

    /* gnome_program_init  - initialize everything (gconf, threads, etc) */
    gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
                        GNOME_PARAM_APP_DATADIR, DATADIR,
                        GNOME_PARAM_POPT_TABLE, popt_options,
                        GNOME_PARAM_HUMAN_READABLE_NAME, _("camorama"), NULL);

    cam->debug = buggery;

    if (poopoo == NULL) {
        cam->video_dev = g_strdup ("/dev/video0");
    } else {
        cam->video_dev = g_strdup (poopoo);
    }
    cam->x = x;
    cam->y = y;
    glade_gnome_init ();

    if (ver) {
        fprintf (stderr, _("\n\nCamorama version %s\n\n"), VERSION);
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
        printf ("gah!\n");
        cam->read = TRUE;
    }
    gc = gconf_client_get_default ();
    cam->gc = gc;

    gconf_client_add_dir (cam->gc, PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
    gconf_client_notify_add (cam->gc, KEY1, (void *) gconf_notify_func,
                             cam->pixdir, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY5, (void *) gconf_notify_func,
                             cam->rhost, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY2, (void *) gconf_notify_func,
                             cam->capturefile, NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY3,
                             (void *) gconf_notify_func_int,
                             GINT_TO_POINTER (cam->savetype), NULL, NULL);
    gconf_client_notify_add (cam->gc, KEY4,
                             (void *) gconf_notify_func_bool,
                             &cam->timestamp, NULL, NULL);

    cam->pixdir = g_strdup (gconf_client_get_string (cam->gc, KEY1, NULL));
    cam->capturefile =
        g_strdup (gconf_client_get_string (cam->gc, KEY2, NULL));
    cam->rhost = g_strdup (gconf_client_get_string (cam->gc, KEY5, NULL));
    cam->rlogin = g_strdup (gconf_client_get_string (cam->gc, KEY6, NULL));
    cam->rpw = g_strdup (gconf_client_get_string (cam->gc, KEY7, NULL));
    cam->rpixdir = g_strdup (gconf_client_get_string (cam->gc, KEY8, NULL));
    cam->rcapturefile =
        g_strdup (gconf_client_get_string (cam->gc, KEY9, NULL));
    cam->savetype = gconf_client_get_int (cam->gc, KEY3, NULL);
    cam->rsavetype = gconf_client_get_int (cam->gc, KEY10, NULL);
    cam->ts_string =
        g_strdup (gconf_client_get_string (cam->gc, KEY16, NULL));
    cam->date_format = "%Y-%m-%d %H:%M:%S";
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

    /* get desktop depth */
    display = (Display *) gdk_x11_get_default_xdisplay ();
    screen_num = xlib_rgb_get_screen ();
    gdk_pixbuf_xlib_init (display, 0);
    cam->desk_depth = xlib_rgb_get_depth ();

    func_state.wacky = 0;
    func_state.threshold = 0;
    func_state.laplace = 0;
    func_state.negative = 0;
    func_state.colour = 0;
    func_state.mirror = 0;
    func_state.smooth = 0;
    func_state.fc = 1;

    cam->dither = 128;

    cam->dev = open (cam->video_dev, O_RDWR);

    camera_cap (cam);
    get_win_info (cam);

    /* query/set window attributes */
    cam->vid_win.x = 0;
    cam->vid_win.y = 0;
    cam->vid_win.width = cam->x;
    cam->vid_win.height = cam->y;
    cam->vid_win.chromakey = 0;
    cam->vid_win.flags = 0;

    set_win_info (cam);
    get_win_info (cam);

    /* get picture attributes */
    get_pic_info (cam);

    /* set_pic_info(cam); */
    cam->contrast = cam->vid_pic.contrast;
    cam->brightness = cam->vid_pic.brightness;
    cam->colour = cam->vid_pic.colour;
    cam->hue = cam->vid_pic.hue;
    cam->wb = cam->vid_pic.whiteness;
    cam->depth = cam->vid_pic.depth / 8;
    cam->pic_buf = malloc (cam->x * cam->y * cam->depth);
    cam->tmp =
        malloc (cam->vid_cap.maxwidth * cam->vid_cap.maxheight * cam->depth);
    //cam->tmp = NULL;
    /* set the buffer size */
    if (cam->read == FALSE) {
        set_buffer (cam);
    }
    //cam->read = FALSE;
    /* initialize cam and create the window */

    if (cam->read == FALSE) {
        pt2Function = timeout_func;
        init_cam (NULL, cam);
    } else {
        printf ("using read()\n");
        cam->pic =
            realloc (cam->pic,
                     (cam->vid_cap.maxwidth * cam->vid_cap.maxheight * 3));
        pt2Function = read_timeout_func;
    }
    cam->pixmap = gdk_pixmap_new (NULL, cam->x, cam->y, cam->desk_depth);

    filename =
        gnome_program_locate_file (NULL,
                                   GNOME_FILE_DOMAIN_APP_DATADIR,
                                   "camorama/camorama.glade", TRUE, NULL);
    if (filename == NULL) {
        error_dialog (_
                      ("Couldn't find the main interface file (camorama.glade)."));
        exit (1);
    }
    //pixfilename = gnome_program_locate_file(NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "pixmaps/camorama.png", TRUE, NULL);
    //printf("pixfile = %s\n",pixfilename);
    //pixfilename);
    //printf("pixfile = %s\n",pixfilename);
    cam->xml = glade_xml_new (filename, NULL, NULL);
    /*eggtray */

    /*tray_icon = egg_tray_icon_new ("Our other cool tray icon");
     * button = gtk_button_new_with_label ("This is a another\ncool tray icon");
     * g_signal_connect (button, "clicked",
     * G_CALLBACK (second_button_pressed), tray_icon);
     * 
     * gtk_container_add (GTK_CONTAINER (tray_icon), button);
     * gtk_widget_show_all (GTK_WIDGET (tray_icon)); */
    load_interface (cam);

    cam->idle_id = gtk_idle_add ((GSourceFunc) pt2Function, (gpointer) cam);

    gtk_timeout_add (2000, (GSourceFunc) fps, cam->status);
    gdk_threads_enter ();
    gtk_main ();
    gdk_threads_leave ();
    return 0;
}
