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

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

/* GType stuff for CamoramaFilterLaplace */
typedef struct _CamoramaFilter      CamoramaFilterLaplace;
typedef struct _CamoramaFilterClass CamoramaFilterLaplaceClass;

G_DEFINE_TYPE(CamoramaFilterLaplace, camorama_filter_laplace, CAMORAMA_TYPE_FILTER);

static void
camorama_filter_laplace_init(CamoramaFilterLaplace* self) {}

static void
camorama_filter_laplace_filter(void *filter, guchar *image, int width, int height, int depth) {
    int x, y;
    unsigned char *image2;
	
    image2 = (unsigned char *) malloc (sizeof (unsigned char) * width * height * depth);
    memcpy (image2, image, width * height * depth);

    y = 0;
    // FIXME: the fist pixel
    for(x = 1; x < width - 1; x++) {
	    int offsets[6] = {
		 y      * width * depth + x * depth - depth,
		 y      * width * depth + x * depth,
		 y      * width * depth + x * depth + depth,
		(y + 1) * width * depth + x * depth - depth,
		(y + 1) * width * depth + x * depth,
		(y + 1) * width * depth + x * depth + depth,
	    };
	    int chan, max = 0;
	    for(chan = 0; chan < depth; chan++) {
		    int new_val = image[offsets[1] + chan] * 6 -
			    	  image2[offsets[0] + chan] -
				  image2[offsets[2] + chan] -
				  image2[offsets[3] + chan] -
				  image2[offsets[4] + chan] -
				  image2[offsets[5] + chan];
		    if(new_val < 0) {
			    new_val = 0;
		    }
		    max = MAX(max, new_val);
	    }
	    for(chan = 0; chan < depth; chan++) {
		    image[offsets[1] + chan] = max;
	    }
    }
    // FIXME: the last pixel

    // inner pixels with 8 neighbours
    for(y = 1; y < height - 1; y++) {
	// FIXME: the first column
	for(x = 1; x < width - 1; x++) {
		int offsets[9] = {
			(y - 1) * width * depth + x * depth - depth,
			(y - 1) * width * depth + x * depth,
			(y - 1) * width * depth + x * depth + depth,
			 y      * width * depth + x * depth - depth,
			 y      * width * depth + x * depth,
			 y      * width * depth + x * depth + depth,
			(y + 1) * width * depth + x * depth - depth,
			(y + 1) * width * depth + x * depth,
			(y + 1) * width * depth + x * depth + depth,
		};
		int chan, max = 0;
		for(chan = 0; chan < depth; chan++) {
			int new_val = image[offsets[4] + chan] * 8 -
				   image2[offsets[0] + chan] -
				   image2[offsets[1] + chan] -
				   image2[offsets[2] + chan] -
				   image2[offsets[3] + chan] -
				   image2[offsets[5] + chan] -
				   image2[offsets[6] + chan] -
				   image2[offsets[7] + chan] -
				   image2[offsets[8] + chan];
			
			if(new_val < 0) {
				new_val = 0;
			}
			max = MAX(max, new_val);
		    }
		for(chan = 0; chan < depth; chan++) {
			image[offsets[4] + chan] = max;
		}
	    }
	// FIXME: the last column
    }

    // FIXME: the last line

    free (image2);
}

static void
camorama_filter_laplace_class_init(CamoramaFilterLaplaceClass* self_class) {
	self_class->filter = camorama_filter_laplace_filter;
	self_class->name   = _("Laplace (4 Neighbours)");
}
