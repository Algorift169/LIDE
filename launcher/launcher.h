#ifndef LIDE_LAUNCHER_H
#define LIDE_LAUNCHER_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <dirent.h>

/**
 * Application entry structure.
 * Stores the display name and executable command for each launcher item.
 */
typedef struct {
    char *name;   /* Display name shown in the list */
    char *exec;   /* Command string to execute when activated */
} AppEntry;

/**
 * Callback for search entry text changes.
 * Filters the list box rows based on display name matching the search text.
 *
 * @param entry   The GtkSearchEntry containing the filter text
 * @param listbox The GtkListBox containing application rows to filter
 *
 * @sideeffect Hides rows whose display name does not match the search text.
 *             Row visibility is toggled via gtk_widget_set_visible().
 */
void search_changed(GtkSearchEntry *entry, GtkListBox *listbox);

/**
 * Callback for list box row activation.
 * Retrieves the AppEntry from the activated row and executes its exec command.
 *
 * @param listbox The GtkListBox containing the activated row
 * @param row     The activated GtkListBoxRow
 * @param data    User data passed during signal connection (unused)
 *
 * @sideeffect Executes external command via system(). Blocks until process completes.
 * @security   Executes stored command string without sanitization.
 */
void launch_selected(GtkListBox *listbox, GtkListBoxRow *row, gpointer data);

/**
 * Sort function for GtkListBox rows.
 * Compares rows by their display name for alphabetical ordering.
 *
 * @param row1 First GtkListBoxRow to compare
 * @param row2 Second GtkListBoxRow to compare
 * @param data User data passed during sort setup (unused)
 * @return     Negative if row1 < row2, zero if equal, positive if row1 > row2
 */
gint sort_list(GtkListBoxRow *row1, GtkListBoxRow *row2, gpointer data);

#endif