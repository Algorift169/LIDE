#ifndef INTERNET_SETTINGS_H
#define INTERNET_SETTINGS_H

#include <gtk/gtk.h>

/**
 * Displays a modal dialog for configuring network interface settings.
 * Supports DHCP and static IP configuration via NetworkManager.
 *
 * @param parent_window The parent GtkWidget to attach the dialog to.
 *                      Must be a valid GtkWindow or castable to GtkWindow.
 *                      The dialog will be modal relative to this window.
 *
 * @sideeffect Blocks execution until the dialog is closed.
 * @sideeffect Executes nmcli commands to apply network changes when settings are applied.
 * @sideeffect Saves network configuration to ~/.config/blackline/network_settings.conf.
 * @sideeffect Displays success/error message dialogs based on operation result.
 * @requires   NetworkManager with nmcli available in PATH.
 */
void show_internet_settings(GtkWidget *parent_window);

#endif