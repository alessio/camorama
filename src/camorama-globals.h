/* This file is part of ...
 *
 * AUTHORS
 *     Sven Herzberg  <herzi@gnome-de.org>
 *
 * Copyright (C) 2003  Greg Jones
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

#ifndef CAMORAMA_GLOBALS_H
#define CAMORAMA_GLOBALS_H

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

GtkWidget *main_window, *prefswindow;
int frames, frames2, seconds;
GtkWidget *dentry, *entry2, *string_entry, *format_selection;
GtkWidget *host_entry, *directory_entry, *filename_entry, *login_entry,
    *pw_entry;

G_END_DECLS

#endif /* !CAMORAMA_GLOBALS_H */
