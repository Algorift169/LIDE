#ifndef VIEW_MODE_H
#define VIEW_MODE_H

#include <gtk/gtk.h>

/**

/*
 * viewMode.h
 * 
 * View mode interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

 * View mode enumeration for tool display layout.
 *
 * LIST mode displays tools as a vertical list of buttons.
 * GRID mode displays tools in a 2-column grid with larger icons.
 */
typedef enum {
    VIEW_MODE_LIST,
    VIEW_MODE_GRID
} ViewMode;

/**
 * Tool item structure defining an application launcher.
 *
 * @label          Display name of the tool.
 * @icon           Icon character or string to display.
 * @callback       Button click callback for list view.
 * @callback_event Button press event callback for grid view.
 * @user_data      Reserved for future use (unused).
 */
typedef struct {
    char *label;
    char *icon;
    void (*callback)(GtkButton *, gpointer);
    gboolean (*callback_event)(GtkWidget *, GdkEventButton *, gpointer);
    void *user_data; // not used
} ToolItem;

// Mode management
/**
 * Save the current view mode to the configuration file.
 *
 * Persists the user's view mode preference to
 * $HOME/.config/blackline/tools_view_mode.conf.
 */
void view_mode_save(void);

/**
 * Load the view mode from the configuration file.
 *
 * Reads the saved view mode from disk. If the file does not exist
 * or contains an invalid value, defaults to VIEW_MODE_LIST.
 */
void view_mode_load(void);

/**
 * Get the current view mode.
 *
 * @return The currently active ViewMode (LIST or GRID).
 *
 * If not already loaded, triggers view_mode_load() automatically.
 */
ViewMode view_mode_get_current(void);

/**
 * Set the current view mode programmatically.
 *
 * @param mode The new view mode (must be VIEW_MODE_LIST or VIEW_MODE_GRID).
 *
 * Does not automatically save to disk; caller should invoke
 * view_mode_save() if persistence is desired.
 */
void view_mode_set_current(ViewMode mode);

// UI creation
/**
 * Create a container widget populated with tools in the specified view mode.
 *
 * @param tools     Array of ToolItem structures defining the applications.
 * @param num_tools Number of tools in the array.
 * @param mode      The view mode to use (LIST or GRID).
 * @param window    Parent window pointer passed to callbacks.
 * @return Newly created GtkWidget (GtkBox for LIST, GtkGrid for GRID)
 *         containing all tools.
 */
GtkWidget* view_mode_create_container(const ToolItem *tools, int num_tools, ViewMode mode, gpointer window);

/**
 * Toggle between LIST and GRID view modes, replacing the container.
 *
 * @param old_container The current container to destroy.
 * @param tools         Array of ToolItem structures.
 * @param num_tools     Number of tools in the array.
 * @param parent_box    Parent GtkBox where the container resides.
 * @param window        Parent window pointer passed to callbacks.
 * @return Newly created container with the toggled view mode.
 *
 * Destroys the old container, creates a new one with the opposite mode,
 * packs it into parent_box at position 2, and saves the new mode to disk.
 */
GtkWidget* view_mode_toggle(GtkWidget *old_container, const ToolItem *tools, int num_tools, GtkWidget *parent_box, gpointer window);

#endif /* VIEW_MODE_H */