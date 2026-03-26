#ifndef VOIDFOX_HISTORY_H
#define VOIDFOX_HISTORY_H

#include <gtk/gtk.h>
#include "voidfox.h"
#include <time.h>

/**
 * History entry structure.
 * Represents a single visited page in browsing history.
 */
typedef struct {
    char *title;          /* Page title (display name) */
    char *url;            /* Full URL of the visited page */
    time_t timestamp;     /* Time of visit (Unix timestamp) */
} HistoryEntry;

/* Function prototypes */

/**
 * Adds a URL to the browsing history.
 * Prevents duplicate consecutive entries and maintains a maximum of 100 entries.
 *
 * @param url   URL to add to history.
 * @param title Page title (if available, uses URL as fallback).
 *
 * @sideeffect Updates history list and saves to disk.
 */
void add_to_history(const char *url, const char *title);

/**
 * Creates and displays the history tab.
 * Shows all visited pages in reverse chronological order with timestamps.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with history list.
 */
void show_history_tab(BrowserWindow *browser);

/**
 * Updates or refreshes the history tab to reflect current history.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect If history tab is open, removes and recreates it with updated data.
 */
void update_history_tab(BrowserWindow *browser);

/**
 * Saves browsing history to disk.
 * Format: timestamp|title|url per line.
 *
 * @sideeffect Writes to HISTORY_FILE in the current directory.
 */
void save_history(void);

/**
 * Loads browsing history from disk.
 * Reads HISTORY_FILE and populates the history list.
 *
 * @sideeffect Appends loaded history entries to global history list.
 * @note Call during application initialization to restore previous browsing history.
 */
void load_history(void);

/**
 * Clears all browsing history.
 *
 * @param button  The button that was clicked (unused).
 * @param browser BrowserWindow instance for UI updates.
 *
 * @sideeffect Frees all history entries and saves empty history.
 * @sideeffect Shows confirmation dialog.
 */
void clear_history(GtkButton *button, BrowserWindow *browser);

#endif /* VOIDFOX_HISTORY_H */