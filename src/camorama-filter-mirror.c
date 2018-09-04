/* This file is part of camorama
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2006  Sven Herzberg <herzi@gnome-de.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "filter.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib/gi18n.h>

/* GType stuff for CamoramaFilterMirror */
typedef struct _CamoramaFilter CamoramaFilterMirror;
typedef struct _CamoramaFilterClass CamoramaFilterMirrorClass;

G_DEFINE_TYPE(CamoramaFilterMirror, camorama_filter_mirror,
              CAMORAMA_TYPE_FILTER);

static void camorama_filter_mirror_init(CamoramaFilterMirror *self)
{
}

static void camorama_filter_mirror_filter(void *filter, guchar *image,
                                          gint width, gint height,
                                          gint depth)
{
    gint x, z, row_length, image_length, new_row, next_row, half_row;
    gint index1, index2;
    guchar temp;

    row_length = width * depth;
    image_length = row_length * height;
    half_row = ((width + 1) / 2) * depth;

    // 0, 320*depth, 640*depth, 960*depth, ...
    for (new_row = 0; new_row < image_length; new_row = next_row) {
        next_row = new_row + row_length;
        for (x = 0; x < half_row; x += depth) { // 0, 3, 6, ..., 160*depth
            index1 = new_row + x;   //   0,   3,   6, ..., 160*depth
            index2 = next_row - x - depth;  // 320, 319, 318, ..., 161*depth
            for (z = 0; z < depth; z++) {   // 0, ..., depth
                    temp = image[index1 + z];
                    image[index1 + z] = image[index2 + z];
                    image[index2 + z] = temp;
            }
        }
    }
}

static void
camorama_filter_mirror_class_init(CamoramaFilterMirrorClass *self_class)
{
    self_class->filter = camorama_filter_mirror_filter;
    // TRANSLATORS: This is a noun
    self_class->name = _("Mirror");
}
