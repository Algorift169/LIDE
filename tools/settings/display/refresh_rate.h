#ifndef LIDE_REFRESH_RATE_H
#define LIDE_REFRESH_RATE_H

#include <gtk/gtk.h>

/*
 * refresh_rate.h
 * 
 * Refresh rate interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**

 * Creates the refresh rate selection widget.
 * Detects available refresh rates for the current display and provides a combo box for selection.
 * When a refresh rate is selected, it applies it immediately using xrandr.
 *
 * @return GtkWidget containing refresh rate settings UI
 */
GtkWidget *refresh_rate_widget_new(void);

#endif /* LIDE_REFRESH_RATE_H */