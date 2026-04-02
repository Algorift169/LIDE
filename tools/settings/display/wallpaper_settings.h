#ifndef WALLPAPER_SETTINGS_H
#define WALLPAPER_SETTINGS_H

#include <gtk/gtk.h>


/*
 * wallpaper_settings.h
 * 
 * Wallpaper settings interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**

 * Creates the wallpaper selection widget.
 * Scans ./images/wallpapers/ directory for wal*.png files and provides
 * a combo box to select and apply wallpapers.
 *
 * @return GtkWidget containing horizontally packed label and combo box.
 *         Displays thumbnail previews and applies wallpaper on selection.
 */
GtkWidget *wallpaper_settings_widget_new(void);

#endif /* WALLPAPER_SETTINGS_H */
