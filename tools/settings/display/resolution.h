#ifndef RESOLUTION_H
#define RESOLUTION_H

#include <gtk/gtk.h>



/*
 * resolution.h
 * 
 * Screen resolution interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**
 * Creates the resolution selection widget.
 * Provides a combo box with common display resolutions.
 *
 * @return GtkWidget containing horizontally packed label and combo box.
 *         Default selection is 1920x1080.
 */
GtkWidget *resolution_widget_new(void);

#endif /* RESOLUTION_H */