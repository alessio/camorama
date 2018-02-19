/* This file  is part of camorama
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

#include "camorama-display.h"

#include "camorama-globals.h" // frames

struct _CamoDisplayPrivate {
	cam* camera;
};

enum {
	PROP_0,
	PROP_CAMERA
};

G_DEFINE_TYPE (CamoDisplay, camo_display, GTK_TYPE_DRAWING_AREA);

GtkWidget*
camo_display_new (cam* camera)
{
	return g_object_new (CAMO_TYPE_DISPLAY,
			     "camera", camera,
			     NULL);
}

static void
camo_display_init (CamoDisplay* self)
{
	self->_private = G_TYPE_INSTANCE_GET_PRIVATE (self, CAMO_TYPE_DISPLAY, CamoDisplayPrivate);
}

static void
display_get_property (GObject   * object,
		      guint       prop_id,
		      GValue    * value,
		      GParamSpec* pspec)
{
	CamoDisplay* self = CAMO_DISPLAY (object);

	switch (prop_id) {
	case PROP_CAMERA:
		g_value_set_pointer (value, self->_private->camera);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
display_set_property (GObject     * object,
		      guint         prop_id,
		      GValue const* value,
		      GParamSpec  * pspec)
{
	CamoDisplay* self = CAMO_DISPLAY (object);

	switch (prop_id) {
	case PROP_CAMERA:
		self->_private->camera = g_value_get_pointer (value);
		g_object_notify (object, "camera");
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static gboolean
display_expose_event (GtkWidget     * widget,
		      GdkEventExpose* event)
{
	CamoDisplay* self = CAMO_DISPLAY (widget);

	gdk_draw_drawable (widget->window,
			   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			   self->_private->camera->pixmap,
			   event->area.x, event->area.y, event->area.x,
			   event->area.y, event->area.width, event->area.height);

	frames++;

	return FALSE;
}

static void
camo_display_class_init (CamoDisplayClass* self_class)
{
	GObjectClass*   object_class = G_OBJECT_CLASS (self_class);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (self_class);

	object_class->get_property = display_get_property;
	object_class->set_property = display_set_property;

	g_object_class_install_property (object_class,
					 PROP_CAMERA,
#warning "FIXME: use a camera object here"
					 g_param_spec_pointer ("camera",
							       "camera",
							       "camera",
							       G_PARAM_READWRITE));

	widget_class->expose_event = display_expose_event;

	g_type_class_add_private (self_class, sizeof (CamoDisplayPrivate));
}

