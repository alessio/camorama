/* This file is part of camorama
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2007  Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef CAMORAMA_DISPLAY_H
#define CAMORAMA_DISPLAY_H

#include <gtk/gtkdrawingarea.h>

#include "v4l.h"

G_BEGIN_DECLS

typedef struct _CamoDisplay        CamoDisplay;
typedef struct _CamoDisplayPrivate CamoDisplayPrivate;
typedef struct _CamoDisplayClass   CamoDisplayClass;

#define CAMO_TYPE_DISPLAY         (camo_display_get_type ())
#define CAMO_DISPLAY(i)           (G_TYPE_CHECK_INSTANCE_CAST ((i), CAMO_TYPE_DISPLAY, CamoDisplay))
#define CAMO_DISPLAY_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CAMO_TYPE_DISPLAY, CamoDisplayClass))
#define CAMO_IS_DISPLAY(i)        (G_TYPE_CHECK_INSTANCE_TYPE ((i), CAMO_TYPE_DISPLAY))
#define CAMO_IS_DISPLAY_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CAMO_TYPE_DISPLAY))
#define CAMO_DISPLAY_GET_CLASS(i) (G_TYPE_INSTANCE_GET_CLASS ((i), CAMO_TYPE_DISPLAY, CamoDisplayClass))

GType      camo_display_get_type (void);
GtkWidget* camo_display_new      (cam* camera);

struct _CamoDisplay {
	GtkDrawingArea      base_instance;
	CamoDisplayPrivate* _private;
};

struct _CamoDisplayClass {
	GtkDrawingAreaClass base_class;
};

G_END_DECLS

#endif /* !CAMORAMA_DISPLAY_H */
