#ifndef UPLOAD_H
#define UPLOAD_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

void upload_init(void);
double get_upload_speed(void);
const char* get_upload_speed_formatted(void);
gboolean update_upload_stats(gpointer user_data);
void upload_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* UPLOAD_H */