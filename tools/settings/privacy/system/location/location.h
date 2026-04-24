#ifndef LIDE_LOCATION_PRIVACY_H
#define LIDE_LOCATION_PRIVACY_H

#include <gtk/gtk.h>

/**
 * Creates a widget to control system‑wide location access.
 *
 * @return GtkWidget.
 */
GtkWidget *location_settings_widget_new(void);

#endif /* LIDE_LOCATION_PRIVACY_H */
