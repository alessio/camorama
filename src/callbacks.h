#ifndef CAMORAMA_CALLBACKS_H
#define CAMORAMA_CALLBACKS_H

#include <gtk/gtk.h>
#include "fileio.h"
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

void on_change_size_activate (GtkWidget * widget, cam * cam);
void on_quit_activate (GtkMenuItem * menuitem, gpointer user_data);
void gconf_notify_func (GConfClient * client, guint cnxn_id,
                        GConfEntry * entry, char *);
void gconf_notify_func_bool (GConfClient * client, guint cnxn_id,
                             GConfEntry * entry, gboolean val);
void gconf_notify_func_int (GConfClient * client, guint cnxn_id,
                            GConfEntry * entry, int val);
int delete_event (GtkWidget *, gpointer data);
void cap_func (GtkWidget *, cam *);
void rcap_func (GtkWidget *, cam *);
void acap_func (GtkWidget *, cam *);
void set_sensitive (cam *);
void tt_enable_func (GtkWidget *, cam *);
void interval_change (GtkWidget *, cam *);
void ts_func (GtkWidget *, cam *);
void customstring_func (GtkWidget *, cam *);
void drawdate_func (GtkWidget *, cam *);
void append_func (GtkWidget *, cam *);
void rappend_func (GtkWidget *, cam *);
void jpg_func (GtkWidget *, cam *);
void png_func (GtkWidget *, cam *);
void ppm_func (GtkWidget *, cam *);
void rts_func (GtkWidget *, cam *);
void rjpg_func (GtkWidget *, cam *);
void rpng_func (GtkWidget *, cam *);
gint (*pt2Function) (cam *);
void rppm_func (GtkWidget *, cam *);
void on_preferences1_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_about_activate (GtkMenuItem * menuitem, cam *cam);
void on_show_adjustments_activate (GtkToggleButton * button, cam *);
void on_show_effects_activate (GtkMenuItem * menuitem, cam *);
void prefs_func (GtkWidget *, cam *);
void capture_func2 (GtkWidget *, cam *);
void capture_func (GtkWidget *, cam *);
gint timeout_capture_func (cam *);
gint fps (GtkWidget *);
gint timeout_func (cam *);
gint read_timeout_func (cam *);
void edge_func1 (GtkToggleButton *, gpointer);
void sobel_func (GtkToggleButton *, gpointer);
void fix_colour_func (GtkToggleButton *, char *);
void threshold_func1 (GtkToggleButton *, gpointer);
void threshold_ch_func (GtkToggleButton *, gpointer);
void edge_func3 (GtkToggleButton *, gpointer);
void mirror_func (GtkToggleButton *, gpointer);
void reichardt_func (GtkToggleButton *, gpointer);
void colour_func (GtkToggleButton *, gpointer);
void smooth_func (GtkToggleButton *, gpointer);
void negative_func (GtkToggleButton *, gpointer);
void on_scale1_drag_data_received (GtkHScale *, cam *);
void on_status_show (GtkWidget *, cam *);
void contrast_change (GtkHScale *, cam *);
void brightness_change (GtkHScale *, cam *);
void colour_change (GtkHScale *, cam *);
void hue_change (GtkHScale *, cam *);
void wb_change (GtkHScale *, cam *);
gboolean on_drawingarea_expose_event (GtkWidget *, GdkEventExpose *, cam *);
void update_tooltip (cam * cam);

G_END_DECLS

#endif /* !CAMORAMA_CALLBACKS_H */

