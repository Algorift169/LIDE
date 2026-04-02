#ifndef LIDE_SHOW_OUTPUT_DEVICE_H
#define LIDE_SHOW_OUTPUT_DEVICE_H

#include <gtk/gtk.h>


/*
 * show_output_device.h
 * 
 * Output device selection interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

GtkWidget *output_device_widget_new(void);

#endif