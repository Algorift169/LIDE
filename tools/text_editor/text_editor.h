#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * text_editor.h
 *
 * Text editor interface and plugin system definitions.
 * Declares text editing callbacks, syntax highlighting hooks,
 * and file I/O operations.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for a system architecture overview.
 *
 * Editor main application structure.
 * Encapsulates all UI components and state for the text editor.
 */
typedef struct {
    GtkWidget *window;           /* Main application window */
    GtkWidget *text_view;        /* GtkTextView widget for editing */
    GtkTextBuffer *buffer;       /* Text buffer containing document content */
    GtkWidget *status_label;     /* Status bar label showing file info */
    GtkWidget *find_dialog;      /* Find/Replace dialog widget */
    GtkWidget *find_entry;       /* Entry for search text in find dialog */
    GtkWidget *replace_entry;    /* Entry for replacement text in replace dialog */
    char *current_file;          /* Current file path (NULL if unsaved) */
    gboolean modified;           /* TRUE if document has unsaved changes */

    /* Window manipulation state */
    int is_dragging;             /* TRUE while user is dragging the window */
    int is_resizing;             /* TRUE while user is resizing the window */
    int resize_edge;             /* Which edge/corner is being resized (from window_resize.h) */
    int drag_start_x;            /* Initial mouse X position for drag operation */
    int drag_start_y;            /* Initial mouse Y position for drag operation */
} Editor;

/* Document operations */

/**
 * Creates a new empty document.
 * Clears the current buffer and resets file state.
 *
 * @param ed Editor instance.
 */
void editor_new(Editor *ed);

/**
 * Opens a file selection dialog and loads the selected file.
 *
 * @param ed Editor instance.
 *
 * @sideeffect Loads file contents into buffer on success.
 * @sideeffect Updates window title with filename.
 */
void editor_open(Editor *ed);

/**
 * Saves the current document.
 * If no filename exists, prompts for save location.
 *
 * @param ed Editor instance.
 */
void editor_save(Editor *ed);

/**
 * Prompts for a new filename and saves the current document.
 *
 * @param ed Editor instance.
 */
void editor_save_as(Editor *ed);

/**
 * Closes the current document.
 * Prompts to save changes if modified.
 *
 * @param ed Editor instance.
 */
void editor_close(Editor *ed);

/* Edit operations */

/**
 * Cuts the selected text to clipboard.
 *
 * @param ed Editor instance.
 */
void editor_cut(Editor *ed);

/**
 * Copies the selected text to clipboard.
 *
 * @param ed Editor instance.
 */
void editor_copy(Editor *ed);

/**
 * Pastes text from clipboard at cursor position.
 *
 * @param ed Editor instance.
 */
void editor_paste(Editor *ed);

/**
 * Undoes the last text edit operation.
 *
 * @param ed Editor instance.
 */
void editor_undo(Editor *ed);

/**
 * Redoes the last undone text edit operation.
 *
 * @param ed Editor instance.
 */
void editor_redo(Editor *ed);

/* Search operations */

/**
 * Opens the find dialog.
 *
 * @param ed Editor instance.
 */
void editor_find(Editor *ed);

/**
 * Finds the next occurrence of the search term.
 *
 * @param ed Editor instance.
 */
void editor_find_next(Editor *ed);

/**
 * Opens the replace dialog.
 *
 * @param ed Editor instance.
 */
void editor_replace(Editor *ed);

/**
 * Replaces all occurrences of the search term in the document.
 *
 * @param ed Editor instance.
 */
void editor_replace_all(Editor *ed);

/* UI helpers */

/**
 * Updates the window title with current filename and modified status.
 *
 * @param ed Editor instance.
 */
void editor_update_title(Editor *ed);

/**
 * Updates the status bar with current cursor position and document statistics.
 *
 * @param ed Editor instance.
 */
void editor_update_status(Editor *ed);

/**
 * Prepares a dialog for display by applying theme styling.
 *
 * @param dialog The dialog widget to style.
 */
void prepare_dialog(GtkWidget *dialog);

#endif /* TEXT_EDITOR_H */