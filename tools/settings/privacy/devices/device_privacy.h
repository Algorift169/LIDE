#ifndef LIDE_DEVICE_PRIVACY_H
#define LIDE_DEVICE_PRIVACY_H

#include <gtk/gtk.h>

/**
 * Creates a widget that controls camera and other device access.
 *
 * @return GtkWidget.
 */
GtkWidget *device_privacy_widget_new(void);

#endif /* LIDE_DEVICE_PRIVACY_H */
