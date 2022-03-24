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
    unsigned int pixformat;
    unsigned int x, y;
    unsigned int depth;
    float max_fps;
    int order;
};

struct colorspace_parms {
    enum v4l2_colorspace     colorspace;
    enum v4l2_xfer_func      xfer_func;
    enum v4l2_ycbcr_encoding ycbcr_enc;
    enum v4l2_quantization   quantization;
};

typedef struct  {
    char *name;
    gint64 value;
} video_control_menu_t;

struct camera;

typedef struct {
    char *name;
    char *group;

    enum v4l2_ctrl_type type;
    guint32 id;
    gint32 min, max, def;
    gint32 step;

    unsigned int menu_size;
    video_control_menu_t *menu;

    struct camera *cam;

    void *next;

} video_controls_t;

typedef struct camera {
    int dev;
    unsigned int width, height;
    int bpp;
    float scale;
    CamoImageSize size;
    char name[32];

    video_controls_t *controls;

    int contrast, brightness, whiteness, colour, hue, zoom;
    guint32 zoom_cid;
    unsigned int bytesperline, sizeimage;
    unsigned int pixformat;
    int input;
    int frame_number;

    int n_threads;

    GMutex remote_save_mutex;      /* Protects n_threads */
    GMutex pixbuf_mutex;           /* Protects pic_buf */

    unsigned int min_width, min_height, max_width, max_height;
    struct colorspace_parms colorspc;

    unsigned int n_res;
    struct resolutions *res;

    char *video_dev;
    unsigned char *image;
    gchar *capturefile, *rcapturefile;
    gchar *pixdir, *host, *proto, *rdir, *uri;
    int savetype, rsavetype;
    gchar *ts_string;
    gchar *date_format;
    gboolean debug, read, userptr, use_libv4l, hidden;
    gboolean cap, rcap, acap, show_adjustments, show_effects;
    gboolean timestamp, rtimestamp, usedate, usestring;
    gboolean rtimefn, timefn;
    GtkWidget *da, *status;
    unsigned char *pic_buf, *tmp;
    guint timeout_id, timeout_fps_id, idle_id;
    guint32 timeout_interval;
    GSettings *gc;
    GtkBuilder *xml;
    GdkPixbuf *pb;

    GtkApplication *app;
    GtkWidget *controls_window;

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

int cam_open(cam_t *cam, int oflag);
int cam_close(cam_t *cam);
unsigned char *cam_read(cam_t *cam);
int cam_ioctl(cam_t *cam, unsigned long cmd, void *arg);
int cam_query_controls(cam_t *cam);
void cam_free_controls(cam_t *cam);
video_controls_t *cam_find_control_per_id(cam_t *cam, guint32 id);
int cam_set_control(cam_t *cam, guint32 id, void *value);
int cam_get_control(cam_t *cam, guint32 id, void *value);

int camera_cap(cam_t *);
void print_cam(cam_t *);
void try_set_win_info(cam_t *cam, unsigned int pixformat,
                      unsigned int *x, unsigned int *y);
void set_win_info(cam_t *cam);
void get_pic_info(cam_t *);
void get_win_info(cam_t *);
void get_supported_resolutions(cam_t *cam, gboolean all_supported);
void start_streaming(cam_t *cam);
void capture_buffers(cam_t *cam, unsigned char *outbuf, unsigned int len);
void stop_streaming(cam_t *cam);
void start_streaming_userptr(cam_t *cam);
void capture_buffers_userptr(cam_t *cam, unsigned char *outbuf);
void stop_streaming_userptr(cam_t *cam);

#endif                          /* !CAMORAMA_V4L_H */
