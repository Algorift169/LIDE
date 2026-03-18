#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

void download_init(void);
double get_download_speed(void);
const char* get_download_speed_formatted(void);
gboolean update_download_stats(gpointer user_data);
void download_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* DOWNLOAD_H */