#include "v4l.h"

int add_rgb_text (guchar *, int, int, char *, char *, gboolean, gboolean);
void remote_save (cam *);
void save_thread (cam *);
int local_save (cam *);
gchar *volume_uri(gchar *host, gchar *proto, gchar *rdir);
void umount_volume(cam *);
void mount_volume (cam *);
