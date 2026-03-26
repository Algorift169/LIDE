#ifndef WIFI_LIST_H
#define WIFI_LIST_H

#include <gtk/gtk.h>

/**
 * Builds the WiFi network list UI.
 * Scans for available networks and creates clickable buttons for each network.
 *
 * @return GtkWidget containing the complete network list container.
 *         The returned widget is a GtkBox with vertically stacked network buttons.
 *
 * @sideeffect Executes nmcli to scan for WiFi networks.
 * @note This function does not connect signal handlers to the buttons.
 *       Callers must connect "clicked" signals separately if needed.
 */
GtkWidget* build_wifi_network_list(void);

/**
 * Displays a modal dialog showing available WiFi networks.
 * Provides controls for enabling/disabling WiFi, rescanning, and connecting to networks.
 *
 * @param parent_window Parent window to attach the dialog to.
 *                      Must be a valid GtkWindow or castable to GtkWindow.
 *
 * @sideeffect Creates and displays modal dialog. Blocks until dialog is closed.
 * @sideeffect Executes nmcli commands for WiFi scanning and connections.
 * @sideeffect Handles network connection logic internally via callbacks.
 */
void show_wifi_list(GtkWidget *parent_window);

#endif /* WIFI_LIST_H */