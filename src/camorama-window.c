/* This file is part of camorama
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

#include "camorama-window.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib/gi18n.h>
#include "callbacks.h"
#include "camorama-filter-chain.h"
#include "camorama-globals.h"
#include "camorama-stock-items.h"
#include "filter.h"
#include "glib-helpers.h"
#include "support.h"

static GQuark menu_item_filter_type = 0;

/* Supported URI protocol schemas */
const gchar * const protos[] = { "ftp", "sftp", "smb" };


static void
add_filter_clicked(GtkMenuItem* menuitem, CamoramaFilterChain* chain) {
	GType filter_type = GPOINTER_TO_SIZE(g_object_get_qdata(G_OBJECT(menuitem), menu_item_filter_type));
	camorama_filter_chain_append(chain, filter_type);
}

struct weak_target {
	GtkTreeModel* model;
	GList       * list;
};

static void
reference_path(GtkTreePath* path, struct weak_target* target) {
	target->list = g_list_prepend(target->list, gtk_tree_row_reference_new(target->model, path));
}

static void
delete_filter(GtkTreeRowReference* ref, GtkTreeModel* model) {
	GtkTreeIter iter;
	GtkTreePath* path = gtk_tree_row_reference_get_path(ref);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

static void
delete_filter_clicked(GtkTreeSelection* sel, GtkMenuItem* menuitem) {
	GtkTreeModel* model;
	GList* paths = gtk_tree_selection_get_selected_rows(sel, &model);
	struct weak_target target = {model, NULL};
	g_list_foreach(paths, G_FUNC(reference_path), &target);
	g_list_foreach(target.list, G_FUNC(delete_filter), model);
	g_list_foreach(target.list, G_FUNC(gtk_tree_row_reference_free), NULL);
	g_list_free(target.list);
	g_list_foreach(paths, G_FUNC(gtk_tree_path_free), NULL);
	g_list_free(paths);
}

static void
menu_position_func(GtkMenu* menu, gint* x, gint* y, gboolean *push_in, gpointer user_data) {
	if(user_data) {
		GdkEventButton* ev = (GdkEventButton*)user_data;
		*x = ev->x;
		*y = ev->y;
	} else {
		// find the selected row and open the popup there
	}
}

static void
show_popup(cam* cam, GtkTreeView* treeview, GdkEventButton* ev) {
	GtkMenu* menu = GTK_MENU(gtk_menu_new());
	GtkWidget* item;
	GtkWidget* add_filters = gtk_menu_new();
	GType* filters;
	guint n_filters, i;
	GtkTreeModel* model = gtk_tree_view_get_model(treeview);
	GtkTreeSelection* sel = gtk_tree_view_get_selection(treeview);

	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

	item = gtk_menu_item_new_with_mnemonic("_Delete");
	g_signal_connect_swapped(item, "activate",
				 G_CALLBACK(delete_filter_clicked), sel);
	gtk_container_add(GTK_CONTAINER(menu), item);
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	
	if(!gtk_tree_selection_count_selected_rows(sel)) {
		gtk_widget_set_sensitive(item, FALSE);
	}

	item = gtk_menu_item_new_with_mnemonic(_("_Add Filter"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), add_filters);
	gtk_container_add(GTK_CONTAINER(menu), item);

	filters = g_type_children(CAMORAMA_TYPE_FILTER, &n_filters);
	for(i = 0; i < n_filters; i++) {
		CamoramaFilterClass* filter_class = g_type_class_ref(filters[i]);
		gchar const* filter_name = filter_class->name;

		if(!filter_name) {
			filter_name = g_type_name(filters[i]);
		}

		item = gtk_menu_item_new_with_label(filter_name);
		g_object_set_qdata(G_OBJECT(item), menu_item_filter_type, GSIZE_TO_POINTER(filters[i]));
		g_signal_connect(item, "activate",
				 G_CALLBACK(add_filter_clicked), model);
		gtk_container_add(GTK_CONTAINER(add_filters), item);
		g_type_class_unref(filter_class);
	}
	g_free(filters);
	
	gtk_widget_show_all(GTK_WIDGET(menu));
	gtk_menu_popup(menu,
		       NULL, NULL,
		       menu_position_func, ev,
		       ev ? ev->button : 0,
		       ev ? ev->time : gtk_get_current_event_time());
}

static void
treeview_popup_menu_cb(cam* cam, GtkTreeView* treeview) {
	show_popup(cam, treeview, NULL);
}

static gboolean
treeview_clicked_cb(cam* cam, GdkEventButton* ev, GtkTreeView* treeview) {
	gboolean retval = GTK_WIDGET_GET_CLASS(treeview)->button_press_event(GTK_WIDGET(treeview), ev);

	if(ev->button == 3) {
		show_popup(cam, treeview, NULL);
		retval = TRUE;
	}

	return retval;
}

static void
tray_clicked_callback (GtkStatusIcon* status, guint button, guint activate_time, cam * cam){
        // FIXME: change to switch
        if (button == 1) {
            if (GTK_WIDGET_VISIBLE
                (GTK_WIDGET(gtk_builder_get_object(cam->xml, "main_window")))) {
                cam->hidden = TRUE;
                gtk_idle_remove (cam->idle_id);
                gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object
                                 (cam->xml, "main_window")));
            } else {
                cam->idle_id =
                    gtk_idle_add ((GSourceFunc) pt2Function, (gpointer) cam);
                gtk_widget_show (GTK_WIDGET(gtk_builder_get_object
                                 (cam->xml, "main_window")));
                cam->hidden = FALSE;
            }
        } else if (button == 3) {

            //gw = MyApp->GetMainWindow ();

            //gnomemeeting_component_view (NULL, (gpointer) gw->ldap_window);

        }
}

static guint resolution_signals[8] = { 0 };


void
load_interface(cam* cam) {
    int i;
    gchar *title;
    GdkPixbuf *logo = NULL;
    GtkCellRenderer*     cell;
    GtkWidget *small_res, *new_res;
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(cam->xml,
                                                          "main_window"));

    menu_item_filter_type = g_quark_from_static_string("camorama-menu-item-filter-type");

    /* set up the tree view */
    GtkTreeView* treeview = GTK_TREE_VIEW(gtk_builder_get_object(cam->xml, "treeview_effects"));
    cell = gtk_cell_renderer_text_new();
    g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(cell), 1);
    gtk_tree_view_insert_column_with_attributes(treeview, -1,
		    				_("Effects"), cell,
						"text", CAMORAMA_FILTER_CHAIN_COL_NAME,
						NULL);
    cam->filter_chain = camorama_filter_chain_new();
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(cam->filter_chain));
    g_object_unref(cam->filter_chain);
    g_signal_connect_swapped(treeview, "button-press-event",
		    	     G_CALLBACK(treeview_clicked_cb), cam);
    g_signal_connect_swapped(treeview, "popup-menu",
		    	     G_CALLBACK(treeview_popup_menu_cb), cam);

    logo = gtk_icon_theme_load_icon(gtk_icon_theme_get_for_screen(gtk_widget_get_screen(window)), CAMORAMA_STOCK_WEBCAM, 24, 0, NULL);
    gtk_window_set_default_icon(logo);
    logo = gtk_icon_theme_load_icon(gtk_icon_theme_get_for_screen(gtk_widget_get_screen(window)), "camorama", 48, 0, NULL);
    if (cam->show_adjustments == FALSE) {
        gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object
                         (cam->xml, "adjustments_table")));

        gtk_window_resize (GTK_WINDOW(window), 320, 240);
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(cam->xml, "showadjustment_item")), cam->show_adjustments);
    if (cam->show_effects == FALSE) {
        gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(cam->xml, "scrolledwindow_effects")));
        gtk_window_resize (GTK_WINDOW(window), 320, 240);
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(cam->xml, "show_effects")), cam->show_effects);

    cam->tray_icon = gtk_status_icon_new_from_stock (CAMORAMA_STOCK_WEBCAM);
    update_tooltip (cam);
    /* add the status to the plug */
    g_object_set_data (G_OBJECT (cam->tray_icon), "available",
                       GINT_TO_POINTER (1));
    g_object_set_data (G_OBJECT (cam->tray_icon), "embedded",
                       GINT_TO_POINTER (0));

    g_signal_connect (cam->tray_icon, "popup-menu",
                      G_CALLBACK (tray_clicked_callback), cam);

    /* connect the signals in the interface 
     * glade_xml_signal_autoconnect(xml);
     * this won't work, can't pass data to callbacks.  have to do it individually :(*/

    title = g_strdup_printf ("Camorama - %s - %dx%d", cam->name,
                             cam->width, cam->height);
    gtk_window_set_title (GTK_WINDOW(window), title);
    g_free (title);

    gtk_window_set_icon (GTK_WINDOW(window), logo);
    gtk_window_set_icon (GTK_WINDOW
                         (GTK_WIDGET(gtk_builder_get_object(cam->xml, "prefswindow"))),
                         logo);

    g_signal_connect (gtk_builder_get_object(cam->xml, "show_effects"), "activate",
                      G_CALLBACK (on_show_effects_activate), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  GTK_WIDGET(gtk_builder_get_object (cam->xml, "togglebutton1")),
                                  cam->show_adjustments);
    g_signal_connect (gtk_builder_get_object(cam->xml, "togglebutton1"), "toggled",
                      G_CALLBACK (on_show_adjustments_activate), cam);


    /*
     * Dynamically generate the resolutions based on what the camera
     * actually supports. Provide a fallback method, if the camera driver
     * is too old and doesn't support formats enumeration.
     */

    small_res = GTK_WIDGET(gtk_builder_get_object(cam->xml, "small"));

    /* Get all supported resolutions by cam->pixformat */
    get_supported_resolutions(cam);

    if (cam->n_res > 0) {
        for (i = 0; i < cam->n_res; i++) {
            char name[32];

            sprintf(name, "%dx%d", cam->res[i].x, cam->res[i].y);

            new_res = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(small_res), name);
            gtk_menu_append(GTK_MENU(GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"))), new_res);
            gtk_widget_show (new_res);
            g_signal_connect(new_res, "activate",
                             G_CALLBACK (on_change_size_activate), cam);
            gtk_widget_set_name(new_res, name);

            if (cam->width == cam->res[i].x && cam->height == cam->res[i].y)
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_res), TRUE);
            else
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_res), FALSE);
        }

        /* We won't actually use the small res */
        gtk_widget_hide (small_res);
    } else {
        g_signal_connect (gtk_builder_get_object(cam->xml, "small"), "activate",
                                       G_CALLBACK (on_change_size_activate), cam);

        new_res = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(small_res), "Medium");
        gtk_menu_append(GTK_MENU(GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"))), new_res);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_res), FALSE);
        gtk_widget_show (new_res);
        g_signal_connect(new_res, "activate", G_CALLBACK (on_change_size_activate), cam);
        gtk_widget_set_name(new_res, "medium");

        new_res = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(small_res), "Large");
        gtk_menu_append(GTK_MENU(GTK_WIDGET(gtk_builder_get_object(cam->xml, "menuitem4_menu"))), new_res);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_res), FALSE);
        gtk_widget_show (new_res);
        g_signal_connect(new_res, "activate", G_CALLBACK (on_change_size_activate), cam);
        gtk_widget_set_name(new_res, "large");
    }

    //g_signal_connect(cam->xml, "capture_func", G_CALLBACK(on_change_size_activate), cam);
    g_signal_connect (gtk_builder_get_object(cam->xml, "button1"), "clicked",
                                   G_CALLBACK (capture_func), cam);
    g_signal_connect (window, "delete-event",
                                   G_CALLBACK (delete_event), NULL);

    /* sliders */
    if (cam->contrast < 0) {
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "contrast_icon")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "contrast_label")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "contrast_slider")));
    } else {
        g_signal_connect (gtk_builder_get_object(cam->xml, "contrast_slider"),
                          "value-changed", G_CALLBACK (contrast_change), cam);
        gtk_range_set_value ((GtkRange *)
                             GTK_WIDGET(gtk_builder_get_object(cam->xml, "contrast_slider")),
                             (int) (cam->contrast / 256));
    }
    if (cam->brightness < 0) {
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "brightness_icon")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "brightness_label")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "brightness_slider")));
    } else {
        g_signal_connect (gtk_builder_get_object(cam->xml, "brightness_slider"),
                          "value-changed", G_CALLBACK (brightness_change), cam);
        gtk_range_set_value ((GtkRange *)
                            GTK_WIDGET(gtk_builder_get_object(cam->xml, "brightness_slider")),
                            (int) (cam->brightness / 256));
    }
    if (cam->colour < 1) {
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "color_icon")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "color_label")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "color_slider")));
    } else {
        g_signal_connect (gtk_builder_get_object(cam->xml, "color_slider"),
                          "value-changed", G_CALLBACK (colour_change), cam);
        gtk_range_set_value ((GtkRange *)
                            GTK_WIDGET(gtk_builder_get_object(cam->xml, "color_slider")),
                            (int) (cam->colour / 256));
    }
    if (cam->hue < 0) {
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "hue_icon")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "hue_label")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "hue_slider")));
    } else {
        g_signal_connect (gtk_builder_get_object(cam->xml, "hue_slider"),
                          "value-changed", G_CALLBACK (hue_change), cam);
        gtk_range_set_value ((GtkRange *)
                             GTK_WIDGET(gtk_builder_get_object(cam->xml, "hue_slider")),
                             (int) (cam->hue / 256));
    }
    if (cam->whiteness < 0) {
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "balance_icon")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "balance_label")));
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(cam->xml, "balance_slider")));
    } else {
        g_signal_connect (gtk_builder_get_object(cam->xml, "balance_slider"),
                          "value-changed", G_CALLBACK (wb_change), cam);
        gtk_range_set_value ((GtkRange *)
                            GTK_WIDGET(gtk_builder_get_object(cam->xml, "balance_slider")),
                            (int) (cam->whiteness / 256));
    }

    /* buttons */
    g_signal_connect (gtk_builder_get_object(cam->xml, "quit"), "activate",
                      G_CALLBACK (on_quit_activate), cam);
    g_signal_connect (gtk_builder_get_object(cam->xml, "imagemenuitem3"),
                      "activate", G_CALLBACK (on_preferences1_activate), cam);
    g_signal_connect (gtk_builder_get_object(cam->xml, "imagemenuitem4"),
                      "activate", G_CALLBACK (on_about_activate), cam);

    /* prefs */
    g_signal_connect (gtk_builder_get_object(cam->xml, "okbutton1"), "clicked",
                      G_CALLBACK (prefs_func), cam);

    /* general */
    g_signal_connect (gtk_builder_get_object(cam->xml, "captured_cb"), "toggled",
                      G_CALLBACK (cap_func), cam);

    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                        "captured_cb"),
                                  cam->cap);

    g_signal_connect (gtk_builder_get_object(cam->xml, "rcapture"), "toggled",
                      G_CALLBACK (rcap_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                        "rcapture"),
                                  cam->rcap);

    g_signal_connect (gtk_builder_get_object(cam->xml, "acapture"), "toggled",
                      G_CALLBACK (acap_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                        "acapture"),
                                  cam->acap);

    g_signal_connect (gtk_builder_get_object(cam->xml, "interval_entry"),
                      "value-changed", G_CALLBACK (interval_change), cam);

    gtk_spin_button_set_value ((GtkSpinButton *)
                               gtk_builder_get_object (cam->xml,
                                                     "interval_entry"),
                               (cam->timeout_interval / 60000));

    /* local */
    dentry = GTK_WIDGET(gtk_builder_get_object(cam->xml, "dentry"));
    entry2 = GTK_WIDGET(gtk_builder_get_object(cam->xml, "entry2"));
    gtk_file_chooser_set_current_folder((GtkFileChooser *) dentry, cam->pixdir);

    gtk_entry_set_text (GTK_ENTRY (entry2), cam->capturefile);

    g_signal_connect (gtk_builder_get_object(cam->xml, "appendbutton"),
                      "toggled", G_CALLBACK (append_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                        "appendbutton"),
                                  cam->timefn);

    g_signal_connect (gtk_builder_get_object(cam->xml, "jpgb"),
                      "toggled", G_CALLBACK (jpg_func), cam);
    if (cam->savetype == JPEG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      gtk_builder_get_object (cam->xml,
                                                            "jpgb"), TRUE);
    }
    g_signal_connect (gtk_builder_get_object(cam->xml, "pngb"),
                      "toggled", G_CALLBACK (png_func), cam);
    if (cam->savetype == PNG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      gtk_builder_get_object (cam->xml,
                                                            "pngb"), TRUE);
    }

    g_signal_connect (gtk_builder_get_object(cam->xml, "tsbutton"),
                      "toggled", G_CALLBACK (ts_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                        "tsbutton"),
                                  cam->timestamp);

    /* remote */
    host_entry = GTK_WIDGET(gtk_builder_get_object(cam->xml, "host_entry"));
    protocol = GTK_WIDGET(gtk_builder_get_object(cam->xml, "remote_protocol"));
    rdir_entry = GTK_WIDGET(gtk_builder_get_object(cam->xml, "rdir_entry"));
    filename_entry = GTK_WIDGET(gtk_builder_get_object(cam->xml, "filename_entry"));

    gtk_entry_set_text (GTK_ENTRY (host_entry), cam->host);
    gtk_entry_set_text (GTK_ENTRY (rdir_entry), cam->rdir);
    gtk_entry_set_text (GTK_ENTRY (filename_entry), cam->rcapturefile);

    if (!cam->proto)
        cam->proto = g_strdup(protos[0]);

    for (i = 0; i < G_N_ELEMENTS (protos); i++) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(protocol), protos[i]);
        if (!strcmp(cam->proto, protos[i]))
            gtk_combo_box_set_active(GTK_COMBO_BOX(protocol), i);
    }

    if (cam->host && cam->proto && cam->rdir) {
        cam->uri = volume_uri(cam->host, cam->proto, cam->rdir);
        mount_volume(cam);
    } else {
        cam->uri = NULL;
    }

    g_signal_connect (gtk_builder_get_object(cam->xml, "timecb"),
                      "toggled", G_CALLBACK (rappend_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                          "timecb"),
                                  cam->rtimefn);

    g_signal_connect (gtk_builder_get_object(cam->xml, "fjpgb"),
                      "toggled", G_CALLBACK (rjpg_func), cam);
    if (cam->rsavetype == JPEG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      gtk_builder_get_object (cam->xml,
                                                            "fjpgb"), TRUE);
    }
    g_signal_connect (gtk_builder_get_object(cam->xml, "fpngb"),
                      "toggled", G_CALLBACK (rpng_func), cam);
    if (cam->rsavetype == PNG) {
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      gtk_builder_get_object (cam->xml,
                                                            "fpngb"), TRUE);
    }

    g_signal_connect (gtk_builder_get_object(cam->xml, "tsbutton2"),
                      "toggled", G_CALLBACK (rts_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  gtk_builder_get_object (cam->xml,
                                                        "tsbutton2"),
                                  cam->rtimestamp);

    /* timestamp */
    g_signal_connect (gtk_builder_get_object(cam->xml, "cscb"),
                      "toggled", G_CALLBACK (customstring_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  GTK_WIDGET(gtk_builder_get_object(cam->xml, "cscb")),
                                  cam->usestring);

    string_entry = GTK_WIDGET(gtk_builder_get_object(cam->xml, "string_entry"));
    gtk_entry_set_text (GTK_ENTRY (string_entry), cam->ts_string);

    g_signal_connect (gtk_builder_get_object(cam->xml, "tscb"),
                      "toggled", G_CALLBACK (drawdate_func), cam);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                                  GTK_WIDGET(gtk_builder_get_object(cam->xml, "tscb")),
                                  cam->usedate);

    cam->status = GTK_WIDGET(gtk_builder_get_object(cam->xml, "status"));
    set_sensitive (cam);
    gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object
                              (cam->xml, "string_entry")), cam->usestring);

    gtk_widget_set_size_request (GTK_WIDGET(gtk_builder_get_object(cam->xml, "da")),
                                 cam->width, cam->height);

    prefswindow = GTK_WIDGET(gtk_builder_get_object(cam->xml, "prefswindow"));
}


