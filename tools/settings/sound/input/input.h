#ifndef LIDE_SOUND_INPUT_H
#define LIDE_SOUND_INPUT_H

#include <gtk/gtk.h>


/*
 * input.h
 * 
 * Audio input interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

GtkWidget *input_volume_widget_new(void);
GtkWidget *input_device_widget_new(void);

#endif