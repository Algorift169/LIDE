#ifndef LIDE_SOUND_H
#define LIDE_SOUND_H

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
/**
 * Creates the sound settings tab.
 * Uses amixer commands to control system volume.
 *
 * @return GtkWidget containing the sound settings UI.
 */
GtkWidget *sound_tab_new(void);

#endif /* LIDE_SOUND_H */