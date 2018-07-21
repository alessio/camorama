#ifndef CAMORAMA_V4L_H
#define CAMORAMA_V4L_H

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
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <signal.h>
#include <png.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

#include "camorama-filter-chain.h"

typedef enum {
   PICMAX = 0,
   PICMIN = 1,
   PICHALF = 2
} CamoImageSize;

enum {
   JPEG = 0,
   PNG = 1,
   PPM = 2
};

struct buffer_start_len {
   void   *start;
   size_t length;
};

typedef struct camera {
   int dev;
   int width;
   int height;
   int depth;
   int desk_depth;
   CamoImageSize size;
   char name[32];
   int contrast, brightness, whiteness, colour, hue, wb, bytesperline;
   unsigned int pixformat;
   int frame_number;

   int min_width, min_height, max_width, max_height;

   char *video_dev;
   unsigned char *image;
   gchar *capturefile, *rcapturefile;
   gchar *pixdir, *rpixdir;
   int savetype, rsavetype;
   gchar *rhost, *rlogin, *rpw;
   gchar *ts_string;
   gchar *date_format;
   gboolean debug, read, hidden;
   gboolean cap, rcap, acap, show_adjustments, show_effects;
   gboolean timestamp, rtimestamp, usedate, usestring;
   gboolean rtimefn, timefn;
   GdkPixmap *pixmap;
   GtkWidget *da, *tray_tooltip, *status;
   unsigned char *pic_buf, *tmp;
   guint timeout_id, idle_id;
   guint32 timeout_interval;
   GConfClient *gc;
   GladeXML *xml;
   GtkStatusIcon *tray_icon;

   CamoramaFilterChain* filter_chain;

   /* Buffer handling - should be used only inside v4l.c */
   struct v4l2_requestbuffers req;
   unsigned int n_buffers;
   struct  {
      void   *start;
      size_t length;
   } *buffers;
} cam;

void camera_cap (cam *);
void set_win_info (cam * cam);
void get_pic_info (cam *);
void get_win_info (cam *);
void start_streaming(cam * cam);
void capture_buffers(cam * cam, char *outbuf, int len);
void stop_streaming(cam * cam);

#endif /* !CAMORAMA_V4L_H */

