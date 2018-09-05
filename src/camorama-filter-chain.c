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

#include "v4l.h"
#include "camorama-filter-chain.h"

#include "filter.h"

struct CamoramaImage {
    guchar *buf;
    gint width;
    gint height;
    gint depth;
};

CamoramaFilterChain *camorama_filter_chain_new(void)
{
    return g_object_new(CAMORAMA_TYPE_FILTER_CHAIN, NULL);
}

void camorama_filter_chain_append(CamoramaFilterChain *self,
                                  GType filter_type)
{
    GtkTreeIter iter;
    GtkListStore *store;
    CamoramaFilter *filter;

    g_return_if_fail(g_type_is_a(filter_type, CAMORAMA_TYPE_FILTER));

    store = GTK_LIST_STORE(self);
    gtk_list_store_append(store, &iter);
    filter = g_object_new(filter_type, NULL);
    gtk_list_store_set(store, &iter,
                       CAMORAMA_FILTER_CHAIN_COL_FILTER, filter,
                       CAMORAMA_FILTER_CHAIN_COL_NAME,
                       camorama_filter_get_name(filter), -1);
    g_object_unref(filter);
}

static gboolean camorama_filter_chain_apply_filter(GtkTreeModel *model,
                                                   GtkTreePath *path,
                                                   GtkTreeIter *iter,
                                                   gpointer user_data)
{
    struct CamoramaImage *img = user_data;
    CamoramaFilter *filter = NULL;

    gtk_tree_model_get(model, iter,
                       CAMORAMA_FILTER_CHAIN_COL_FILTER, &filter, -1);
    camorama_filter_apply(filter, img->buf, img->width, img->height,
                          img->depth);

    return FALSE;
}

void camorama_filter_chain_apply(CamoramaFilterChain *self,
                                 guchar *image, gint width, gint height,
                                 gint depth)
{
    struct CamoramaImage img = { image, width, height, depth };

    gtk_tree_model_foreach(GTK_TREE_MODEL(self),
                           camorama_filter_chain_apply_filter, &img);
}

/* GType stuff */
G_DEFINE_TYPE(CamoramaFilterChain, camorama_filter_chain,
              GTK_TYPE_LIST_STORE);

static void camorama_filter_chain_init(CamoramaFilterChain *self)
{
    GType column_types[CAMORAMA_FILTER_CHAIN_N_COLUMNS];

    column_types[CAMORAMA_FILTER_CHAIN_COL_NAME] = G_TYPE_STRING;
    column_types[CAMORAMA_FILTER_CHAIN_COL_FILTER] = G_TYPE_OBJECT;
    gtk_list_store_set_column_types(GTK_LIST_STORE(self),
                                    CAMORAMA_FILTER_CHAIN_N_COLUMNS,
                                    column_types);
}

static void camorama_filter_chain_class_init(CamoramaFilterChainClass *
                                             self)
{
}
