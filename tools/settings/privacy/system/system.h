#ifndef LIDE_SYSTEM_PRIVACY_H
#define LIDE_SYSTEM_PRIVACY_H

#include <gtk/gtk.h>

/**
 * Creates the widget that contains all system‑related privacy settings.
 * Uses a notebook inside a frame.
 *
 * @return GtkWidget containing cache, file history, location, screen lock.
 */
GtkWidget *system_settings_widget_new(void);

#endif /* LIDE_SYSTEM_PRIVACY_H */
