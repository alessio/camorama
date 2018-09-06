#ifndef CAMORAMA_V4L_H
#define CAMORAMA_V4L_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <signal.h>
#include <png.h>
#include <gconf/gconf-client.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <gtk/gtk.h>
#pragma GCC diagnostic pop

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
    void *start;
    size_t length;
};

struct resolutions {
    unsigned int x, y;
};

typedef struct camera {
    int dev;
    unsigned int width, height;
    unsigned int screen_width, screen_height;
    int bpp;
    float scale;
    CamoImageSize size;
    char name[32];
    int contrast, brightness, whiteness, colour, hue, bytesperline;
    unsigned int pixformat;
    int frame_number;

    unsigned int min_width, min_height, max_width, max_height;

    unsigned int n_res;
    struct resolutions *res;

    char *video_dev;
    unsigned char *image;
    gchar *capturefile, *rcapturefile;
    gchar *pixdir, *host, *proto, *rdir, *uri;
    int savetype, rsavetype;
    gchar *ts_string;
    gchar *date_format;
    gboolean debug, read, hidden;
    gboolean cap, rcap, acap, show_adjustments, show_effects;
    gboolean timestamp, rtimestamp, usedate, usestring;
    gboolean rtimefn, timefn;
    GtkWidget *da, *status;
    unsigned char *pic_buf, *tmp;
    guint timeout_id, idle_id;
    guint32 timeout_interval;
    GConfClient *gc;
    GtkBuilder *xml;
    GdkPixbuf *pb;

    CamoramaFilterChain *filter_chain;

    gboolean rdir_ok;
    GFile *rdir_file;
    GMountOperation *rdir_mop;

    /* Buffer handling - should be used only inside v4l.c */
    struct v4l2_requestbuffers req;
    unsigned int n_buffers;
    struct {
        void *start;
        size_t length;
    } *buffers;
} cam_t;

void camera_cap(cam_t *);
void print_cam(cam_t *);
void try_set_win_info(cam_t *cam, unsigned int *x, unsigned int *y);
void set_win_info(cam_t *cam);
void get_pic_info(cam_t *);
void get_win_info(cam_t *);
void get_supported_resolutions(cam_t *cam);
void start_streaming(cam_t *cam);
void capture_buffers(cam_t *cam, unsigned char *outbuf, unsigned int len);
void stop_streaming(cam_t *cam);

#endif                          /* !CAMORAMA_V4L_H */
