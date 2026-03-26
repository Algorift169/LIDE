#ifndef REFRESH_RATE_H
#define REFRESH_RATE_H

#include <gtk/gtk.h>

/**
 * Creates the refresh rate selection widget.
 * Provides a combo box with common display refresh rates.
 *
 * @return GtkWidget containing horizontally packed label and combo box.
 *         Default selection is 60 Hz.
 */
GtkWidget *refresh_rate_widget_new(void);

#endif /* REFRESH_RATE_H */