#ifndef LIDE_PANEL_H
#define LIDE_PANEL_H

#include <gtk/gtk.h>
#include <time.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Timer callback that updates a label with the current system time.
 * Formats the time with date and full timestamp: "Mon 01 Jan 12:34:56"
 *
 * @param label Pointer to a GtkLabel widget to update
 * @return      G_SOURCE_CONTINUE to keep the timer running
 *
 * @sideeffect Updates the text of the provided label widget.
 *             Called repeatedly from a GLib timeout source.
 */
gboolean update_clock(gpointer label);

/**
 * Reads current battery percentage from sysfs.
 * Attempts BAT0 first, falls back to BAT1.
 *
 * @return Battery percentage (0-100), or -1 if no battery found.
 *
 * @sideeffect Opens sysfs battery capacity file for reading.
 */
int get_battery_percent(void);

/**
 * Checks whether battery is currently charging.
 * Reads status from sysfs and checks for "Charging" or "Full" state.
 *
 * @return TRUE if charging or full, FALSE otherwise.
 *
 * @sideeffect Opens sysfs battery status file for reading.
 */
gboolean check_battery_charging(void);

/**
 * Checks internet connectivity by pinging Google DNS (8.8.8.8).
 * Updates global is_internet_connected flag.
 *
 * @sideeffect Executes ping command and updates global state.
 */
void check_internet_connectivity(void);

/**
 * Checks current WiFi connection status using nmcli.
 * Updates global is_wifi_connected flag and wifi_ssid string.
 *
 * @sideeffect Executes nmcli command and updates global state.
 */
void check_wifi_status(void);

/**
 * Creates and displays the network settings window/tab.
 * Provides controls for WiFi connection, network configuration, and brightness.
 *
 * @param button The button that triggered the launch.
 * @param data   User data passed during signal connection (unused).
 *
 * @sideeffect Creates a new top-level window if not already open.
 * @sideeffect Starts periodic updates for network status.
 */
void launch_network_tab(GtkButton *button, gpointer data);

#endif /* LIDE_PANEL_H */