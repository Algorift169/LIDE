/*
 * wifi_connect.h
 * 
 * WiFi connection interface. Handles network credential input and
 * connection establishment via NetworkManager.
 */

#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <gtk/gtk.h>

/**
 * Displays a modal dialog for connecting to a WiFi network.
 * For open networks, connects directly without password prompt.
 * For secured networks, shows password entry dialog.
 *
 * @param parent_window Parent window to attach the dialog to.
 *                      Must be a valid GtkWindow or castable to GtkWindow.
 * @param ssid          Network SSID to connect to.
 *
 * @sideeffect Executes nmcli commands to connect to the network.
 * @sideeffect Shows success/error message dialogs.
 * @sideeffect Blocks execution until the connection dialog is closed.
 */
void show_wifi_connect_dialog(GtkWidget *parent_window, const char *ssid);

/**
 * Quick connect to WiFi network without showing password dialog.
 * Useful for reconnecting to previously connected networks or open networks.
 *
 * @param ssid Network SSID to connect to.
 *
 * @sideeffect Executes nmcli connection command in background.
 * @note No feedback is provided on connection success or failure.
 */
void wifi_quick_connect(const char *ssid);

/**
 * Removes a WiFi network from stored connections.
 * Deletes the connection profile from NetworkManager.
 *
 * @param ssid Network SSID to forget.
 *
 * @sideeffect Executes nmcli to delete the connection profile.
 * @sideeffect Subsequent connections to this network will require re-authentication.
 */
void wifi_forget_network(const char *ssid);

#endif /* WIFI_CONNECT_H */