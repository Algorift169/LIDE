#ifndef LIDE_OPTIONAL_TAB_H
#define LIDE_OPTIONAL_TAB_H

#include <X11/Xlib.h>

/**
 * Shows the right-click context menu at the specified screen coordinates.
 */
void show_context_menu(Display *d, int x, int y);

/**
 * Registers external callback functions for menu actions.
 */
void register_context_menu_callbacks(
    void (*new_folder_cb)(const char *path, const char *name),
    void (*new_file_cb)(const char *path, const char *name),
    void (*terminal_cb)(void),
    void (*change_bg_cb)(void),
    void (*display_settings_cb)(void),
    void (*show_files_cb)(void),
    char *(*get_desktop_path_cb)(void)
);

/**
 * Handles button presses on the context menu.
 */
int handle_menu_button(Display *d, XButtonEvent *ev);

/**
 * Cleans up context menu resources.
 */
void context_menu_cleanup(void);

/**
 * Checks if the context menu is currently active.
 */
int is_menu_active(void);

#endif /* LIDE_OPTIONAL_TAB_H */