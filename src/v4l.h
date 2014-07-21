#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <linux/types.h>
#include <linux/videodev.h>
#include <signal.h>
#include <png.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include "eggtrayicon.h"

#define PICMAX 0
#define PICMIN 1
#define PICHALF 2
#define JPEG 0
#define PNG 1
#define PPM 2

typedef struct camera {
    int dev;
    int x;
    int y;
    int depth;
    int desk_depth;
    int size;
    int dither;
    int contrast, brightness, colour, hue, wb;
    int frame_number;
    struct video_capability vid_cap;
    struct video_picture vid_pic;
    struct video_window vid_win;
    struct video_mbuf vid_buf;
    struct video_mmap vid_map;
    char *video_dev;
    unsigned char *pic;
    unsigned char *image;
    gchar *capturefile, *rcapturefile;
    gchar *pixdir, *rpixdir;
    int savetype, rsavetype;
    gchar *rhost, *rlogin, *rpw;
    gchar *ts_string;
    gchar *date_format;
    gboolean debug, read, hidden;
    gboolean cap, rcap, acap, show_adjustments;
    gboolean timestamp, rtimestamp, usedate, usestring;
    gboolean rtimefn, timefn;
    GdkPixmap *pixmap;
	 GdkWindow *window;
    GtkWidget *da, *tooltips, *tray_tooltip, *status;
    unsigned char *pic_buf, *tmp;
    guint timeout_id, idle_id;
    guint32 timeout_interval;
    GConfClient *gc;
    GladeXML *xml;
    EggTrayIcon *tray_icon;
} cam;

void camera_cap (cam *);
void set_win_info (cam * cam);
void get_pic_info (cam *);
void set_pic_info (cam *);
void get_win_info (cam *);
void set_buffer (cam *);
