#ifndef VOIDFOX_DOWNLOAD_STATS_H
#define VOIDFOX_DOWNLOAD_STATS_H

#include <gtk/gtk.h>
#include "voidfox.h"

/**
 * Displays the download manager window.
 * Shows all active and completed downloads with real-time progress bars.
 *
 * @param browser BrowserWindow instance to associate with the window.
 *                Used as the transient parent for the download manager window.
 *
 * @sideeffect Creates and displays a modal download manager window.
 * @sideeffect Starts a periodic refresh timer (500ms) to update progress.
 * @sideeffect If the window already exists, brings it to the front instead of creating a new one.
 *
 * @note The window is non-modal but stays above the parent browser window.
 * @note Caller does not need to manage the window lifecycle; it self-destructs on close.
 */
void show_download_stats_window(BrowserWindow *browser);

#endif /* VOIDFOX_DOWNLOAD_STATS_H */