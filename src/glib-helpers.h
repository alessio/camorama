/* this file is part of criawips, a gnome presentation application
 *
 * AUTHORS
 *       Sven Herzberg        <herzi@gnome-de.org>
 *
 * Copyright (C) 2005 Sven Herzberg
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

#ifndef GLIB_HELPERS_H
#define GLIB_HELPERS_H

#include <glib.h>

G_BEGIN_DECLS

#define G_FUNC(func)	    ((GFunc)(func))
#define G_SOURCE_FUNC(func) ((GSourceFunc)(func))

G_END_DECLS

#endif /* GLIB_HELPERS_H */

