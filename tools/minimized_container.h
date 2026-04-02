#ifndef MINIMIZED_CONTAINER_H
#define MINIMIZED_CONTAINER_H

#include <gtk/gtk.h>
#include <X11/Xlib.h>

/*
 * minimized_container.h
 * 
 * Minimized container interface definitions. Manages the taskbar for
 * minimized windows, allowing quick access and restoration of hidden windows.
 */

/**
 * Initialize the minimized container system.
 *
 * Opens X display, sets up event monitoring, and creates the container
 * window. Must be called after GTK initialization and before any other
 * functions in this module.
 */
void minimized_container_initialize(void);

/**
 * Add a window to the minimized list programmatically.
 *
 * @param xid X11 window identifier to add.
 *
 * This function is exposed for external callers but is typically triggered
 * automatically by UnmapNotify events. Adds the window to the UI list
 * if it does not match ignore criteria.
 */
void minimized_container_add_window(Window xid);

/**
 * Remove a window from the minimized list programmatically.
 *
 * @param xid X11 window identifier to remove.
 *
 * This function is exposed for external callers but is typically triggered
 * automatically by MapNotify or DestroyNotify events. Destroys the
 * corresponding UI row and frees associated memory.
 */
void minimized_container_remove_window(Window xid);

/**
 * Create a toggle button for the panel that shows/hides the minimized container.
 *
 * @return A new GTK button widget.
 *
 * The button's clicked signal is connected to the container visibility toggle.
 * Caller is responsible for adding the button to a panel or toolbar.
 */
GtkWidget* minimized_container_get_toggle_button(void);

#endif /* MINIMIZED_CONTAINER_H */