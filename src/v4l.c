#include <errno.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include "v4l.h"
#include "support.h"

extern int frame_number;

void print_cam(cam_t *cam)
{
    printf("\nCamera Info\n");
    printf("-----------\n");
    printf("device = %s, x = %d, y = %d\n", cam->video_dev, cam->width,
           cam->height);
    printf("bits per pixel = %d\n", cam->bpp);
    if (cam->width <= 0 || cam->height <= 0) {
        switch (cam->size) {
        case PICMAX:
            printf("size = PICMAX\n");
        break;
        case PICMIN:
            printf("size = PICMIN\n");
        break;
        case PICHALF:
        default:
            printf("size = PICHALF\n");
        break;
        }
    }
    printf("capture directory = %s, capture file = %s\n", cam->pixdir,
           cam->capturefile);
    printf("remote host = %s\n", cam->uri);
    if (strcmp(cam->ts_string, "Camorama!"))
        printf("timestamp = %s\n\n", cam->ts_string);
}

static void insert_resolution(cam_t *cam, unsigned int x, unsigned int y,
                              float max_fps)
{
    unsigned int i;

    try_set_win_info(cam, &x, &y);
    for (i = 0; i < cam->n_res; i++) {
        if (cam->res[i].x == x && cam->res[i].y == y)
        return;
    }

    cam->res = realloc(cam->res, (cam->n_res + 1) * sizeof(struct resolutions));

    cam->res[cam->n_res].x = x;
    cam->res[cam->n_res].y = y;
    cam->res[cam->n_res].max_fps = max_fps;
    cam->n_res++;
}

static int sort_func(const void *__b, const void *__a)
{
    const struct resolutions *a = __a;
    const struct resolutions *b = __b;
    int r;

    r = (int)b->x - a->x;
    if (!r)
         r = (int)b->y - a->y;

    return r;
}

static float get_max_fps_discrete(cam_t *cam,
                                  struct v4l2_frmsizeenum *frmsize)
{
    struct v4l2_frmivalenum frmival;
    float fps, max_fps = -1;

    frmival.width = frmsize->discrete.width;
    frmival.height = frmsize->discrete.height;
    frmival.pixel_format = frmsize->pixel_format;
    frmival.index = 0;

    for (frmival.index = 0;
         !v4l2_ioctl(cam->dev, VIDIOC_ENUM_FRAMEINTERVALS, &frmival);
         frmival.index++) {
            fps = ((float)frmival.discrete.denominator)/frmival.discrete.numerator;
            if (fps > max_fps)
                max_fps = fps;
    }
    return max_fps;
}

void get_supported_resolutions(cam_t *cam)
{
    struct v4l2_fmtdesc fmt;
    struct v4l2_frmsizeenum frmsize;
    int i;
    unsigned int x, y;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (fmt.index = 0;
         !v4l2_ioctl(cam->dev, VIDIOC_ENUM_FMT, &fmt);
         fmt.index++) {
        if (cam->pixformat != fmt.pixelformat)
            continue;

        frmsize.pixel_format = fmt.pixelformat;
        frmsize.index = 0;

        while (!v4l2_ioctl(cam->dev, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    insert_resolution(cam, frmsize.discrete.width,
                                      frmsize.discrete.height,
                                      get_max_fps_discrete(cam, &frmsize));
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                    for (i = 0; i < 4; i++) {
                        x = frmsize.stepwise.min_width +
                            i * (frmsize.stepwise.max_width -
                                 frmsize.stepwise.min_width) / 4;
                        y = frmsize.stepwise.min_height +
                            i * (frmsize.stepwise.max_height -
                                 frmsize.stepwise.min_height) / 4;
                        insert_resolution(cam, x, y, -1);
                    }
            }
            frmsize.index++;
        }
    }
    qsort(cam->res, cam->n_res, sizeof(struct resolutions), sort_func);
}

void camera_cap(cam_t *cam)
{
    char *msg;
    int i;
    struct v4l2_capability vid_cap = { 0 };
    struct v4l2_fmtdesc fmtdesc = {.type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
    struct v4l2_format fmt;

    /* Query device capabilities */
    if (v4l2_ioctl(cam->dev, VIDIOC_QUERYCAP, &vid_cap) == -1) {

        msg = g_strdup_printf(_("Could not connect to video device (%s).\n"
                                "Please check connection. Error: %d"),
                              cam->video_dev, errno);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    /* Query supported resolutions */

    cam->rdir_ok = FALSE;
    cam->min_width = (unsigned)-1;
    cam->min_height = (unsigned)-1;
    cam->max_width = 0;
    cam->max_height = 0;
    for (i = 0;; i++) {
        fmtdesc.index = i;

        if (v4l2_ioctl(cam->dev, VIDIOC_ENUM_FMT, &fmtdesc))
        break;

        if (cam->debug == TRUE)
            printf("format index %d: FOURCC: '%c%c%c%c' (%08x)%s\n", i,
                   fmtdesc.pixelformat & 0xff,
                   (fmtdesc.pixelformat >> 8) & 0xff,
                   (fmtdesc.pixelformat >> 16) & 0xff,
                   fmtdesc.pixelformat >> 24,
                   fmtdesc.pixelformat,
                   fmtdesc.
                   flags & V4L2_FMT_FLAG_EMULATED ? " (emulated)" : "");

        /* FIXME: add a check for emulated formats */

        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;
        fmt.fmt.pix.width = 48;
        fmt.fmt.pix.height = 32;

        if (!v4l2_ioctl(cam->dev, VIDIOC_TRY_FMT, &fmt)) {
            if (fmt.fmt.pix.width < cam->min_width)
                    cam->min_width = fmt.fmt.pix.width;
            if (fmt.fmt.pix.height < cam->min_height)
                    cam->min_height = fmt.fmt.pix.height;
            if (cam->debug == TRUE)
                    printf("  MIN: %dx%d\n", fmt.fmt.pix.width,
                           fmt.fmt.pix.height);
        }

        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;
        fmt.fmt.pix.width = 100000;
        fmt.fmt.pix.height = 100000;

        if (!v4l2_ioctl(cam->dev, VIDIOC_TRY_FMT, &fmt)) {
        if (fmt.fmt.pix.width > cam->max_width)
                cam->max_width = fmt.fmt.pix.width;
        if (fmt.fmt.pix.height > cam->max_height)
                cam->max_height = fmt.fmt.pix.height;
        if (cam->debug == TRUE)
                printf("  MAX: %dx%d\n", fmt.fmt.pix.width,
                       fmt.fmt.pix.height);
        }
    }

    /* Adjust camera resolution */

    if (cam->width > 0 && cam->height > 0) {
        if (cam->max_width < cam->width)
            cam->width = cam->max_width;
        else if (cam->min_width > cam->width)
            cam->width = cam->min_width;

        if (cam->max_height < cam->height)
            cam->height = cam->max_height;
        else if (cam->min_height > cam->height)
            cam->height = cam->min_height;
    } else {
        switch (cam->size) {
        case PICMAX:
            cam->width = cam->max_width;
            cam->height = cam->max_height;
        break;

        case PICMIN:
            cam->width = cam->min_width;
            cam->height = cam->min_height;
        break;

        case PICHALF:
            cam->width = cam->max_width / 2;
            cam->height = cam->max_height / 2;
        break;

        default:
            cam->width = cam->max_width / 2;
            cam->height = cam->max_height / 2;
        break;
        }
    }

    if (!(vid_cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
        msg = g_strdup_printf(_("Device %s is not a video capture device)."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    if (!(vid_cap.device_caps & V4L2_CAP_STREAMING))
        cam->read = TRUE;

    strncpy(cam->name, (const char *)vid_cap.card, sizeof(cam->name));
    cam->name[sizeof(cam->name) - 1] = '\0';

    if (cam->debug == TRUE) {
        printf("\nVIDIOC_QUERYCAP\n");
        printf("device name = %s\n", vid_cap.card);
        printf("device caps = 0x%08x\n", vid_cap.device_caps);
        printf("max width = %d\n", cam->max_width);
        printf("max height = %d\n", cam->max_height);
        printf("min width = %d\n", cam->min_width);
        printf("min height = %d\n", cam->min_height);
    }
}

void get_pic_info(cam_t *cam)
{
    int i;

    if (cam->debug == TRUE)
        printf("\nVideo control settings:\n");

    i = v4l2_get_control(cam->dev, V4L2_CID_HUE);
    if (i >= 0) {
        cam->hue = i;
        if (cam->debug == TRUE)
            printf("hue = %d\n", cam->hue);
    } else {
        cam->hue = -1;
    }
    i = v4l2_get_control(cam->dev, V4L2_CID_SATURATION);
    if (i >= 0) {
        cam->colour = i;
        if (cam->debug == TRUE)
            printf("colour = %d\n", cam->colour);
    } else {
        cam->colour = -1;
    }
    i = v4l2_get_control(cam->dev, V4L2_CID_CONTRAST);
    if (i >= 0) {
        cam->contrast = i;
        if (cam->debug == TRUE)
            printf("contrast = %d\n", cam->contrast);
    } else {
        cam->contrast = -1;
    }
    i = v4l2_get_control(cam->dev, V4L2_CID_WHITENESS);
    if (i >= 0) {
        cam->whiteness = i;
        if (cam->debug == TRUE)
            printf("whiteness = %d\n", cam->whiteness);
    } else {
        cam->whiteness = -1;
    }
    i = v4l2_get_control(cam->dev, V4L2_CID_BRIGHTNESS);
    if (i >= 0) {
        cam->brightness = i;
        if (cam->debug == TRUE)
            printf("brightness = %d\n", cam->brightness);
    } else {
        cam->brightness = -1;
    }
}

void get_win_info(cam_t *cam)
{
    gchar *msg;
    struct v4l2_format fmt = { 0 };

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(cam->dev, VIDIOC_G_FMT, &fmt)) {
        msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."),
                              cam->video_dev);
        error_dialog(msg);
        if (cam->debug == TRUE)
            g_message("VIDIOC_G_FMT  --  could not get picture info");

        return;
    }

    if (cam->debug == TRUE) {
        printf("\nVIDIOC_G_FMT\n");
        printf("format FOURCC: '%c%c%c%c' (%08x)\n",
               fmt.fmt.pix.pixelformat & 0xff,
               (fmt.fmt.pix.pixelformat >> 8) & 0xff,
               (fmt.fmt.pix.pixelformat >> 16) & 0xff,
               fmt.fmt.pix.pixelformat >> 24,
               fmt.fmt.pix.pixelformat);
        printf("x = %d\n", fmt.fmt.pix.width);
        printf("y = %d\n", fmt.fmt.pix.height);
        if (fmt.fmt.pix.bytesperline)
            printf("bytes/line = %d\n", fmt.fmt.pix.bytesperline);
    }

    if (!fmt.fmt.pix.bytesperline) {
        if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420)
            fmt.fmt.pix.bytesperline = fmt.fmt.pix.width * 2;
        else
            fmt.fmt.pix.bytesperline = fmt.fmt.pix.width * 3;
    }

    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR24 ||
        fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {

        cam->pixformat = fmt.fmt.pix.pixelformat;
        cam->bpp = ((fmt.fmt.pix.bytesperline << 3) + (fmt.fmt.pix.width - 1)) / fmt.fmt.pix.width;
        cam->width = fmt.fmt.pix.width;
        cam->height = fmt.fmt.pix.height;
        cam->bytesperline = fmt.fmt.pix.bytesperline;
    }
}

void try_set_win_info(cam_t *cam, unsigned int *x, unsigned int *y)
{
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = cam->pixformat;
    fmt.fmt.pix.width = *x;
    fmt.fmt.pix.height = *y;
    if (!v4l2_ioctl(cam->dev, VIDIOC_TRY_FMT, &fmt)) {
        *x = fmt.fmt.pix.width;
        *y = fmt.fmt.pix.height;
    }
}

void set_win_info(cam_t *cam)
{
    gchar *msg;
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* Get current settings, apply our changes and try the new setting */
    if (v4l2_ioctl(cam->dev, VIDIOC_G_FMT, &fmt)) {
        if (cam->debug) {
            g_message("VIDIOC_G_FMT  --  could not get window info, exiting....");
        }
        msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24 &&
        fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUV420)
        cam->pixformat = V4L2_PIX_FMT_BGR24;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = cam->pixformat;
    fmt.fmt.pix.width = cam->width;
    fmt.fmt.pix.height = cam->height;
    if (v4l2_ioctl(cam->dev, VIDIOC_S_FMT, &fmt)) {
        if (cam->debug)
            g_message("VIDIOC_S_FMT  --  could not set window info, exiting....");

        msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    /* Check if returned format is valid */
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24 &&
        fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUV420) {
        if (cam->debug) {
            g_message("VIDIOC_S_FMT  --  could not set format to %c%c%c%c (was set to %c%c%c%c instead), exiting....",
                      cam->pixformat & 0xff,
                      (cam->pixformat >> 8) & 0xff,
                      (cam->pixformat >> 16) & 0xff,
                      cam->pixformat >> 24,
                      fmt.fmt.pix.pixelformat & 0xff,
                      (fmt.fmt.pix.pixelformat >> 8) & 0xff,
                      (fmt.fmt.pix.pixelformat >> 16) & 0xff,
                      fmt.fmt.pix.pixelformat >> 24);
        }
        msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    /* Resolution may have changed. Store the retrieved one */
    cam->pixformat = fmt.fmt.pix.pixelformat;
    cam->bytesperline = fmt.fmt.pix.bytesperline;

    cam->bpp = ((fmt.fmt.pix.bytesperline << 3) + (fmt.fmt.pix.width - 1)) / fmt.fmt.pix.width;

    cam->width = fmt.fmt.pix.width;
    cam->height = fmt.fmt.pix.height;
}

void start_streaming(cam_t *cam)
{
    char *msg;
    unsigned int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    memset(&cam->req, 0, sizeof(cam->req));
    cam->req.count = 2;
    cam->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam->req.memory = V4L2_MEMORY_MMAP;
    if (v4l2_ioctl(cam->dev, VIDIOC_REQBUFS, &cam->req)) {
        msg = g_strdup_printf(_("VIDIOC_REQBUFS  --  could not request buffers (%s), exiting...."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    cam->buffers = calloc(cam->req.count, sizeof(*cam->buffers));
    for (cam->n_buffers = 0;
         cam->n_buffers < cam->req.count;
         ++cam->n_buffers) {
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = cam->n_buffers;

        if (v4l2_ioctl(cam->dev, VIDIOC_QUERYBUF, &buf)) {
            msg = g_strdup_printf(_("VIDIOC_QUERYBUF  --  could not query buffers (%s), exiting...."),
                                  cam->video_dev);
            error_dialog(msg);
            g_free(msg);
            exit(0);
        }

        cam->buffers[cam->n_buffers].length = buf.length;
        cam->buffers[cam->n_buffers].start = v4l2_mmap(NULL, buf.length,
                                                       PROT_READ | PROT_WRITE,
                                                       MAP_SHARED,
                                                       cam->dev,
                                                       buf.m.offset);

        if (cam->buffers[cam->n_buffers].start == MAP_FAILED) {
            msg = g_strdup_printf(_("failed to memory map buffers (%s), exiting...."),
                                  cam->video_dev);
            error_dialog(msg);
            g_free(msg);
            exit(0);
        }
    }

    for (i = 0; i < cam->n_buffers; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (v4l2_ioctl(cam->dev, VIDIOC_QBUF, &buf)) {
            msg = g_strdup_printf(_("VIDIOC_QBUF  --  could not enqueue buffers (%s), exiting...."),
                                  cam->video_dev);
            error_dialog(msg);
            g_free(msg);
            exit(0);
        }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(cam->dev, VIDIOC_STREAMON, &type)) {
        msg = g_strdup_printf(_("failed to start streaming (%s), exiting...."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }
}

void capture_buffers(cam_t *cam, unsigned char *outbuf, unsigned int len)
{
    char *msg;
    unsigned char *inbuf;
    int r;
    unsigned int y;
    fd_set fds;
    struct v4l2_buffer buf;
    struct timeval tv;

    do {
        FD_ZERO(&fds);
        FD_SET(cam->dev, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(cam->dev + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno == EINTR)));

    if (r == -1) {
        msg = g_strdup_printf(_("Timeout while waiting for frames (%s)"),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    v4l2_ioctl(cam->dev, VIDIOC_DQBUF, &buf);

    if (len > buf.bytesused)
        len = buf.bytesused;

    inbuf = cam->buffers[buf.index].start;
    for (y = 0; y < cam->height; y++) {
        memcpy(outbuf, inbuf, cam->width * cam->bpp / 8);
        outbuf += cam->width * cam->bpp / 8;
        inbuf += cam->bytesperline;
    }

    v4l2_ioctl(cam->dev, VIDIOC_QBUF, &buf);
}

void stop_streaming(cam_t *cam)
{
    char *msg;
    unsigned int i;
    int r;
    enum v4l2_buf_type type;
    fd_set fds, fderrs;
    struct v4l2_buffer buf;
    struct timeval tv;

    /* Dequeue all pending buffers */
    for (i = 0; i < cam->n_buffers; ++i) {
        FD_ZERO(&fds);
        FD_SET(cam->dev, &fds);
        FD_ZERO(&fderrs);
        FD_SET(cam->dev, &fderrs);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(cam->dev + 1, &fds, NULL, &fderrs, &tv);
        if (r == -1 && errno == EINTR)
            continue;

        if (r != -1) {
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if (v4l2_ioctl(cam->dev, VIDIOC_DQBUF, &buf))
                break;
        }
    };

    /* Streams off */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(cam->dev, VIDIOC_STREAMOFF, &type)) {
        msg = g_strdup_printf(_("failed to stop streaming (%s), exiting...."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    /* Unmap buffers */
    for (i = 0; i < cam->n_buffers; ++i)
        v4l2_munmap(cam->buffers[i].start, cam->buffers[i].length);

    /* Free existing buffers */
    memset(&cam->req, 0, sizeof(cam->req));
    cam->req.count = 0;
    cam->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam->req.memory = V4L2_MEMORY_MMAP;
    v4l2_ioctl(cam->dev, VIDIOC_REQBUFS, &cam->req);

    free(cam->buffers);
    cam->buffers = NULL;
}
