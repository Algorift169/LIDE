#ifndef VOIDFOX_BOOKMARKS_H
#define VOIDFOX_BOOKMARKS_H

#include <gtk/gtk.h>
#include "voidfox.h"

/**
 * Bookmark structure.
 * Represents a saved browser bookmark with title and URL.
 */
typedef struct {
    char *title;  /* Display title for the bookmark */
    char *url;    /* Full URL of the bookmarked page */
} Bookmark;

/* Function prototypes */

/**
 * Creates the bookmarks menu.
 * Includes "Add Bookmark", "Manage Bookmarks", and a list of saved bookmarks.
 *
 * @param browser BrowserWindow instance for callbacks.
 * @return GtkWidget containing the complete bookmarks menu.
 */
GtkWidget* create_bookmarks_menu(BrowserWindow *browser);

/**
 * Adds a bookmark to the internal list.
 *
 * @param browser BrowserWindow instance.
 * @param url     URL to bookmark.
 * @param title   Display title for the bookmark.
 *
 * @sideeffect Appends to global bookmarks list if URL not already present.
 * @note Does not automatically save to disk; caller should call save_bookmarks().
 */
void add_bookmark(BrowserWindow *browser, const char *url, const char *title);

/**
 * Displays the bookmarks menu anchored to a button.
 *
 * @param menu   The menu to display.
 * @param button The button to anchor the menu to.
 */
void show_bookmarks_menu(GtkWidget *menu, GtkWidget *button);

/**
 * Saves all bookmarks to disk.
 * Writes to BOOKMARKS_FILE in format: title|url per line.
 *
 * @sideeffect Creates/overwrites the bookmarks file.
 */
void save_bookmarks(void);

/**
 * Loads bookmarks from disk.
 * Reads BOOKMARKS_FILE and populates the internal bookmarks list.
 *
 * @sideeffect Appends loaded bookmarks to global bookmarks list.
 * @note Call during application initialization to restore saved bookmarks.
 */
void load_bookmarks(void);

/**
 * Displays the bookmarks management tab.
 * Shows all saved bookmarks with options to open them.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Creates a new tab in the notebook with bookmarks list.
 */
void show_bookmarks_tab(BrowserWindow *browser);

#endif /* VOIDFOX_BOOKMARKS_H */