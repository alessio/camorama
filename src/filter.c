#include "v4l.h"

static inline void move_420_block (int yTL, int yTR, int yBL, int yBR, int u,
                                   int v, int rowPixels, unsigned char *rgb,
                                   int bits);

/*these two functions are borrowed from the ov511 driver, with the author, 
 *Mark McClelland's kind encoragement*/

/* LIMIT: convert a 16.16 fixed-point value to a byte, with clipping. */
#define LIMIT(x) ((x)>0xffffff?0xff: ((x)<=0xffff?0:((x)>>16)))

void
yuv420p_to_rgb (unsigned char *image, unsigned char *temp, int x, int y,
                int z)
{
    const int numpix = x * y;
    const int bytes = z;        /* (z*8) >> 3; */
    int i, j, y00, y01, y10, y11, u, v;
    unsigned char *pY = image;
    unsigned char *pU = pY + numpix;
    unsigned char *pV = pU + numpix / 4;
    unsigned char *image2 = temp;

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

void fix_colour (char *image, int x, int y)
{
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

void negative (unsigned char *image, int x, int y, int z)
{
    int i;
    for (i = 0; i < x * y * z; i++) {
        image[i] = 255 - image[i];

    }
}

void threshold (unsigned char *image, int x, int y, int threshold_value)
{
    int i;
    for (i = 0; i < x * y; i++) {
        if ((image[0] + image[1] + image[2]) > (threshold_value * 3)) {
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

void
threshold_channel (unsigned char *image, int x, int y, int threshold_value)
{
    int i;
    for (i = 0; i < x * y; i++) {
        if (image[0] > threshold_value) {
            image[0] = 255;
        } else {
            image[0] = 0;
        }
        if (image[1] > threshold_value) {
            image[1] = 255;
        } else {
            image[1] = 0;
        }
        if (image[2] > threshold_value) {
            image[2] = 255;
        } else {
            image[2] = 0;
        }
        image += 3;
    }
}

void mirror (unsigned char *image, int x, int y, int z)
{
    int i, j, k;
    unsigned char *image2;

    image2 = (char *) malloc (sizeof (unsigned char) * x * y * z);
    memcpy (image2, image, x * y * z);

    for (i = 0; i < y; i++) {
        for (j = 0; j < x; j++) {
            for (k = 0; k < z; k++) {
                /*ow, my brain! */
                image[(i * x * z) + (j * z) + k] =
                    image2[(i * x * z) - (j * z) + k];
            }

        }

    }

    free (image2);

}

void wacky (unsigned char *image, int z, int x, int y)
{
    int i;
    int neighbours;
    int total;
    unsigned char *image2, *image3;
    image2 = (unsigned char *) malloc (sizeof (unsigned char) * x * y * z);
    memcpy (image2, image, x * y * z);
    image3 = image2;

    for (i = 0; i < x * y; i++) {
        total = 0;
        neighbours = 0;

        if (i < x * z) {
            /*we are in the top row */
        } else {
            image2 -= (x + 1) * z;
            total = total + ((1 / 6) * image2[0]);
            image2 += z;
            total = total + ((4 / 6) * image2[0]);
            image2 += z;
            total = total + ((1 / 6) * image2[0]);
            neighbours = neighbours + z;
            image2 += (x - 1) * z;
        }
        if (i > x * (y - 1) * z) {
            /*we are in the bottom row */
        } else {
            image2 += (x + 1) * z;
            total = total + ((1 / 6) * image2[0]);
            image2 -= z;
            total = total + ((4 / 6) * image2[0]);
            image2 -= z;
            total = total + ((1 / 6) * image2[0]);
            image2 -= (x - 1) * z;
            neighbours = neighbours + z;
        }

        image2 += z;
        total = total + ((4 / 6) * image2[0]);
        image2 -= z;
        neighbours++;

        image2 -= z;
        total = total + ((4 / 6) * image2[0]);
        image2 += z;
        neighbours++;

        image[0] = image[0] * (-20 / 6);
        image[0] = image[0] + total;
        image[1] = image[0];
        image[2] = image[0];
        image += z;
    }
    free (image2);
}

void smooth (unsigned char *image, int z, int x, int y)
{
    int i;
    int neighbours;
    int total0, total1, total2;
    unsigned char *image2, *image3;
    int tr = 0, br = 0;

    image2 = (unsigned char *) malloc (sizeof (unsigned char) * x * y * z);
    memcpy (image2, image, x * y * z);
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
            image2 -= (x + 1) * z;
            total0 = total0 + image2[0];
            total1 = total1 + image2[1];
            total2 = total2 + image2[2];

            total0 = total0 + image2[3];
            total1 = total1 + image2[4];
            total2 = total2 + image2[5];

            total0 = total0 + image2[6];
            total1 = total1 + image2[7];
            total2 = total2 + image2[8];
            neighbours = neighbours + z;
            if (tr > 1) {
                tr = 0;
            }
            image2 += (x + 1) * z;
        }
        if (i > x * (y - 1)) {
            br++;
            /*we are in the bottom row */
        } else {
            image2 += (x - 1) * z;
            total0 = total0 + image2[0];
            total1 = total1 + image2[1];
            total2 = total2 + image2[2];

            total0 = total0 + image2[3];
            total1 = total1 + image2[4];
            total2 = total2 + image2[5];

            total0 = total0 + image2[6];
            total1 = total1 + image2[7];
            total2 = total2 + image2[8];

            image2 -= (x - 1) * z;

            neighbours = neighbours + z;
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

void laplace (unsigned char *image, int z, int x, int y)
{
    int i;
    int neighbours;
    int total0, total1, total2;
    unsigned char *image2, *image3;
	
    image2 = (unsigned char *) malloc (sizeof (unsigned char) * x * y * z);
    memcpy (image2, image, x * y * z);
    image3 = image2;

    for (i = 1; i < x * (y - 1); i++) {
        total0 = 0;
        total1 = 0;
        total2 = 0;
        neighbours = 0;

        image2 -= (x + 1) * 3;
        total0 = total0 + image2[0];
        total1 = total1 + image2[1];
        total2 = total2 + image2[2];

        total0 = total0 + image2[3];
        total1 = total1 + image2[4];
        total2 = total2 + image2[5];

        total0 = total0 + image2[6];
        total1 = total1 + image2[7];
        total2 = total2 + image2[8];

        image2 += (x + 1) * 3;

        image2 += (x - 1) * 3;
        total0 = total0 + image2[0];
        total1 = total1 + image2[1];
        total2 = total2 + image2[2];

        total0 = total0 + image2[3];
        total1 = total1 + image2[4];
        total2 = total2 + image2[5];

        total0 = total0 + image2[6];
        total1 = total1 + image2[7];
        total2 = total2 + image2[8];

        image2 -= (x - 1) * 3;

        image2 += 3;
        total0 = total0 + image2[0];
        total1 = total1 + image2[1];
        total2 = total2 + image2[2];
        image2 -= 3;

        image2 -= 3;
        total0 = total0 + image2[0];
        total1 = total1 + image2[1];
        total2 = total2 + image2[2];
        image2 += 3;

        if (image[0] * 8 < total0) {
            image[0] = 0;
        } else {
            image[0] = (image[0] * 8) - total0;
        }
        if (image[1] * 8 < total1) {
            image[1] = 0;
        } else {
            image[1] = (image[1] * 8) - total1;
        }
        if (image[2] * 8 < total2) {
            image[2] = 0;
        } else {
            image[2] = (image[2] * 8) - total2;
        }

        image += 3;
        image2 += 3;
    }

    free (image3);

}

void bw (unsigned char *image, int x, int y)
{
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
void bw2 (unsigned char *image, int x, int y)
{
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

/* fix this at some point, very slow */
void sobel (unsigned char *image, int x, int y)
{
    int i, j, grad[3];
    int deltaX[3], deltaY[3];
    int width = x * 3;
    unsigned char *image2;

    image2 = (char *) malloc (sizeof (unsigned char) * (x * y * 3));

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
