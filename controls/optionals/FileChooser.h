#ifndef FILE_CHOOSER_H
#define FILE_CHOOSER_H

#include <gtk/gtk.h>

typedef enum {
    CHOOSER_FILE,     // Select a file
    CHOOSER_FOLDER    // Select or create a folder
} FileChooserMode;

typedef struct {
    GtkWidget *dialog;
    GtkWidget *location_entry;     // Current path display
    GtkWidget *search_entry;       // Search/filter text field
    GtkWidget *file_list;          // GtkTreeView for file listing with icons
    GtkWidget *filename_entry;     // Text input for new file/folder name
    GtkListStore *file_store;      // GtkListStore: Icon, Name, Size, Type, Modified, FullPath
    GtkWidget *up_button;          // Up/parent directory button
    GtkWidget *home_button;        // Home directory button
    char current_path[1024];        // Currently browsing path
    char selected_path[1024];       // Final selected path
    int completed;                 // Flag when dialog is done
    FileChooserMode mode;          // CHOOSER_FILE or CHOOSER_FOLDER
} FileChooser;

/**
 * Create a new file chooser dialog with the specified mode and initial path.
 */
FileChooser* file_chooser_new(FileChooserMode mode, const char *initial_path);

/**
 * Show the file chooser dialog modally and return the selected path.
 * Returns allocated string or NULL if cancelled.
 */
char* file_chooser_show(FileChooser *chooser);

/**
 * Destroy the file chooser and free all resources.
 */
void file_chooser_destroy(FileChooser *chooser);

#endif // FILE_CHOOSER_H
