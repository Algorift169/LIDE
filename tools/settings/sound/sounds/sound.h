#ifndef LIDE_SOUND_SOUNDS_H
#define LIDE_SOUND_SOUNDS_H

#include <gtk/gtk.h>


/*
 * sound.h
 * 
 * Sound system interface definitions and constants
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

GtkWidget *alert_sounds_widget_new(void);
GtkWidget *volume_levels_widget_new(void);

#endif /* LIDE_SOUND_SOUNDS_H */