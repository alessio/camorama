#include "filter.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib/gi18n.h>

gchar const*
camorama_filter_get_name(CamoramaFilter* self) {
	gchar const* name = CAMORAMA_FILTER_GET_CLASS(self)->name;
	g_return_val_if_fail(name, G_OBJECT_TYPE_NAME(self));
	return _(name);
}

void
camorama_filter_apply(CamoramaFilter* self, guchar* image, gint width, gint height, gint depth) {
	g_return_if_fail(CAMORAMA_FILTER_GET_CLASS(self)->filter);

	CAMORAMA_FILTER_GET_CLASS(self)->filter(self, image, width, height, depth);
}

/* GType stuff ifor CamoramaFilter */
G_DEFINE_ABSTRACT_TYPE(CamoramaFilter, camorama_filter, G_TYPE_OBJECT);

static void
camorama_filter_init(CamoramaFilter* self) {}

static void
camorama_filter_class_init(CamoramaFilterClass* self_class) {}

#include "v4l.h"

static inline void move_420_block (int yTL, int yTR, int yBL, int yBR, int u,
                                   int v, int rowPixels, unsigned char *rgb,
                                   int bits);

/*these two functions are borrowed from the ov511 driver, with the author, 
 *Mark McClelland's kind encoragement*/

/* LIMIT: convert a 16.16 fixed-point value to a byte, with clipping. */
#define LIMIT(x) ((x)>0xffffff?0xff: ((x)<=0xffff?0:((x)>>16)))

void
yuv420p_to_rgb (unsigned char *image, unsigned char *temp, int x, int y, int z) {
    const int numpix = x * y;
    const int bytes = z;        /* (z*8) >> 3; */
    int i, j, y00, y01, y10, y11, u, v;
    unsigned char *pY = image;
    unsigned char *pU = pY + numpix;
    unsigned char *pV = pU + numpix / 4;
    unsigned char *image2 = temp;
    if(FALSE) // FIXME: make TRUE to add debugging in here
	g_print("%s\n", "yuv420p->rgb");
    for (j = 0; j <= y - 2; j += 2) {
        for (i = 0; i <= x - 2; i += 2) {
            y00 = *pY;
            y01 = *(pY + 1);
            y10 = *(pY + x);
            y11 = *(pY + x + 1);
            u = (*pU++) - 128;
            v = (*pV++) - 128;

                move_420_block (y00, y01, y10, y11, u, v, x, image2, z * 8);

                pY += 2;
                image2 += 2 * bytes;
            }
            pY += x;
            image2 += x * bytes;
        }
}

static inline void
move_420_block (int yTL, int yTR, int yBL, int yBR, int u, int v,
                int rowPixels, unsigned char *rgb, int bits)
{
    const int rvScale = 91881;
    const int guScale = -22553;
    const int gvScale = -46801;
    const int buScale = 116129;
    const int yScale = 65536;
    int r, g, b;

    g = guScale * u + gvScale * v;
    if (1) {
        r = buScale * u;
        b = rvScale * v;
    } else {
        r = rvScale * v;
        b = buScale * u;
    }

    yTL *= yScale;
    yTR *= yScale;
    yBL *= yScale;
    yBR *= yScale;

    if (bits == 24) {
        /* Write out top two pixels */
        rgb[0] = LIMIT (b + yTL);
        rgb[1] = LIMIT (g + yTL);
        rgb[2] = LIMIT (r + yTL);

        rgb[3] = LIMIT (b + yTR);
        rgb[4] = LIMIT (g + yTR);
        rgb[5] = LIMIT (r + yTR);

        /* Skip down to next line to write out bottom two pixels */
        rgb += 3 * rowPixels;
        rgb[0] = LIMIT (b + yBL);
        rgb[1] = LIMIT (g + yBL);
        rgb[2] = LIMIT (r + yBL);

        rgb[3] = LIMIT (b + yBR);
        rgb[4] = LIMIT (g + yBR);
        rgb[5] = LIMIT (r + yBR);
    } else if (bits == 16) {
        /* Write out top two pixels */
        rgb[0] = ((LIMIT (b + yTL) >> 3) & 0x1F)
            | ((LIMIT (g + yTL) << 3) & 0xE0);
        rgb[1] = ((LIMIT (g + yTL) >> 5) & 0x07)
            | (LIMIT (r + yTL) & 0xF8);

        rgb[2] = ((LIMIT (b + yTR) >> 3) & 0x1F)
            | ((LIMIT (g + yTR) << 3) & 0xE0);
        rgb[3] = ((LIMIT (g + yTR) >> 5) & 0x07)
            | (LIMIT (r + yTR) & 0xF8);

        /* Skip down to next line to write out bottom two pixels */
        rgb += 2 * rowPixels;

        rgb[0] = ((LIMIT (b + yBL) >> 3) & 0x1F)
            | ((LIMIT (g + yBL) << 3) & 0xE0);
        rgb[1] = ((LIMIT (g + yBL) >> 5) & 0x07)
            | (LIMIT (r + yBL) & 0xF8);

        rgb[2] = ((LIMIT (b + yBR) >> 3) & 0x1F)
            | ((LIMIT (g + yBR) << 3) & 0xE0);
        rgb[3] = ((LIMIT (g + yBR) >> 5) & 0x07)
            | (LIMIT (r + yBR) & 0xF8);
    }
}

/* GType stuff for CamoramaFilterColor */
typedef struct _CamoramaFilter      CamoramaFilterColor;
typedef struct _CamoramaFilterClass CamoramaFilterColorClass;

G_DEFINE_TYPE(CamoramaFilterColor, camorama_filter_color, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_color_init(CamoramaFilterColor* self) {}

static void
camorama_filter_color_filter(CamoramaFilterColor* filter, guchar *image, int x, int y, int depth) {
	int i;
	char tmp;
	i = x * y;
	while (--i) {
		tmp = image[0];
		image[0] = image[2];
		image[2] = tmp;
		image += 3;
	}
}

static void
camorama_filter_color_class_init(CamoramaFilterColorClass* self_class) {
	self_class->filter = camorama_filter_color_filter;
	self_class->name   = _("Color Correction");
}

/* GType stuff for CamoramaFilterInvert */
typedef struct _CamoramaFilter      CamoramaFilterInvert;
typedef struct _CamoramaFilterClass CamoramaFilterInvertClass;

G_DEFINE_TYPE(CamoramaFilterInvert, camorama_filter_invert, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_invert_init(CamoramaFilterInvert* self) {}

static void
camorama_filter_invert_filter(CamoramaFilter* filter, guchar *image, int x, int y, int depth) {
#warning "FIXME: add a checking cast here"
	CamoramaFilterInvert* self = (CamoramaFilterInvert*)filter;
	int i;
	for (i = 0; i < x * y * depth; i++) {
		image[i] = 255 - image[i];
	}
}

static void
camorama_filter_invert_class_init(CamoramaFilterClass* self_class) {
	self_class->filter = camorama_filter_invert_filter;
	self_class->name   = _("Invert");
}

/* GType stuff for CamoramaFilterThreshold */
typedef struct _CamoramaFilterThreshold {
	CamoramaFilter base_instance;
	gint threshold;
} CamoramaFilterThreshold;
typedef struct _CamoramaFilterClass CamoramaFilterThresholdClass;

G_DEFINE_TYPE(CamoramaFilterThreshold, camorama_filter_threshold, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_threshold_init(CamoramaFilterThreshold* self) {
	self->threshold = 127;
}

static void
camorama_filter_threshold_filter(CamoramaFilter* filter, guchar *image, int x, int y, int depth) {
#warning "FIXME: cast"
    CamoramaFilterThreshold* self = (CamoramaFilterThreshold*)filter;
    int i;
    for (i = 0; i < x * y; i++) {
        if ((image[0] + image[1] + image[2]) > (self->threshold * 3)) {
            image[0] = 255;
            image[1] = 255;
            image[2] = 255;
        } else {
            image[0] = 0;
            image[1] = 0;
            image[2] = 0;
        }
        image += 3;
    }
}

static void
camorama_filter_threshold_class_init(CamoramaFilterThresholdClass* self_class) {
	self_class->filter = camorama_filter_threshold_filter;
	self_class->name   = _("Threshold (Overall)");
}

/* GType stuff for CamoramaFilterThresholdChannel */
typedef struct _CamoramaFilterThreshold CamoramaFilterThresholdChannel;
typedef struct _CamoramaFilterClass     CamoramaFilterThresholdChannelClass;

G_DEFINE_TYPE(CamoramaFilterThresholdChannel, camorama_filter_threshold_channel, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_threshold_channel_init(CamoramaFilterThresholdChannel* self) {
	self->threshold = 127;
}

static void
camorama_filter_threshold_channel_filter(CamoramaFilter* filter, unsigned char *image, int x, int y, int depth) {
#warning "FIXME: cast"
    CamoramaFilterThresholdChannel* self = (CamoramaFilterThresholdChannel*)filter;
    int i;
    for (i = 0; i < x * y; i++) {
        if (image[0] > self->threshold) {
            image[0] = 255;
        } else {
            image[0] = 0;
        }
        if (image[1] > self->threshold) {
            image[1] = 255;
        } else {
            image[1] = 0;
        }
        if (image[2] > self->threshold) {
            image[2] = 255;
        } else {
            image[2] = 0;
        }
        image += 3;
    }
}

static void
camorama_filter_threshold_channel_class_init(CamoramaFilterThresholdChannelClass* self_class) {
	self_class->filter = camorama_filter_threshold_channel_filter;
	self_class->name   = _("Threshold (Per Channel)");
}

/* GType stuff for CamoramaFilterWacky */
typedef struct _CamoramaFilter      CamoramaFilterWacky;
typedef struct _CamoramaFilterClass CamoramaFilterWackyClass;

G_DEFINE_TYPE(CamoramaFilterWacky, camorama_filter_wacky, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_wacky_init(CamoramaFilterWacky* self) {}

static void
camorama_filter_wacky_filter(CamoramaFilter* filter, unsigned char *image, int x, int y, int depth) {
    int i;
    int neighbours;
    int total;
    unsigned char *image2, *image3;
    image2 = (unsigned char *) malloc (sizeof (unsigned char) * x * y * depth);
    memcpy (image2, image, x * y * depth);
    image3 = image2;

    for (i = 0; i < x * y; i++) {
        total = 0;
        neighbours = 0;

        if (i < x * depth) {
            /*we are in the top row */
        } else {
            image2 -= (x + 1) * depth;
            total = total + ((1 / 6) * image2[0]);
            image2 += depth;
            total = total + ((4 / 6) * image2[0]);
            image2 += depth;
            total = total + ((1 / 6) * image2[0]);
            neighbours = neighbours + depth;
            image2 += (x - 1) * depth;
        }
        if (i > x * (y - 1) * depth) {
            /*we are in the bottom row */
        } else {
            image2 += (x + 1) * depth;
            total = total + ((1 / 6) * image2[0]);
            image2 -= depth;
            total = total + ((4 / 6) * image2[0]);
            image2 -= depth;
            total = total + ((1 / 6) * image2[0]);
            image2 -= (x - 1) * depth;
            neighbours = neighbours + depth;
        }

        image2 += depth;
        total = total + ((4 / 6) * image2[0]);
        image2 -= depth;
        neighbours++;

        image2 -= depth;
        total = total + ((4 / 6) * image2[0]);
        image2 += depth;
        neighbours++;

        image[0] = image[0] * (-20 / 6);
        image[0] = image[0] + total;
        image[1] = image[0];
        image[2] = image[0];
        image += depth;
    }
    free (image2);
}

static void
camorama_filter_wacky_class_init(CamoramaFilterWackyClass* self_class) {
	self_class->filter = camorama_filter_wacky_filter;
	self_class->name   = _("Wacky");
}

/* GType stuff for CamoramaFilterSmotth */
typedef struct _CamoramaFilter      CamoramaFilterSmooth;
typedef struct _CamoramaFilterClass CamoramaFilterSmoothClass;

G_DEFINE_TYPE(CamoramaFilterSmooth, camorama_filter_smooth, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_smooth_init(CamoramaFilterSmooth* self) {}

static void
camorama_filter_smooth_filter(CamoramaFilter* filter, guchar *image, int x, int y, int depth) {
    int i;
    int neighbours;
    int total0, total1, total2;
    unsigned char *image2, *image3;
    int tr = 0, br = 0;

    image2 = (unsigned char *) malloc (sizeof (unsigned char) * x * y * depth);
    memcpy (image2, image, x * y * depth);
    image3 = image2;

    for (i = 0; i < x * y; i++) {
        total0 = 0;
        total1 = 0;
        total2 = 0;
        neighbours = 0;

        if (i < x) {
            /*we are in the top row */
            tr++;
        } else {
            image2 -= (x + 1) * depth;
            total0 = total0 + image2[0];
            total1 = total1 + image2[1];
            total2 = total2 + image2[2];

            total0 = total0 + image2[3];
            total1 = total1 + image2[4];
            total2 = total2 + image2[5];

            total0 = total0 + image2[6];
            total1 = total1 + image2[7];
            total2 = total2 + image2[8];
            neighbours = neighbours + depth;
            if (tr > 1) {
                tr = 0;
            }
            image2 += (x + 1) * depth;
        }
        if (i > x * (y - 1)) {
            br++;
            /*we are in the bottom row */
        } else {
            image2 += (x - 1) * depth;
            total0 = total0 + image2[0];
            total1 = total1 + image2[1];
            total2 = total2 + image2[2];

            total0 = total0 + image2[3];
            total1 = total1 + image2[4];
            total2 = total2 + image2[5];

            total0 = total0 + image2[6];
            total1 = total1 + image2[7];
            total2 = total2 + image2[8];

            image2 -= (x - 1) * depth;

            neighbours = neighbours + depth;
        }

        image2 += 3;
        total0 = total0 + image2[0];
        total1 = total1 + image2[1];
        total2 = total2 + image2[2];
        image2 -= 3;
        neighbours++;

        image2 -= 3;
        total0 = total0 + image2[0];
        total1 = total1 + image2[1];
        total2 = total2 + image2[2];
        image2 += 3;
        neighbours++;

        image[0] = (int) (total0 / neighbours);
        image[1] = (int) (total1 / neighbours);
        image[2] = (int) (total2 / neighbours);

        image += 3;
        image2 += 3;
    }
    free (image3);
}

static void
camorama_filter_smooth_class_init(CamoramaFilterSmoothClass* self_class) {
	self_class->filter = camorama_filter_smooth_filter;
	self_class->name   = _("Smooth");
}

/* GType for CamoramaFilterMono */
typedef struct _CamoramaFilter      CamoramaFilterMono;
typedef struct _CamoramaFilterClass CamoramaFilterMonoClass;

G_DEFINE_TYPE(CamoramaFilterMono, camorama_filter_mono, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_mono_init(CamoramaFilterMono* self) {}

static void
camorama_filter_mono_filter(CamoramaFilter* filter, unsigned char *image, int x, int y, int depth) {
    int i;
    int total, avg;

    for (i = 0; i < x * y; i++) {
        total = image[0] + image[1] + image[2];
        avg = (int) (total / 3);

        image[0] = avg;
        image[1] = avg;
        image[2] = avg;
        image += 3;
    }
}

static void
camorama_filter_mono_class_init(CamoramaFilterMonoClass* self_class) {
	self_class->filter = camorama_filter_mono_filter;
	self_class->name   = _("Monochrome");
}

/* GType for CamoramaFilterMonoWeight */
typedef struct _CamoramaFilter      CamoramaFilterMonoWeight;
typedef struct _CamoramaFilterClass CamoramaFilterMonoWeightClass;

G_DEFINE_TYPE(CamoramaFilterMonoWeight, camorama_filter_mono_weight, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_mono_weight_init(CamoramaFilterMonoWeight* self) {}

static void
camorama_filter_mono_weight_filter(CamoramaFilter* filter, unsigned char *image, int x, int y, int depth) {
    int i;
    int avg;
    for (i = 0; i < x * y; i++) {
        avg = (int) ((image[0] * 0.2125) + (image[1] * 0.7154) +
                     (image[2] * 0.0721));

        image[0] = avg;
        image[1] = avg;
        image[2] = avg;
        image += 3;             /* bump to next triplet */
    }
}

static void
camorama_filter_mono_weight_class_init(CamoramaFilterMonoWeightClass* self_class) {
	self_class->filter = camorama_filter_mono_weight_filter;
	self_class->name   = _("Monochrome (Weight)");
}

/* GType stuff for CamoramaFilterSobel */
typedef struct _CamoramaFilter      CamoramaFilterSobel;
typedef struct _CamoramaFilterClass CamoramaFilterSobelClass;

G_DEFINE_TYPE(CamoramaFilterSobel, camorama_filter_sobel, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_sobel_init(CamoramaFilterSobel* self) {}

/* fix this at some point, very slow */
static void
camorama_filter_sobel_filter(CamoramaFilter* filter, unsigned char *image, int x, int y, int depth) {
    int i, j, grad[3];
    int deltaX[3], deltaY[3];
    int width = x * 3;
    guchar *image2;

    image2 = (guchar *) malloc (sizeof (guchar) * (x * y * 3));

    for (i = width; i < (y - 1) * width; i++) {
        for (j = 0; j <= 2; j++) {
            deltaX[j] =
                2 * image[i + 1] + image[i - width + 1] +
                image[i + width + 1] - 2 * image[i - 1] -
                image[i - width - 1] - image[i + width - 1];

            deltaY[j] =
                image[i - width - 1] + 2 * image[i -
                                                 width] +
                image[i - width + 1] - image[i + width -
                                             1] -
                2 * image[i + width] - image[i + width + 1];
            grad[j] = (abs (deltaX[j]) + abs (deltaY[j]));
            grad[j] = grad[j] / 5.66;   /* <<<<<------------------------ new line */
            if (grad[j] > 255) {
                grad[j] = 255;
            }
            image2[i + j] = (unsigned char) grad[j];
        }
    }

    memcpy (image, image2, (x * y * 3));
    free (image2);
}

static void
camorama_filter_sobel_class_init(CamoramaFilterSobelClass* self_class) {
	self_class->filter = camorama_filter_sobel_filter;
	// TRANSLATORS: http://en.wikipedia.org/wiki/Sobel
	self_class->name   = _("Sobel");
}

/* general filter initialization */
void
camorama_filters_init(void) {
	camorama_filter_color_get_type();
	camorama_filter_invert_get_type();
	camorama_filter_threshold_get_type();
	camorama_filter_threshold_channel_get_type();
	camorama_filter_mirror_get_type();
	camorama_filter_wacky_get_type();
	camorama_filter_smooth_get_type();
	camorama_filter_laplace_get_type();
	camorama_filter_mono_get_type();
	camorama_filter_mono_weight_get_type();
	camorama_filter_sobel_get_type();
}

