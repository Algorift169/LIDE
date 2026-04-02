#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <glib.h>


/*
 * download.h
 * 
 * Download tracking interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the download speed monitor.
 * Detects the active network interface and captures baseline byte counts.
 * Must be called before any other download functions.
 *
 * @sideeffect Opens /proc/net/dev to read interface statistics.
 * @sideeffect Prints initialization message to stdout.
 */
void download_init(void);

/**
 * Calculates current download speed based on delta between successive reads.
 * Each call updates the internal byte count reference for the next calculation.
 *
 * @return Download speed in kilobytes per second (KB/s).
 *         Returns 0.0 on initialization or if delta is negative.
 *
 * @sideeffect Updates internal statistics state.
 */
double get_download_speed(void);

/**
 * Returns current download speed as a formatted string with appropriate units.
 *
 * @return Pointer to static string buffer containing formatted speed
 *         (e.g., "1.5 MB/s", "256.0 KB/s"). Valid until next call to this function
 *         or format_speed() in the implementation.
 *
 * @sideeffect Updates internal statistics state via get_download_speed().
 */
const char* get_download_speed_formatted(void);

/**
 * GLib timeout callback that refreshes download statistics without calculating speed.
 * Designed to be used with g_timeout_add() for background monitoring.
 *
 * @param user_data Unused, passed through from GLib timeout registration.
 * @return G_SOURCE_CONTINUE to keep timeout active.
 *
 * @sideeffect Updates internal byte count statistics.
 */
gboolean update_download_stats(gpointer user_data);

/**
 * Cleans up download monitor resources.
 * Removes any active GLib timeout sources.
 *
 * @sideeffect Stops periodic background updates if they were scheduled.
 */
void download_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* DOWNLOAD_H */