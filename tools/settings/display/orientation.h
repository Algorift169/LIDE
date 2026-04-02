#ifndef ORIENTATION_H
#define ORIENTATION_H

#include <gtk/gtk.h>



/*
 * orientation.h
 * 
 * Screen orientation interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**
 * Creates the orientation selection widget.
 * Provides radio buttons for selecting screen orientation modes:
 * Landscape (normal), Portrait (90°), Landscape flipped (180°), Portrait flipped (270°).
 *
 * @return GtkWidget containing horizontally packed radio buttons.
 *         Default selection is Landscape.
 */
GtkWidget *orientation_widget_new(void);

#endif /* ORIENTATION_H */