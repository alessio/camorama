#ifndef CAMORAMA_CALLBACKS_H
#define CAMORAMA_CALLBACKS_H

#include "v4l.h"
#include "fileio.h"
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

void on_change_size_activate(GtkWidget * widget, cam_t *cam);
void on_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
void gconf_notify_func(GConfClient *client, guint cnxn_id,
                       GConfEntry *entry, char **);
void gconf_notify_func_bool(GConfClient *client, guint cnxn_id,
                            GConfEntry *entry, gboolean *val);
void gconf_notify_func_int(GConfClient *client, guint cnxn_id,
                           GConfEntry *entry, int *val);
int delete_event(GtkWidget *, gpointer data);
void cap_func(GtkWidget *, cam_t *);
void rcap_func(GtkWidget *, cam_t *);
void acap_func(GtkWidget *, cam_t *);
void set_sensitive(cam_t *);
void tt_enable_func(GtkWidget *, cam_t *);
void interval_change(GtkWidget *, cam_t *);
void ts_func(GtkWidget *, cam_t *);
void customstring_func(GtkWidget *, cam_t *);
void drawdate_func(GtkWidget *, cam_t *);
void append_func(GtkWidget *, cam_t *);
void rappend_func(GtkWidget *, cam_t *);
void jpg_func(GtkWidget *, cam_t *);
void png_func(GtkWidget *, cam_t *);
void ppm_func(GtkWidget *, cam_t *);
void rts_func(GtkWidget *, cam_t *);
void rjpg_func(GtkWidget *, cam_t *);
void rpng_func(GtkWidget *, cam_t *);
void draw_callback(GtkWidget *, cairo_t *, cam_t *cam);

gint(*pt2Function) (cam_t *);
void rppm_func(GtkWidget *, cam_t *);
void on_preferences1_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_about_activate(GtkMenuItem *menuitem, cam_t *cam);
void on_show_adjustments_activate(GtkToggleButton *button, cam_t *);
void on_show_effects_activate(GtkMenuItem *menuitem, cam_t *);
void prefs_func(GtkWidget *, cam_t *);
void capture_func2(GtkWidget *, cam_t *);
void capture_func(GtkWidget *, cam_t *);
gint timeout_capture_func(cam_t *);
gint fps(GtkWidget *);
gint timeout_func(cam_t *);
gint read_timeout_func(cam_t *);
void edge_func1(GtkToggleButton *, gpointer);
void sobel_func(GtkToggleButton *, gpointer);
void fix_colour_func(GtkToggleButton *, char *);
void threshold_func1(GtkToggleButton *, gpointer);
void threshold_ch_func(GtkToggleButton *, gpointer);
void edge_func3(GtkToggleButton *, gpointer);
void mirror_func(GtkToggleButton *, gpointer);
void reichardt_func(GtkToggleButton *, gpointer);
void colour_func(GtkToggleButton *, gpointer);
void smooth_func(GtkToggleButton *, gpointer);
void negative_func(GtkToggleButton *, gpointer);
void on_scale1_drag_data_received(GtkHScale *, cam_t *);
void on_status_show(GtkWidget *, cam_t *);
void contrast_change(GtkHScale *, cam_t *);
void brightness_change(GtkHScale *, cam_t *);
void colour_change(GtkHScale *, cam_t *);
void hue_change(GtkHScale *, cam_t *);
void wb_change(GtkHScale *, cam_t *);
gboolean on_drawingarea_expose_event(GtkWidget *, GdkEventExpose *,
                                     cam_t *);
void update_tooltip(cam_t *cam);

G_END_DECLS

#endif                          /* !CAMORAMA_CALLBACKS_H */
