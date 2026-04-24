#ifndef LIDE_SCREEN_LOCK_H
#define LIDE_SCREEN_LOCK_H

#include <gtk/gtk.h>

/**
 * Creates a widget for screen lock timer and enable/disable.
 *
 * @return GtkWidget.
 */
GtkWidget *screen_lock_widget_new(void);

#endif /* LIDE_SCREEN_LOCK_H */
