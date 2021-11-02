/* this file is part of camorama, a gnome webcam viewer
 *
 * AUTHORS
 *        Greg Jones             <greg@fixedgear.org>
 *        Bastien Nocera             <hadess@hadess.net>
 *      Sven Herzberg        <herzi@gnome-de.org>
 *
 * Copyright (C) 2003 Greg Jones
 * Copyright (C) 2003 Bastien Nocera
 * Copyright (C) 2005,2006 Sven Herzberg
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

#ifndef CAMORAMA_FILTER_H
#define CAMORAMA_FILTER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _CamoramaFilter CamoramaFilter;
typedef struct _CamoramaFilterClass CamoramaFilterClass;

#define CAMORAMA_TYPE_FILTER         (camorama_filter_get_type())
#define CAMORAMA_FILTER(i)           (G_TYPE_CHECK_INSTANCE_CAST((i),       \
                                      CAMORAMA_TYPE_FILTER, CamoramaFilter))
#define CAMORAMA_FILTER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST((c),          \
                                      CAMORAMA_TYPE_FILTER,                 \
                                      CamoramaFilterClass))
#define CAMORAMA_IS_FILTER(i)        (G_TYPE_CHECK_INSTANCE_TYPE((i),       \
                                      CAMORAMA_TYPE_FILTER))
#define CAMORAMA_IS_FILTER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE((c),          \
                                      CAMORAMA_TYPE_FILTER))
#define CAMORAMA_FILTER_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS((i),        \
                                      CAMORAMA_TYPE_FILTER,                 \
                                      CamoramaFilterClass))

GType camorama_filter_get_type(void);
GType camorama_filter_mirror_get_type(void);
GType camorama_filter_laplace_get_type(void);
GType camorama_filter_reichardt_get_type(void);
GType camorama_filter_color_get_type(void);
GType camorama_filter_invert_get_type(void);
GType camorama_filter_threshold_get_type(void);
GType camorama_filter_threshold_channel_get_type(void);
GType camorama_filter_wacky_get_type(void);
GType camorama_filter_mono_get_type(void);
GType camorama_filter_mono_weight_get_type(void);
GType camorama_filter_sobel_get_type(void);
GType camorama_filter_smooth_get_type(void);

void camorama_filter_color_filter(void *__filter, guchar *image, int x, int y,
                                  int depth);

void camorama_filters_init(void);
gchar const *camorama_filter_get_name(CamoramaFilter * self);
void camorama_filter_apply(CamoramaFilter *self, guchar *image,
                           gint width, gint height, gint depth);

struct _CamoramaFilter {
    GObject base_instance;
};

struct _CamoramaFilterClass {
    GObjectClass base_class;

    gchar const *name;

    void (*filter)(void *self,
                    guchar *image, gint width, gint height, gint depth);
};

G_END_DECLS

#endif                          /* !CAMORAMA_FILTER_H */
