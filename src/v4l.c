#include <errno.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include "v4l.h"
#include "support.h"

extern int frame_number;

#define BYTE_CLAMP(a) CLAMP(a, 0, 255)

/* Formats that are natively supported */
static const unsigned int supported_formats[] = {
    V4L2_PIX_FMT_BGR24,
    V4L2_PIX_FMT_RGB24,
    V4L2_PIX_FMT_ARGB32,
    V4L2_PIX_FMT_XRGB32,
    V4L2_PIX_FMT_BGR32,
    V4L2_PIX_FMT_ABGR32,
    V4L2_PIX_FMT_XBGR32,
    V4L2_PIX_FMT_YUYV,
    V4L2_PIX_FMT_UYVY,
    V4L2_PIX_FMT_YVYU,
    V4L2_PIX_FMT_VYUY,
    V4L2_PIX_FMT_RGB565,
    V4L2_PIX_FMT_RGB565X,
    V4L2_PIX_FMT_RGB32,
    V4L2_PIX_FMT_NV12,
    V4L2_PIX_FMT_NV21,
    V4L2_PIX_FMT_YUV420,
    V4L2_PIX_FMT_YVU420,
};

#define ARRAY_SIZE(a)  (sizeof(a)/sizeof(*a))

static gboolean is_format_supported(cam_t *cam, unsigned int pixformat)
{
    unsigned int i;

    /*
     * As libv4l supports more formats and already selects the format
     * that provides the highest frame rate, use it, if not disabled.
     */
    if (cam->use_libv4l)
        return pixformat == V4L2_PIX_FMT_RGB24;

    for (i = 0; i < ARRAY_SIZE(supported_formats); i++)
        if (supported_formats[i] == pixformat)
            return TRUE;

    return FALSE;
}

static void convert_yuv(enum v4l2_ycbcr_encoding enc,
                        int32_t y, int32_t u, int32_t v,
                        unsigned char **dst)
{
    int full_scale = 1; // FIXME: add support for non-full_scale

    if (full_scale)
        y *= 65536;
    else
        y = (y - 16) * 76284;

    u -= 128;
    v -= 128;

    /*
     * TODO: add BT2020 and SMPTE240M and better handle
     * other differences
     */
    switch (enc) {
    case V4L2_YCBCR_ENC_601:
    case V4L2_YCBCR_ENC_XV601:
    case V4L2_YCBCR_ENC_SYCC:
        /*
         * ITU-R BT.601 matrix:
         *    R = 1.164 * y +    0.0 * u +  1.596 * v
         *    G = 1.164 * y + -0.392 * u + -0.813 * v
         *    B = 1.164 * y +  2.017 * u +    0.0 * v
         */
        *(*dst)++ = BYTE_CLAMP((y              + 104595 * v) >> 16);
        *(*dst)++ = BYTE_CLAMP((y -  25690 * u -  53281 * v) >> 16);
        *(*dst)++ = BYTE_CLAMP((y + 132186 * u             ) >> 16);
        break;
    case V4L2_YCBCR_ENC_DEFAULT:
    case V4L2_YCBCR_ENC_709:
    case V4L2_YCBCR_ENC_XV709:
    case V4L2_YCBCR_ENC_BT2020:
    case V4L2_YCBCR_ENC_BT2020_CONST_LUM:
    case V4L2_YCBCR_ENC_SMPTE240M:
    default:
        /*
         * ITU-R BT.709 matrix:
         *    R = 1.164 * y +    0.0 * u +  1.793 * v
         *    G = 1.164 * y + -0.213 * u + -0.533 * v
         *    B = 1.164 * y +  2.112 * u +    0.0 * v
         */
        *(*dst)++ = BYTE_CLAMP((y              + 117506 * v) >> 16);
        *(*dst)++ = BYTE_CLAMP((y -  13959 * u -  34931 * v) >> 16);
        *(*dst)++ = BYTE_CLAMP((y + 138412 * u             ) >> 16);
        break;
    }
}

static void copy_two_pixels(cam_t *cam,
                            enum v4l2_ycbcr_encoding enc,
                            unsigned char *plane0,
                            unsigned char *plane1,
                            unsigned char *plane2,
                            unsigned char **dst)
{
    uint32_t fourcc = cam->pixformat;
    int32_t y_off, u_off, u, v;
    uint16_t pix;
    int i;

    switch (cam->pixformat) {
    case V4L2_PIX_FMT_RGB565: /* rrrrrggg gggbbbbb */
        for (i = 0; i < 2; i++) {
            pix = (plane0[0] << 8) + plane0[1];

            *(*dst)++ = (unsigned char)(((pix & 0xf800) >> 11) << 3) | 0x07;
            *(*dst)++ = (unsigned char)((((pix & 0x07e0) >> 5)) << 2) | 0x03;
            *(*dst)++ = (unsigned char)((pix & 0x1f) << 3) | 0x07;

            plane0 += 2;
        }
        break;
    case V4L2_PIX_FMT_RGB565X: /* gggbbbbb rrrrrggg */
        for (i = 0; i < 2; i++) {
            pix = (plane0[1] << 8) + plane0[0];

            *(*dst)++ = (unsigned char)(((pix & 0xf800) >> 11) << 3) | 0x07;
            *(*dst)++ = (unsigned char)((((pix & 0x07e0) >> 5)) << 2) | 0x03;
            *(*dst)++ = (unsigned char)((pix & 0x1f) << 3) | 0x07;

            plane0 += 2;
        }
        break;
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_VYUY:
        y_off = (fourcc == V4L2_PIX_FMT_YUYV || fourcc == V4L2_PIX_FMT_YVYU) ? 0 : 1;
        u_off = (fourcc == V4L2_PIX_FMT_YUYV || fourcc == V4L2_PIX_FMT_UYVY) ? 0 : 2;

        u = plane0[(1 - y_off) + u_off];
        v = plane0[(1 - y_off) + (2 - u_off)];

        for (i = 0; i < 2; i++)
            convert_yuv(enc, plane0[y_off + (i << 1)], u, v, dst);

        break;
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        if (fourcc == V4L2_PIX_FMT_NV12) {
            u = plane1[0];
            v = plane1[1];
        } else {
            u = plane1[1];
            v = plane1[0];
        }

        for (i = 0; i < 2; i++)
            convert_yuv(enc, plane0[i], u, v, dst);

        break;
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
        if (fourcc == V4L2_PIX_FMT_YUV420) {
            u = plane1[0];
            v = plane2[0];
        } else {
            u = plane2[0];
            v = plane1[0];
        }

        for (i = 0; i < 2; i++)
            convert_yuv(enc, plane0[i], u, v, dst);

        break;
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_XRGB32:
        for (i = 0; i < 2; i++) {
            *(*dst)++ = plane0[1];
            *(*dst)++ = plane0[2];
            *(*dst)++ = plane0[3];

            plane0 += 4;
        }
        break;
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_XBGR32:
        for (i = 0; i < 2; i++) {
            *(*dst)++ = plane0[2];
            *(*dst)++ = plane0[1];
            *(*dst)++ = plane0[0];

            plane0 += 4;
        }
        break;
    default:
    case V4L2_PIX_FMT_BGR24:
        for (i = 0; i < 2; i++) {
            *(*dst)++ = plane0[2];
            *(*dst)++ = plane0[1];
            *(*dst)++ = plane0[0];

            plane0 += 3;
        }
        break;
    }
}

static unsigned int convert_to_rgb24(cam_t *cam)
{
    unsigned char *plane0 = cam->pic_buf;
    unsigned char *p_out = cam->tmp;
    uint32_t width = cam->width;
    uint32_t height = cam->height;
    uint32_t bytesperline = cam->bytesperline;
    unsigned char *plane0_start = plane0;
    unsigned char *plane1_start = NULL;
    unsigned char *plane2_start = NULL;
    unsigned char *plane1 = NULL;
    unsigned char *plane2 = NULL;
    enum v4l2_ycbcr_encoding enc;
    unsigned int x, y, depth;
    uint32_t num_planes = 1;
    unsigned char *p_start;
    uint32_t plane0_size;
    uint32_t w_dec = 0;
    uint32_t h_dec = 0;

    if (cam->ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT)
        enc = V4L2_MAP_YCBCR_ENC_DEFAULT(cam->colorspace);
    else
        enc = cam->ycbcr_enc;

    switch (cam->pixformat) {
    case V4L2_PIX_FMT_BGR24:
        depth = 24;
        break;
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_XBGR32:
        depth = 32;
        break;
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        num_planes = 2;
        depth = 8;                              /* Depth of plane 0 */
        h_dec = 1;
        break;
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
        num_planes = 3;
        depth = 8;                              /* Depth of plane 0 */
        h_dec = 1;
        w_dec = 1;
        break;
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_VYUY:
    default:
        depth = 16;
        break;
    }

    p_start = p_out;

    if (num_planes > 1) {
        plane0_size = (width * height * depth) >> 3;
        plane1_start = plane0_start + plane0_size;
    }

    if (num_planes > 2)
        plane2_start = plane1_start + (plane0_size >> (w_dec + h_dec));

    for (y = 0; y < height; y++) {
        plane0 = plane0_start + bytesperline * y;
        if (num_planes > 1)
            plane1 = plane1_start + (bytesperline >> w_dec) * (y >> h_dec);
        if (num_planes > 2)
            plane2 = plane2_start + (bytesperline >> w_dec) * (y >> h_dec);

        for (x = 0; x < width >> 1; x++) {
            copy_two_pixels(cam, enc, plane0, plane1, plane2, &p_out);

            plane0 += depth >> 2;
            if (num_planes > 1)
                plane1 += depth >> (2 + w_dec);
            if (num_planes > 2)
                plane2 += depth >> (2 + w_dec);
        }
    }

    return p_out - p_start;
}

int cam_open(cam_t *cam, int oflag)
{
    if (cam->use_libv4l)
        return v4l2_open(cam->video_dev, oflag);
    else
        return open(cam->video_dev, oflag);
}

int cam_close(cam_t *cam)
{
    if (cam->use_libv4l)
        return v4l2_close(cam->dev);
    else
        return close(cam->dev);
}

unsigned char *cam_read(cam_t *cam)
{
    unsigned char *pic_buf = cam->pic_buf;
    int ret = 0;

    if (cam->read) {
        if (cam->use_libv4l)
            ret = v4l2_read(cam->dev, cam->pic_buf,
                            (cam->width * cam->height * cam->bpp / 8));
        else
            ret = read(cam->dev, cam->pic_buf,
                       (cam->width * cam->height * cam->bpp / 8));
    } else if (cam->userptr) {
            capture_buffers_userptr(cam, cam->pic_buf);
    } else {
            capture_buffers(cam, cam->pic_buf, cam->bytesperline);
    }
    if (ret)
        return NULL;

    if (cam->pixformat != V4L2_PIX_FMT_RGB24) {
        convert_to_rgb24(cam);
        pic_buf = cam->tmp;
    }

    return pic_buf;
}

int cam_ioctl(cam_t *cam, unsigned long cmd, void *arg)
{
    if (cam->use_libv4l)
        return v4l2_ioctl(cam->dev, cmd, arg);
    else
        return ioctl(cam->dev, cmd, arg);
}

int cam_set_control(cam_t *cam, int cid, int value)
{
    struct v4l2_queryctrl qctrl = { .id = cid };
    struct v4l2_control ctrl = { .id = cid };
    int ret;

    if (cam->use_libv4l)
        return v4l2_set_control(cam->dev, cid, value);

    ret = cam_ioctl(cam, VIDIOC_QUERYCTRL, &qctrl);
    if (ret)
            return ret;

    if ((qctrl.flags & V4L2_CTRL_FLAG_DISABLED) ||
        (qctrl.flags & V4L2_CTRL_FLAG_GRABBED))
            return 0;

    if (qctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
        ctrl.value = value ? 1 : 0;
    else
        ctrl.value = ((long long) value * (qctrl.maximum - qctrl.minimum) +
                      32767) / 65535 + qctrl.minimum;

    ret = cam_ioctl(cam, VIDIOC_S_CTRL, &ctrl);

    return ret;
}

int cam_get_control(cam_t *cam, int cid)
{
        struct v4l2_queryctrl qctrl = { .id = cid };
        struct v4l2_control ctrl = { .id = cid };

    if (cam->use_libv4l)
        return v4l2_get_control(cam->dev, cid);

    if (cam_ioctl(cam, VIDIOC_QUERYCTRL, &qctrl))
        return -1;
    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        errno = EINVAL;
        return -1;
    }

    if (cam_ioctl(cam, VIDIOC_G_CTRL, &ctrl))
            return -1;

    return (((long long) ctrl.value - qctrl.minimum) * 65535 +
                        (qctrl.maximum - qctrl.minimum) / 2) /
            (qctrl.maximum - qctrl.minimum);
}

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

static void insert_resolution(cam_t *cam, unsigned int pixformat,
                              unsigned int x, unsigned int y,
                              float max_fps)
{
    unsigned int i;

    try_set_win_info(cam, pixformat, &x, &y);

    if (cam->res) {
        for (i = 0; i < cam->n_res; i++) {
            if (cam->res[i].x == x && cam->res[i].y == y &&
                cam->res[i].pixformat == pixformat)
            return;
        }
    }

    cam->res = realloc(cam->res, (cam->n_res + 1) * sizeof(struct resolutions));

    cam->res[cam->n_res].pixformat = pixformat;
    cam->res[cam->n_res].x = x;
    cam->res[cam->n_res].y = y;
    cam->res[cam->n_res].max_fps = max_fps;

    if (cam->debug == TRUE)
        printf("  Resolution #%d: FOURCC: '%c%c%c%c' (%dx%d %.2f fps)\n",
               cam->n_res,
                pixformat & 0xff,
                (pixformat >> 8) & 0xff,
                (pixformat >> 16) & 0xff,
                pixformat >> 24,
                x, y, (double)max_fps);

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
    if (!r)
         r = (int)b->max_fps - a->max_fps;

    return r;
}

static float get_max_fps_discrete(cam_t *cam,
                                  struct v4l2_frmsizeenum *frmsize)
{
    struct v4l2_frmivalenum frmival = { 0 };
    float fps, max_fps = -1;

    frmival.width = frmsize->discrete.width;
    frmival.height = frmsize->discrete.height;
    frmival.pixel_format = frmsize->pixel_format;
    frmival.index = 0;

    for (frmival.index = 0;
         !cam_ioctl(cam, VIDIOC_ENUM_FRAMEINTERVALS, &frmival);
         frmival.index++) {
            fps = ((float)frmival.discrete.denominator)/frmival.discrete.numerator;
            if (fps > max_fps)
                max_fps = fps;
    }
    return max_fps;
}

void get_supported_resolutions(cam_t *cam, gboolean all_supported)
{
    struct v4l2_fmtdesc fmtdesc = { 0 };
    struct v4l2_format fmt;
    struct v4l2_frmsizeenum frmsize = { 0 };
    int i;
    gboolean has_framesizes = FALSE;
    unsigned int x, y;

    if (cam->n_res) {
        /* Free it, as the resolutions will be re-inserted */
        free(cam->res);
        cam->res = NULL;
        cam->n_res = 0;
    }
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (fmtdesc.index = 0;
         !cam_ioctl(cam, VIDIOC_ENUM_FMT, &fmtdesc);
         fmtdesc.index++) {
        if (!all_supported) {
            if (cam->pixformat != fmtdesc.pixelformat)
                continue;

        } else {
            if (cam->debug == TRUE)
                printf("format index %d: FOURCC: '%c%c%c%c' (%08x)%s\n",
                       fmtdesc.index,
                       fmtdesc.pixelformat & 0xff,
                       (fmtdesc.pixelformat >> 8) & 0xff,
                       (fmtdesc.pixelformat >> 16) & 0xff,
                       fmtdesc.pixelformat >> 24,
                       fmtdesc.pixelformat,
                       fmtdesc.
                       flags & V4L2_FMT_FLAG_EMULATED ? " (emulated)" : "");

            memset(&fmt, 0, sizeof(fmt));
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;
            fmt.fmt.pix.width = 48;
            fmt.fmt.pix.height = 32;

            if (!cam_ioctl(cam, VIDIOC_TRY_FMT, &fmt)) {
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

            if (!cam_ioctl(cam, VIDIOC_TRY_FMT, &fmt)) {
            if (fmt.fmt.pix.width > cam->max_width)
                    cam->max_width = fmt.fmt.pix.width;
            if (fmt.fmt.pix.height > cam->max_height)
                    cam->max_height = fmt.fmt.pix.height;
            if (cam->debug == TRUE)
                    printf("  MAX: %dx%d\n", fmt.fmt.pix.width,
                        fmt.fmt.pix.height);
            }
        }

        if (!is_format_supported(cam, fmtdesc.pixelformat))
            continue;

        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;

        while (!cam_ioctl(cam, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
            has_framesizes = TRUE;

            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    insert_resolution(cam, frmsize.pixel_format,
                                      frmsize.discrete.width,
                                      frmsize.discrete.height,
                                      get_max_fps_discrete(cam, &frmsize));
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                    for (i = 0; i <= 4; i++) {
                        x = frmsize.stepwise.min_width +
                            i * (frmsize.stepwise.max_width -
                                 frmsize.stepwise.min_width) / 4;
                        y = frmsize.stepwise.min_height +
                            i * (frmsize.stepwise.max_height -
                                 frmsize.stepwise.min_height) / 4;
                        insert_resolution(cam, frmsize.pixel_format, x, y, -1);
                    }
            }
            frmsize.index++;
        }

        if (!has_framesizes)
            insert_resolution(cam, fmtdesc.pixelformat,
                              fmt.fmt.pix.width, fmt.fmt.pix.height, -1);
    }

    if (cam->res)
        qsort(cam->res, cam->n_res, sizeof(struct resolutions), sort_func);
}

int camera_cap(cam_t *cam)
{
    char *msg;
    struct v4l2_capability vid_cap = { 0 };

    /* Query device capabilities */
    if (cam_ioctl(cam, VIDIOC_QUERYCAP, &vid_cap) == -1) {

        msg = g_strdup_printf(_("Could not connect to video device (%s).\n"
                                "Please check connection. Error: %d"),
                              cam->video_dev, errno);
        error_dialog(msg);
        g_free(msg);
        return 1;
    }

    /* Query supported resolutions */

    cam->rdir_ok = FALSE;
    cam->min_width = (unsigned)-1;
    cam->min_height = (unsigned)-1;
    cam->max_width = 0;
    cam->max_height = 0;

    get_supported_resolutions(cam, TRUE);

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
        msg = g_strdup_printf(_("Device %s is not a video capture device."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        return 1;
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

    return 0;
}

static int v4l_get_zoom(cam_t *cam)
{
    int i;

    cam->zoom_cid = V4L2_CID_ZOOM_ABSOLUTE;
    i = cam_get_control(cam, cam->zoom_cid);
    if (i >= 0)
        return i;

    cam->zoom_cid = V4L2_CID_ZOOM_RELATIVE;
    i = cam_get_control(cam, cam->zoom_cid);
    if (i >= 0)
        return i;

    cam->zoom_cid = V4L2_CID_ZOOM_CONTINUOUS;
    i = cam_get_control(cam, cam->zoom_cid);
    if (i >= 0)
        return i;

    cam->zoom_cid = -1;
    return -1;
}

void get_pic_info(cam_t *cam)
{
    int i;

    if (cam->debug == TRUE)
        printf("\nVideo control settings:\n");

    i = cam_get_control(cam, V4L2_CID_HUE);
    if (i >= 0) {
        cam->hue = i;
        if (cam->debug == TRUE)
            printf("hue = %d\n", cam->hue);
    } else {
        cam->hue = -1;
    }
    i = cam_get_control(cam, V4L2_CID_SATURATION);
    if (i >= 0) {
        cam->colour = i;
        if (cam->debug == TRUE)
            printf("colour = %d\n", cam->colour);
    } else {
        cam->colour = -1;
    }
    i = v4l_get_zoom(cam);
    if (cam->zoom_cid) {
        cam->zoom = i;
        if (cam->debug == TRUE)
            printf("zoom = %d\n", cam->zoom);
    } else {
        cam->zoom = -1;
    }
    i = cam_get_control(cam, V4L2_CID_CONTRAST);
    if (i >= 0) {
        cam->contrast = i;
        if (cam->debug == TRUE)
            printf("contrast = %d\n", cam->contrast);
    } else {
        cam->contrast = -1;
    }
    i = cam_get_control(cam, V4L2_CID_WHITENESS);
    if (i >= 0) {
        cam->whiteness = i;
        if (cam->debug == TRUE)
            printf("whiteness = %d\n", cam->whiteness);
    } else {
        cam->whiteness = -1;
    }
    i = cam_get_control(cam, V4L2_CID_BRIGHTNESS);
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

    if (cam_ioctl(cam, VIDIOC_G_FMT, &fmt)) {
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

    if (is_format_supported(cam, fmt.fmt.pix.pixelformat)) {
        cam->pixformat = fmt.fmt.pix.pixelformat;
        cam->bpp = ((fmt.fmt.pix.bytesperline << 3) + (fmt.fmt.pix.width - 1)) / fmt.fmt.pix.width;
        cam->width = fmt.fmt.pix.width;
        cam->height = fmt.fmt.pix.height;
        cam->bytesperline = fmt.fmt.pix.bytesperline;
        cam->sizeimage = fmt.fmt.pix.sizeimage;
    }
}

void try_set_win_info(cam_t *cam, unsigned int pixformat,
                      unsigned int *x, unsigned int *y)
{
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = pixformat;
    fmt.fmt.pix.width = *x;
    fmt.fmt.pix.height = *y;
    if (!cam_ioctl(cam, VIDIOC_TRY_FMT, &fmt)) {
        *x = fmt.fmt.pix.width;
        *y = fmt.fmt.pix.height;
    }
}

void set_win_info(cam_t *cam)
{
    struct v4l2_format fmt;
    gchar *msg;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* Get current settings, apply our changes and try the new setting */
    if (cam_ioctl(cam, VIDIOC_G_FMT, &fmt)) {
        if (cam->debug) {
            g_message("VIDIOC_G_FMT  --  could not get window info, exiting....");
        }
        msg = g_strdup_printf(_("Could not connect to video device (%s).\nPlease check connection."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    if (!cam->n_res) {
        msg = g_strdup_printf(_("Device video formats are incompatible with camorama."));
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    cam->pixformat = cam->res[0].pixformat;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = cam->pixformat;
    fmt.fmt.pix.width = cam->width;
    fmt.fmt.pix.height = cam->height;
    if (cam_ioctl(cam, VIDIOC_S_FMT, &fmt)) {
        if (cam->debug)
            g_message("VIDIOC_S_FMT  --  could not set window info, exiting....");

        msg = g_strdup_printf(_("Could not set window info on video device (%s).\nPlease check connection."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }
//    cam->pixformat = fmt.fmt.pix.pixelformat;
    cam->colorspace = fmt.fmt.pix.colorspace;
    cam->ycbcr_enc = fmt.fmt.pix.ycbcr_enc;

    /* Check if returned format is valid */
    if (!is_format_supported(cam, cam->pixformat)) {
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
        msg = g_strdup_printf(_("Could not set format to %c%c%c%c on video device (%s)."),
                              cam->pixformat & 0xff,
                              (cam->pixformat >> 8) & 0xff,
                              (cam->pixformat >> 16) & 0xff,
                              cam->pixformat >> 24,
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    /* Resolution may have changed. Store the retrieved one */
    cam->pixformat = fmt.fmt.pix.pixelformat;
    cam->bytesperline = fmt.fmt.pix.bytesperline;
    cam->sizeimage = fmt.fmt.pix.sizeimage;
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
    if (cam_ioctl(cam, VIDIOC_REQBUFS, &cam->req)) {
        msg = g_strdup_printf(_("VIDIOC_REQBUFS  --  could not request buffers (%s), exiting...."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    cam->buffers = calloc(cam->req.count, sizeof(*cam->buffers));
    if (!cam->buffers) {
        msg = g_strdup_printf(_("could not allocate memory for video buffers, exiting...."));
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    for (cam->n_buffers = 0;
         cam->n_buffers < cam->req.count;
         ++cam->n_buffers) {
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = cam->n_buffers;

        if (cam_ioctl(cam, VIDIOC_QUERYBUF, &buf)) {
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
        if (cam_ioctl(cam, VIDIOC_QBUF, &buf)) {
            msg = g_strdup_printf(_("VIDIOC_QBUF  --  could not enqueue buffers (%s), exiting...."),
                                  cam->video_dev);
            error_dialog(msg);
            g_free(msg);
            exit(0);
        }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (cam_ioctl(cam, VIDIOC_STREAMON, &type)) {
        msg = g_strdup_printf(_("failed to start streaming (%s), exiting...."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }
}

void start_streaming_userptr(cam_t *cam)
{
    char *msg;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    memset(&cam->req, 0, sizeof(cam->req));
    cam->req.count = 2;
    cam->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam->req.memory = V4L2_MEMORY_USERPTR;
    if (cam_ioctl(cam, VIDIOC_REQBUFS, &cam->req)) {
        msg = g_strdup_printf(_("VIDIOC_REQBUFS  --  could not request buffers (%s), exiting...."),
                              cam->video_dev);
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    cam->buffers = calloc(cam->req.count, sizeof(*cam->buffers));
    if (!cam->buffers) {
        msg = g_strdup_printf(_("could not allocate memory for video buffers, exiting...."));
        error_dialog(msg);
        g_free(msg);
        exit(0);
    }

    for (cam->n_buffers = 0;
         cam->n_buffers < cam->req.count;
         ++cam->n_buffers) {
        memset(&buf, 0, sizeof(buf));

        cam->buffers[cam->n_buffers].length = cam->sizeimage;
        cam->buffers[cam->n_buffers].start = calloc(1, cam->sizeimage);

        if (!cam->buffers[cam->n_buffers].start) {
            msg = g_strdup_printf(_("could not allocate memory for video buffers, exiting...."));
            error_dialog(msg);
            g_free(msg);
            exit(0);
        }

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = cam->n_buffers;
        buf.length = cam->buffers[cam->n_buffers].length;
        buf.m.userptr = (unsigned long)cam->buffers[cam->n_buffers].start;

        if (cam_ioctl(cam, VIDIOC_QBUF, &buf)) {
            msg = g_strdup_printf(_("VIDIOC_QBUF  --  could not query buffers (%s), exiting...."),
                                  cam->video_dev);
            error_dialog(msg);
            g_free(msg);
            exit(0);
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (cam_ioctl(cam, VIDIOC_STREAMON, &type)) {
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
    cam_ioctl(cam, VIDIOC_DQBUF, &buf);

    if (len > buf.bytesused)
        len = buf.bytesused;

    inbuf = cam->buffers[buf.index].start;
    for (y = 0; y < cam->height; y++) {
        memcpy(outbuf, inbuf, cam->width * cam->bpp / 8);
        outbuf += cam->width * cam->bpp / 8;
        inbuf += cam->bytesperline;
    }

    cam_ioctl(cam, VIDIOC_QBUF, &buf);
}

void capture_buffers_userptr(cam_t *cam, unsigned char *outbuf)
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
    buf.memory = V4L2_MEMORY_USERPTR;
    cam_ioctl(cam, VIDIOC_DQBUF, &buf);

    inbuf = cam->buffers[buf.index].start;
    for (y = 0; y < cam->height; y++) {
        memcpy(outbuf, inbuf, cam->width * cam->bpp / 8);
        outbuf += cam->width * cam->bpp / 8;
        inbuf += cam->bytesperline;
    }

    cam_ioctl(cam, VIDIOC_QBUF, &buf);
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
            if (cam_ioctl(cam, VIDIOC_DQBUF, &buf))
                break;
        }
    };

    /* Streams off */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (cam_ioctl(cam, VIDIOC_STREAMOFF, &type)) {
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
    cam_ioctl(cam, VIDIOC_REQBUFS, &cam->req);

    free(cam->buffers);
    cam->buffers = NULL;
}

void stop_streaming_userptr(cam_t *cam)
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
            buf.memory = V4L2_MEMORY_USERPTR;
            if (cam_ioctl(cam, VIDIOC_DQBUF, &buf))
                break;
        }
    };

    /* Streams off */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (cam_ioctl(cam, VIDIOC_STREAMOFF, &type)) {
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
    cam_ioctl(cam, VIDIOC_REQBUFS, &cam->req);

    free(cam->buffers);
    cam->buffers = NULL;
}
