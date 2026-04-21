/**
 * ============================================================================
 * FileChooser.c - Custom GTK3 File Selection Dialog
 * ============================================================================
 *
 * A lightweight, modern file chooser dialog for the LIDE desktop environment.
 * Features window dragging, file/folder navigation, search filtering, and
 * detailed file information display.
 *
 * DESIGN GOALS:
 * - Responsive, non-blocking dialog (spawns in separate process)
 * - Full mouse event handling (drag, click, double-click)
 * - Clean LIDE-themed UI (dark background, neon green accents)
 * - Support both file and folder selection modes
 *
 * KEY FEATURES:
 * - Window dragging by titlebar
 * - Home/Up/Refresh navigation buttons
 * - File list with columns (icon, name, size, type, modified date)
 * - Search/filter text field for quick file finding
 * - Filename input field for specifying custom names
 * - Centered window positioning
 * - Modal dialog with Create/Cancel buttons
 *
 * ============================================================================
 */

#include "FileChooser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <glib.h>

/* ============================================================================
 * CONSTANTS & TYPE DEFINITIONS
 * ============================================================================ */

/**
 * Tree view column indices for the file list display.
 * Each file entry in the list store contains these columns organized left-to-right.
 */
enum {
    COL_ICON = 0,       /**< File type icon/indicator */
    COL_NAME,           /**< Filename (primary visible column) */
    COL_SIZE,           /**< File size in human-readable format (KB, MB, etc) */
    COL_TYPE,           /**< File type: "File" or "Folder" */
    COL_MODIFIED,       /**< Last modification timestamp (YYYY-MM-DD HH:MM format) */
    COL_FULLPATH,       /**< Complete filesystem path (hidden, used for file ops) */
    NUM_COLS            /**< Total number of columns */
};

/**
 * File metadata structure for directory listing.
 * Used internally during directory scanning to gather and sort file information.
 *
 * Fields:
 *   name     - Display filename (without path)
 *   full_path - Complete filesystem path from root
 *   size      - File size in bytes
 *   is_dir    - Non-zero if this is a directory
 *   mtime     - Modification time (UNIX timestamp)
 */
typedef struct {
    char name[256];
    char full_path[1024];
    off_t size;
    int is_dir;
    time_t mtime;
} FileInfo;

/**
 * Window drag state tracking for mouse event handling.
 * Used to track whether user is currently dragging the window.
 */
typedef struct {
    int is_dragging;       /**< Non-zero if mouse drag is in progress */
    int drag_start_x;      /**< Window X coordinate when drag started */
    int drag_start_y;      /**< Window Y coordinate when drag started */
    int drag_offset_x;     /**< Mouse position offset from window origin at drag start */
    int drag_offset_y;     /**< Mouse position offset from window origin at drag start */
} DragState;

static DragState drag_state = {0, 0, 0, 0, 0};

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * format_file_size() - Convert bytes to human-readable file size string.
 *
 * Converts large byte values into familiar units: B (bytes), KB, MB, GB.
 * Example: 1536 bytes -> "1.5 KB"
 *
 * @param size  File size in bytes
 * @return      Static string buffer containing formatted size (caller must not free)
 *
 * IMPORTANT: Returns pointer to static storage. Subsequent calls overwrite the buffer.
 */
static char* format_file_size(off_t size)
{
    static char buffer[32];

    if (size < 1024) {
        snprintf(buffer, sizeof(buffer), "%lld B", (long long)size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB", (double)size / 1024);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f MB", (double)size / (1024 * 1024));
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f GB", (double)size / (1024 * 1024 * 1024));
    }

    return buffer;
}

/**
 * get_file_type_string() - Return display string for file type.
 *
 * @param is_dir  Non-zero if item is a directory
 * @return        "Folder" for directories, "File" for regular files
 */
static const char* get_file_type_string(int is_dir)
{
    return is_dir ? "Folder" : "File";
}

/**
 * format_mtime() - Convert UNIX timestamp to human-readable date/time.
 *
 * Formats modification time in YYYY-MM-DD HH:MM format.
 * Example: 1703001600 -> "2023-12-20 09:20"
 *
 * @param mtime  Modification time (UNIX timestamp)
 * @return       Static string buffer with formatted datetime
 */
static char* format_mtime(time_t mtime)
{
    static char buffer[32];
    struct tm *tm_info = localtime(&mtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm_info);
    return buffer;
}

/**
 * get_icon_name() - Return icon name for file type.
 *
 * Returns strings suitable for gtk_cell_renderer_pixbuf icon lookup.
 *
 * @param is_dir  Non-zero if item is a directory
 * @return        "folder" for directories, "document" for files
 */
static const char* get_icon_name(int is_dir)
{
    return is_dir ? "folder" : "document";
}

/* ============================================================================
 * DIRECTORY LOADING & FILE LISTING
 * ============================================================================ */

/**
 * file_chooser_load_directory() - Scan filesystem and populate file list.
 *
 * Reads the current directory from chooser->current_path, scans all non-hidden
 * files/folders, and populates the GTK tree view with file metadata.
 *
 * FLOW:
 * 1. Clear the existing list store
 * 2. Open the directory for reading
 * 3. Read each entry (skip hidden files starting with ".")
 * 4. Gather metadata: size, type, modification time
 * 5. Add each file as a row in the tree store
 * 6. Update location bar to show current path
 *
 * ERROR HANDLING: Issues warning and returns silently if directory cannot be opened.
 *
 * @param chooser  FileChooser instance with current_path set
 */
static void file_chooser_load_directory(FileChooser *chooser)
{
    if (!chooser || !chooser->file_store) return;

    gtk_list_store_clear(chooser->file_store);

    DIR *dir = opendir(chooser->current_path);
    if (!dir) {
        g_warning("Failed to open directory: %s", chooser->current_path);
        return;
    }

    /**
     * Read all files in directory.
     * Collect metadata first, then add to tree store.
     * Limit to 1024 files in a single directory (reasonable limit).
     */
    struct dirent *entry;
    int count = 0;
    FileInfo files[1024];

    while ((entry = readdir(dir)) && count < 1024) {
        /* Skip hidden files (starting with ".") but include ".." for parent navigation */
        if (entry->d_name[0] == '.') {
            if (strcmp(entry->d_name, "..") != 0) {
                continue;
            }
        }

        /* Build full path and stat the file to get metadata */
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", chooser->current_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        /* Store file metadata for tree view population */
        strncpy(files[count].name, entry->d_name, sizeof(files[count].name) - 1);
        strncpy(files[count].full_path, full_path, sizeof(files[count].full_path) - 1);
        files[count].size = st.st_size;
        files[count].is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
        files[count].mtime = st.st_mtime;
        count++;
    }
    closedir(dir);

    /**
     * Populate tree view with collected file information.
     * Each row represents one file/folder with icon, name, size, type, mtime.
     */
    for (int i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(chooser->file_store, &iter);

        /* Format size string (folders show "-" instead of size) */
        gchar *size_str = NULL;
        if (files[i].is_dir) {
            size_str = g_strdup("-");
        } else {
            size_str = g_strdup(format_file_size(files[i].size));
        }

        /* Add row to list store */
        gtk_list_store_set(chooser->file_store, &iter,
                          COL_ICON, get_icon_name(files[i].is_dir),
                          COL_NAME, files[i].name,
                          COL_SIZE, size_str,
                          COL_TYPE, get_file_type_string(files[i].is_dir),
                          COL_MODIFIED, format_mtime(files[i].mtime),
                          COL_FULLPATH, files[i].full_path,
                          -1);

        g_free(size_str);
    }

    /* Update location entry to show the current browsing path */
    gtk_entry_set_text(GTK_ENTRY(chooser->location_entry), chooser->current_path);
}

/* ============================================================================
 * NAVIGATION CALLBACKS
 * ============================================================================ */

/**
 * on_up_clicked() - Navigate to parent directory.
 *
 * Removes the last path component and reloads the directory listing.
 * Example: /home/user/Documents -> /home/user
 *
 * At filesystem root, button press is ignored (no parent to navigate to).
 *
 * @param button   GtkButton that was clicked (unused)
 * @param user_data Pointer to FileChooser instance
 *
 * SIGNAL: Connected to "clicked" signal of Up button
 */
static void on_up_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; /* Suppress unused parameter warning */
    FileChooser *chooser = (FileChooser*)user_data;

    char *last_slash = strrchr(chooser->current_path, '/');
    if (last_slash && last_slash != chooser->current_path) {
        *last_slash = '\0';
    } else if (last_slash == chooser->current_path) {
        /* At filesystem root, can't go up */
        return;
    }

    file_chooser_load_directory(chooser);
}

/**
 * on_home_clicked() - Navigate to user's home directory.
 *
 * Sets current_path to $HOME environment variable or falls back to
 * /home/username if $HOME is not set.
 *
 * @param button   GtkButton that was clicked (unused)
 * @param user_data Pointer to FileChooser instance
 *
 * SIGNAL: Connected to "clicked" signal of Home button
 */
static void on_home_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; /* Suppress unused parameter warning */
    FileChooser *chooser = (FileChooser*)user_data;

    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }

    if (home) {
        strncpy(chooser->current_path, home, sizeof(chooser->current_path) - 1);
        file_chooser_load_directory(chooser);
    }
}

/* ============================================================================
 * FILE SELECTION CALLBACKS
 * ============================================================================ */

/**
 * on_row_activated() - Handle double-click on file/folder item.
 *
 * Double-click behavior depends on file type:
 *   - FOLDER: Navigate into the folder (change current_path and reload listing)
 *   - FILE: Select the file and close dialog (only in CHOOSER_FILE mode)
 *
 * Extracts file metadata from TreeView row and performs appropriate action.
 *
 * @param tree     GtkTreeView widget (the file list)
 * @param path     Path to the activated row in the tree model
 * @param column   Column that was activated (unused)
 * @param user_data Pointer to FileChooser instance
 *
 * SIGNAL: Connected to "row-activated" signal (double-click/Enter on row)
 */
static void on_row_activated(GtkTreeView *tree, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer user_data)
{
    (void)column; /* Suppress unused parameter warning */
    FileChooser *chooser = (FileChooser*)user_data;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tree);

    if (!gtk_tree_model_get_iter(model, &iter, path)) return;

    /* Extract path and type from tree row */
    char *fullpath = NULL;
    gchar *type_str = NULL;

    gtk_tree_model_get(model, &iter,
                      COL_FULLPATH, &fullpath,
                      COL_TYPE, &type_str,
                      -1);

    int is_dir = (strcmp(type_str, "Folder") == 0);

    if (is_dir) {
        /**
         * FOLDER SELECTED: Navigate into directory
         * Update current_path and reload file listing
         */
        strncpy(chooser->current_path, fullpath, sizeof(chooser->current_path) - 1);
        file_chooser_load_directory(chooser);
    } else if (chooser->mode == CHOOSER_FILE) {
        /**
         * FILE SELECTED: Record selection and close dialog
         * Only applicable when in CHOOSER_FILE mode
         */
        strncpy(chooser->selected_path, fullpath, sizeof(chooser->selected_path) - 1);
        chooser->completed = 1;
        gtk_widget_hide(chooser->dialog);
    }

    g_free(type_str);
}

/**
 * on_search_changed() - Handle search text input changes.
 *
 * Called whenever user types in the search entry field.
 * For now, performs simple directory reload (basic implementation).
 *
 * TODO: Implement proper GtkTreeModelFilter for live filtering without
 *       reloading directory contents. This would provide better UX for
 *       large directories.
 *
 * @param search   GtkSearchEntry widget
 * @param user_data Pointer to FileChooser instance
 *
 * SIGNAL: Connected to "search-changed" signal
 */
static void on_search_changed(GtkSearchEntry *search, gpointer user_data)
{
    (void)search; /* Suppress unused parameter warning */
    FileChooser *chooser = (FileChooser*)user_data;

    /* Basic implementation: reload directory on search change */
    file_chooser_load_directory(chooser);
}

/**
 * on_create_clicked() - Handle Create button click.
 *
 * Validates the filename entry and if non-empty, constructs the full path
 * by combining current_path with the entered filename. Sets dialog as
 * completed so file_chooser_show() can return the selected path.
 *
 * VALIDATION: If filename is empty or whitespace-only, logs warning and
 *             leaves dialog open for user to correct.
 *
 * @param button   GtkButton that was clicked (unused)
 * @param user_data Pointer to FileChooser instance
 *
 * SIGNAL: Connected to "clicked" signal of Create button
 */
static void on_create_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; /* Suppress unused parameter warning */
    FileChooser *chooser = (FileChooser*)user_data;
    const char *filename = gtk_entry_get_text(GTK_ENTRY(chooser->filename_entry));

    if (!filename || strlen(filename) == 0) {
        g_warning("Please enter a filename or folder name");
        return;
    }

    /* Construct full path from current directory + entered name */
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", chooser->current_path, filename);

    strncpy(chooser->selected_path, full_path, sizeof(chooser->selected_path) - 1);
    chooser->completed = 1;
    gtk_widget_hide(chooser->dialog);
}

/**
 * on_cancel_clicked() - Handle Cancel button click.
 *
 * Closes the dialog without setting selected_path (marked by empty string).
 * Calling code detects empty selected_path to determine if user cancelled.
 *
 * @param button   GtkButton that was clicked (unused)
 * @param user_data Pointer to FileChooser instance
 *
 * SIGNAL: Connected to "clicked" signal of Cancel button
 */
static void on_cancel_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; /* Suppress unused parameter warning */
    FileChooser *chooser = (FileChooser*)user_data;
    chooser->selected_path[0] = '\0';  /* Clear selection to indicate cancellation */
    chooser->completed = 1;
    gtk_widget_hide(chooser->dialog);
}

/* ============================================================================
 * WINDOW DRAGGING SUPPORT
 * ============================================================================ */

/**
 * on_dialog_motion_notify() - Handle mouse movement for window dragging.
 *
 * Implements window drag-to-move behavior. When user holds mouse button while
 * dragging in titlebar area, window follows mouse movement.
 *
 * CALCULATION:
 *   new_x = drag_start_x + (current_mouse_x - drag_offset_x)
 *   new_y = drag_start_y + (current_mouse_y - drag_offset_y)
 *
 * @param widget   Event source widget
 * @param event    GdkEventMotion with current mouse position
 * @param user_data Pointer to FileChooser instance
 *
 * @return TRUE if event was handled (prevents further processing)
 *
 * SIGNAL: Connected to "motion-notify-event" signal
 */
static gboolean on_dialog_motion_notify(GtkWidget *widget, GdkEventMotion *event,
                                       gpointer user_data)
{
    FileChooser *chooser = (FileChooser*)user_data;

    if (drag_state.is_dragging && GTK_IS_WINDOW(chooser->dialog)) {
        /* Calculate new window position based on mouse movement */
        int new_x = drag_state.drag_start_x + (int)event->x_root - drag_state.drag_offset_x;
        int new_y = drag_state.drag_start_y + (int)event->y_root - drag_state.drag_offset_y;

        gtk_window_move(GTK_WINDOW(chooser->dialog), new_x, new_y);
        return TRUE;
    }

    return FALSE;
}

/**
 * on_titlebar_button_press() - Handle mouse button press on titlebar.
 *
 * Detects clicks in titlebar area (top ~30 pixels) to initiate window dragging.
 * Records starting position and current mouse offset for drag calculation.
 *
 * @param widget   GtkEventBox (titlebar area)
 * @param event    GdkEventButton with click position and button number
 * @param user_data Pointer to FileChooser instance
 *
 * @return TRUE if event was handled
 *
 * SIGNAL: Connected to "button-press-event" signal on titlebar event box
 */
static gboolean on_titlebar_button_press(GtkWidget *widget, GdkEventButton *event,
                                        gpointer user_data)
{
    FileChooser *chooser = (FileChooser*)user_data;

    if (event->button == 1) {  /* Left mouse button */
        drag_state.is_dragging = 1;

        /* Get current window position */
        gint win_x, win_y;
        gtk_window_get_position(GTK_WINDOW(chooser->dialog), &win_x, &win_y);

        drag_state.drag_start_x = win_x;
        drag_state.drag_start_y = win_y;
        drag_state.drag_offset_x = (int)event->x_root;
        drag_state.drag_offset_y = (int)event->y_root;

        return TRUE;
    }

    return FALSE;
}

/**
 * on_titlebar_button_release() - Handle mouse button release.
 *
 * Ends the window dragging operation started by on_titlebar_button_press().
 *
 * @param widget   GtkEventBox (titlebar area)
 * @param event    GdkEventButton with button number
 * @param user_data Pointer to FileChooser instance
 *
 * @return TRUE if event was handled
 *
 * SIGNAL: Connected to "button-release-event" signal on titlebar event box
 */
static gboolean on_titlebar_button_release(GtkWidget *widget, GdkEventButton *event,
                                          gpointer user_data)
{
    (void)widget;
    (void)user_data;

    if (event->button == 1) {  /* Left mouse button */
        drag_state.is_dragging = 0;
        return TRUE;
    }

    return FALSE;
}

/* ============================================================================
 * PUBLIC API - FILE CHOOSER CREATION & DISPLAY
 * ============================================================================ */

/**
 * file_chooser_new() - Create and initialize a new file chooser dialog.
 *
 * Allocates and sets up all GTK widgets including:
 *   - Main dialog window (600x500, centered, modal)
 *   - Navigation bar (Home, Up buttons, location entry)
 *   - Search text entry field
 *   - Main file list tree view with columns
 *   - Bottom filename entry field
 *   - Create/Cancel action buttons
 *
 * LAYOUT:
 *   [===== DIALOG WINDOW =====]
 *   | [Home] [Up] [Location..] |  <- Navigation bar
 *   | [Search field........] |  <- Search/filter
 *   |                        |
 *   |  Icon | Name | Size |  |  <- File list columns
 *   |  [Folder]  My Folder|4.0|
 *   |  [File] doc.txt     |2.3|
 *   |                        |
 *   | Name: [text field...] |
 *   | [Create] [Cancel]     |
 *   [======================]
 *
 * WINDOW PROPERTIES:
 *   - Decorated: Yes (window manager provides titlebar for dragging)
 *   - Modal: Yes (blocks interaction with other windows)
 *   - Resizable: Yes
 *   - Default size: 600x500 pixels
 *   - Centered on screen
 *
 * @param mode           CHOOSER_FILE or CHOOSER_FOLDER selection mode
 * @param initial_path   Starting directory (NULL = user's home directory)
 *
 * @return  Newly allocated FileChooser structure, or NULL on error
 *
 * MEMORY: Caller must free with file_chooser_destroy() when done
 *
 * ERRORS: Returns NULL if memory allocation fails or GTK initialization fails
 */
FileChooser* file_chooser_new(FileChooserMode mode, const char *initial_path)
{
    FileChooser *chooser = g_new0(FileChooser, 1);
    chooser->mode = mode;

    /* Set initial browsing path */
    if (initial_path) {
        strncpy(chooser->current_path, initial_path, sizeof(chooser->current_path) - 1);
    } else {
        /* Default to user's home directory */
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) home = pw->pw_dir;
        }
        strncpy(chooser->current_path, home ? home : "/", sizeof(chooser->current_path) - 1);
    }

    /* ================================================
     * CREATE MAIN DIALOG WINDOW
     * ================================================ */

    chooser->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chooser->dialog),
                        mode == CHOOSER_FOLDER ? "Create New Item" : "Select File");
    gtk_window_set_default_size(GTK_WINDOW(chooser->dialog), 700, 500);
    gtk_window_set_type_hint(GTK_WINDOW(chooser->dialog), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_modal(GTK_WINDOW(chooser->dialog), TRUE);
    gtk_window_set_gravity(GTK_WINDOW(chooser->dialog), GDK_GRAVITY_CENTER);
    gtk_window_set_position(GTK_WINDOW(chooser->dialog), GTK_WIN_POS_CENTER);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(chooser->dialog), main_vbox);

    /* ================================================
     * CREATE TITLEBAR (DRAGGABLE AREA)
     * ================================================ */

    GtkWidget *titlebar = gtk_event_box_new();
    gtk_widget_set_size_request(titlebar, -1, 35);
    gtk_widget_set_app_paintable(titlebar, TRUE);

    GtkWidget *title_label = gtk_label_new(
        mode == CHOOSER_FOLDER ? "Create New Item" : "Select File");
    gtk_widget_set_margin_start(title_label, 15);
    gtk_widget_set_margin_top(title_label, 8);

    gtk_container_add(GTK_CONTAINER(titlebar), title_label);
    gtk_box_pack_start(GTK_BOX(main_vbox), titlebar, FALSE, FALSE, 0);

    /* Connect drag events to titlebar */
    g_signal_connect(titlebar, "button-press-event",
                    G_CALLBACK(on_titlebar_button_press), chooser);
    g_signal_connect(titlebar, "button-release-event",
                    G_CALLBACK(on_titlebar_button_release), chooser);
    g_signal_connect(chooser->dialog, "motion-notify-event",
                    G_CALLBACK(on_dialog_motion_notify), chooser);

    /* ================================================
     * CREATE CONTENT AREA
     * ================================================ */

    GtkWidget *content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), content_vbox, TRUE, TRUE, 0);

    /* Navigation bar: Home, Up, Location */
    GtkWidget *nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    chooser->home_button = gtk_button_new_from_icon_name("go-home", GTK_ICON_SIZE_BUTTON);
    chooser->up_button = gtk_button_new_from_icon_name("go-up", GTK_ICON_SIZE_BUTTON);
    chooser->location_entry = gtk_entry_new();
    gtk_widget_set_hexpand(chooser->location_entry, TRUE);
    gtk_editable_set_editable(GTK_EDITABLE(chooser->location_entry), FALSE);

    gtk_box_pack_start(GTK_BOX(nav_box), chooser->home_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_box), chooser->up_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nav_box), chooser->location_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content_vbox), nav_box, FALSE, FALSE, 0);

    /* Search entry field */
    GtkWidget *search_label = gtk_label_new("Search:");
    chooser->search_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(chooser->search_entry, TRUE);

    GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(search_box), search_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), chooser->search_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content_vbox), search_box, FALSE, FALSE, 0);

    /* File list tree view with columns */
    chooser->file_store = gtk_list_store_new(NUM_COLS,
                                            G_TYPE_STRING,  /* Icon name */
                                            G_TYPE_STRING,  /* Filename */
                                            G_TYPE_STRING,  /* File size */
                                            G_TYPE_STRING,  /* File type */
                                            G_TYPE_STRING,  /* Modified time */
                                            G_TYPE_STRING); /* Full path (hidden) */

    chooser->file_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(chooser->file_store));

    /* Setup tree view columns */
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* Icon column (narrow, icon only) */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("", renderer, "text", COL_ICON, NULL);
    gtk_tree_view_column_set_fixed_width(column, 30);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chooser->file_list), column);

    /* Name column (wide, main display) */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_min_width(column, 200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chooser->file_list), column);

    /* Size column (numeric, right-aligned) */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", COL_SIZE, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_fixed_width(column, 80);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chooser->file_list), column);

    /* Type column (File/Folder) */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", COL_TYPE, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_fixed_width(column, 70);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chooser->file_list), column);

    /* Modified column (date/time) */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", COL_MODIFIED, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_fixed_width(column, 150);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chooser->file_list), column);

    /* Wrap tree view in scroll window */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), chooser->file_list);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(content_vbox), scrolled, TRUE, TRUE, 0);

    /* Filename entry field (for selecting/creating new file) */
    GtkWidget *filename_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *filename_label = gtk_label_new("Name:");
    chooser->filename_entry = gtk_entry_new();
    gtk_widget_set_hexpand(chooser->filename_entry, TRUE);

    gtk_box_pack_start(GTK_BOX(filename_box), filename_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filename_box), chooser->filename_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content_vbox), filename_box, FALSE, FALSE, 0);

    /* ================================================
     * CREATE ACTION BUTTONS
     * ================================================ */

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);

    GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
    GtkWidget *create_button = gtk_button_new_with_label("Create");

    gtk_box_pack_end(GTK_BOX(button_box), create_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_vbox), button_box, FALSE, FALSE, 0);

    /* ================================================
     * CONNECT SIGNAL HANDLERS
     * ================================================ */

    /* Navigation signals */
    g_signal_connect(chooser->home_button, "clicked",
                    G_CALLBACK(on_home_clicked), chooser);
    g_signal_connect(chooser->up_button, "clicked",
                    G_CALLBACK(on_up_clicked), chooser);

    /* File list interaction */
    g_signal_connect(chooser->file_list, "row-activated",
                    G_CALLBACK(on_row_activated), chooser);

    /* Search field */
    g_signal_connect(chooser->search_entry, "search-changed",
                    G_CALLBACK(on_search_changed), chooser);

    /* Action buttons */
    g_signal_connect(create_button, "clicked",
                    G_CALLBACK(on_create_clicked), chooser);
    g_signal_connect(cancel_button, "clicked",
                    G_CALLBACK(on_cancel_clicked), chooser);

    /* Enable dragging (motion events must be enabled for move detection) */
    gtk_widget_add_events(chooser->dialog, GDK_POINTER_MOTION_MASK);

    /* Show all widgets */
    gtk_widget_show_all(main_vbox);

    /* Load initial directory listing */
    file_chooser_load_directory(chooser);

    return chooser;
}

/**
 * file_chooser_show() - Display dialog modally and return user's selection.
 *
 * Shows the file chooser dialog and blocks until user completes the action:
 * - Returns path if user selects a file/folder or creates a new item
 * - Returns NULL if user clicks Cancel
 *
 * FLOW:
 * 1. Reset completion flag and selection
 * 2. Show dialog window
 * 3. Process GTK events until user clicks Create or Cancel
 * 4. Hide dialog
 * 5. Return selected path (or NULL if cancelled)
 *
 * NOTE: This is NOT a blocking modal in the GTK sense because the calling
 *       process is the only one using the dialog. Real blocking would require
 *       gtk_dialog_run() but we manage completion manually for better control.
 *
 * @param chooser  FileChooser instance from file_chooser_new()
 *
 * @return  Allocated string with selected file path, or NULL if cancelled
 *          Caller must free returned string with g_free()
 */
char* file_chooser_show(FileChooser *chooser)
{
    if (!chooser || !chooser->dialog) return NULL;

    chooser->completed = 0;
    chooser->selected_path[0] = '\0';

    gtk_widget_show(chooser->dialog);

    /* Process GTK events until dialog is completed */
    while (!chooser->completed) {
        if (gtk_events_pending()) {
            gtk_main_iteration();
        } else {
            g_usleep(10000);  /* Sleep 10ms to prevent busy-wait */
        }
    }

    gtk_widget_hide(chooser->dialog);

    /* Return selected path (allocated string) or NULL if cancelled */
    if (chooser->selected_path[0] != '\0') {
        return g_strdup(chooser->selected_path);
    }
    return NULL;
}

/**
 * file_chooser_destroy() - Clean up file chooser and free all resources.
 *
 * Destroys GTK widgets and deallocates the FileChooser structure.
 * Should be called when dialog is no longer needed to prevent memory leaks.
 *
 * @param chooser  FileChooser instance (nullpointer safe)
 */
void file_chooser_destroy(FileChooser *chooser)
{
    if (!chooser) return;

    if (chooser->dialog) {
        gtk_widget_destroy(chooser->dialog);
    }

    g_free(chooser);
}
