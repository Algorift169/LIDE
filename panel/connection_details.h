#ifndef CONNECTION_DETAILS_H
#define CONNECTION_DETAILS_H

#include <gtk/gtk.h>

/**
 * Displays a modal dialog with network connection details.
 * Retrieves and shows hostname, IP address, gateway, and DNS information.
 *
 * @param parent_window The parent GtkWidget to attach the dialog to.
 *                      Must be a valid GtkWindow or castable to GtkWindow.
 *                      The dialog will be modal relative to this window.
 *
 * @sideeffect Blocks execution until the dialog is closed.
 * @sideeffect Executes shell commands via popen() to gather system network data.
 */
void show_connection_details(GtkWidget *parent_window);

#endif