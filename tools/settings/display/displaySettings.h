#ifndef DISPLAY_SETTINGS_H
#define DISPLAY_SETTINGS_H

#include <gtk/gtk.h>



/*
 * displaySettings.h
 * 
 * Display settings interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**
 * Creates the display settings tab widget.
 * Provides a composite UI containing orientation, refresh rate, and resolution controls.
 *
 * @return GtkWidget containing the complete display settings UI.
 *         The widget is a vertically packed GtkBox with framed sections.
 *         Caller assumes ownership of the returned widget.
 */
GtkWidget *display_settings_tab_new(void);

#endif /* DISPLAY_SETTINGS_H */