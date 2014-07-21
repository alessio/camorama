#include "v4l.h"
#include <libgnomevfs/gnome-vfs.h>

int add_rgb_text (guchar *, int, int, char *, char *, gboolean, gboolean);
void remote_save (cam *);
void save_thread (cam *);
int local_save (cam *);
