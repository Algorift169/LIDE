#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include "file-roller.h"
#include "../controls/optionals/FileChooser.h"

/**
 * file-roller.c
 *
 * Universal file viewer/editor for LIDE desktop environment.
 * Supports images, text, PDFs, videos, archives, and more.
 * Text files can be edited and saved.
 * Uses custom FileChooser dialog for all file operations.
 *
 * This module is part of the LIDE desktop environment system.
 */

/* ========== FILE TYPE DETECTION ========== */

/**
 * Get file type from filename/extension
 */
FileType get_file_type(const char *filename)
{
    if (!filename) return FILE_TYPE_UNKNOWN;

    /* Get file extension */
    const char *ext = strrchr(filename, '.');
    if (!ext) return FILE_TYPE_UNKNOWN;

    ext++; /* Skip the dot */
    char ext_lower[256];
    strncpy(ext_lower, ext, sizeof(ext_lower) - 1);
    ext_lower[sizeof(ext_lower) - 1] = '\0';

    /* Convert to lowercase */
    for (int i = 0; ext_lower[i]; i++) {
        ext_lower[i] = tolower((unsigned char)ext_lower[i]);
    }

    /* Check image formats */
    if (strcmp(ext_lower, "png") == 0 || strcmp(ext_lower, "jpg") == 0 ||
        strcmp(ext_lower, "jpeg") == 0 || strcmp(ext_lower, "gif") == 0 ||
        strcmp(ext_lower, "bmp") == 0 || strcmp(ext_lower, "webp") == 0 ||
        strcmp(ext_lower, "tiff") == 0 || strcmp(ext_lower, "svg") == 0) {
        return FILE_TYPE_IMAGE;
    }

    /* Check text formats */
    if (strcmp(ext_lower, "txt") == 0 || strcmp(ext_lower, "c") == 0 ||
        strcmp(ext_lower, "h") == 0 || strcmp(ext_lower, "py") == 0 ||
        strcmp(ext_lower, "js") == 0 || strcmp(ext_lower, "html") == 0 ||
        strcmp(ext_lower, "css") == 0 || strcmp(ext_lower, "json") == 0 ||
        strcmp(ext_lower, "xml") == 0 || strcmp(ext_lower, "sh") == 0 ||
        strcmp(ext_lower, "conf") == 0 || strcmp(ext_lower, "cfg") == 0 ||
        strcmp(ext_lower, "md") == 0 || strcmp(ext_lower, "log") == 0) {
        return FILE_TYPE_TEXT;
    }

    /* Check PDF format */
    if (strcmp(ext_lower, "pdf") == 0) {
        return FILE_TYPE_PDF;
    }

    /* Check video formats */
    if (strcmp(ext_lower, "mp4") == 0 || strcmp(ext_lower, "webm") == 0 ||
        strcmp(ext_lower, "mkv") == 0 || strcmp(ext_lower, "avi") == 0 ||
        strcmp(ext_lower, "mov") == 0 || strcmp(ext_lower, "flv") == 0 ||
        strcmp(ext_lower, "wmv") == 0 || strcmp(ext_lower, "m4v") == 0) {
        return FILE_TYPE_VIDEO;
    }

    /* Check audio formats */
    if (strcmp(ext_lower, "mp3") == 0 || strcmp(ext_lower, "wav") == 0 ||
        strcmp(ext_lower, "flac") == 0 || strcmp(ext_lower, "aac") == 0 ||
        strcmp(ext_lower, "ogg") == 0 || strcmp(ext_lower, "m4a") == 0 ||
        strcmp(ext_lower, "wma") == 0) {
        return FILE_TYPE_AUDIO;
    }

    /* Check archive formats */
    if (strcmp(ext_lower, "zip") == 0 || strcmp(ext_lower, "tar") == 0 ||
        strcmp(ext_lower, "gz") == 0 || strcmp(ext_lower, "7z") == 0 ||
        strcmp(ext_lower, "rar") == 0 || strcmp(ext_lower, "xz") == 0 ||
        strcmp(ext_lower, "bz2") == 0) {
        return FILE_TYPE_ARCHIVE;
    }

    return FILE_TYPE_UNKNOWN;
}

/**
 * Get human readable file type name
 */
const char *get_file_type_name(FileType type)
{
    switch (type) {
        case FILE_TYPE_IMAGE:
            return "Image";
        case FILE_TYPE_TEXT:
            return "Text Document";
        case FILE_TYPE_PDF:
            return "PDF Document";
        case FILE_TYPE_VIDEO:
            return "Video";
        case FILE_TYPE_AUDIO:
            return "Audio";
        case FILE_TYPE_ARCHIVE:
            return "Archive";
        default:
            return "Unknown";
    }
}

/**
 * Check if file type is supported
 */
gboolean is_file_type_supported(FileType type)
{
    return (type != FILE_TYPE_UNKNOWN);
}

/* ========== APPLICATION STATE ========== */

typedef enum {
    DIALOG_MODE_OPEN,
    DIALOG_MODE_OPEN_FOLDER
} DialogMode;

typedef struct {
    GtkWidget *window;              /* Main window */
    GtkWidget *stack;               /* View stack for different file types */
    GtkWidget *statusbar;           /* Status bar */
    GtkWidget *header_bar;          /* Header bar for title */

    /* Image viewer widgets */
    GtkWidget *image_viewer;        /* Image display area */
    GtkWidget *image_scroll;        /* Scroll container for image */
    GdkPixbuf *current_pixbuf;      /* Current image pixbuf */
    GdkPixbuf *original_pixbuf;     /* Original image pixbuf */
    double zoom_level;              /* Zoom level (1.0 = 100%) */

    /* Text viewer/editor widgets */
    GtkWidget *text_view;           /* Text display/editing area */
    GtkWidget *text_scroll;         /* Scroll container for text */
    gboolean text_modified;         /* Whether text has unsaved changes */

    /* PDF viewer stub */
    GtkWidget *pdf_label;           /* Placeholder for PDF */

    /* Video viewer stub */
    GtkWidget *video_label;         /* Placeholder for video */

    /* Archive browser stub */
    GtkWidget *archive_tree;        /* Archive contents tree */
    GtkWidget *archive_scroll;      /* Scroll container */

    /* File properties */
    char *current_filename;         /* Currently opened file path */
    char *current_folder;           /* Current folder for dialogs */
    FileType current_file_type;     /* Type of current file */
    gboolean is_modified;           /* Whether file has unsaved changes */
    
    /* Window dragging */
    gboolean is_dragging;
    int drag_start_x, drag_start_y;
    int window_start_x, window_start_y;
    gboolean is_resizing;
    int resize_edge;

} AppState;

/* ========== FORWARD DECLARATIONS ========== */
static void open_file(AppState *state, const char *filename);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AppState *state);
static void save_file(AppState *state);
static void save_file_as(AppState *state);
static void update_window_title(AppState *state);

/* ========== WINDOW CONTROL FUNCTIONS ========== */

static void on_minimize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

static void on_maximize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    if (gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window))) & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_window_unmaximize(GTK_WINDOW(window));
    } else {
        gtk_window_maximize(GTK_WINDOW(window));
    }
}

static void on_close_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

/* ========== WINDOW DRAGGING ========== */

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    AppState *state = (AppState *)data;
    
    if (event->button == 1) {
        state->is_dragging = TRUE;
        state->drag_start_x = event->x_root;
        state->drag_start_y = event->y_root;
        gtk_window_get_position(GTK_WINDOW(state->window), &state->window_start_x, &state->window_start_y);
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    AppState *state = (AppState *)data;
    
    if (event->button == 1) {
        state->is_dragging = FALSE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    AppState *state = (AppState *)data;
    
    if (state->is_dragging) {
        int dx = event->x_root - state->drag_start_x;
        int dy = event->y_root - state->drag_start_y;
        gtk_window_move(GTK_WINDOW(state->window), 
                       state->window_start_x + dx, 
                       state->window_start_y + dy);
        return TRUE;
    }
    return FALSE;
}

/* ========== IMAGE VIEWER FUNCTIONS ========== */

/**
 * Scale pixbuf for display while preserving aspect ratio
 */
static GdkPixbuf *scale_pixbuf(GdkPixbuf *pixbuf, int max_width, int max_height)
{
    if (!pixbuf) return NULL;

    int orig_width = gdk_pixbuf_get_width(pixbuf);
    int orig_height = gdk_pixbuf_get_height(pixbuf);

    double scale_x = (double)max_width / orig_width;
    double scale_y = (double)max_height / orig_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    if (scale >= 1.0) {
        return g_object_ref(pixbuf);
    }

    int new_width = (int)(orig_width * scale);
    int new_height = (int)(orig_height * scale);

    return gdk_pixbuf_scale_simple(pixbuf, new_width, new_height, GDK_INTERP_BILINEAR);
}

/**
 * Load and display image file
 */
static void load_image(AppState *state, const char *filename)
{
    if (!filename) return;

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);

    if (error) {
        g_error_free(error);
        gtk_label_set_text(GTK_LABEL(state->image_viewer), "Failed to load image");
        return;
    }

    if (!pixbuf) {
        gtk_label_set_text(GTK_LABEL(state->image_viewer), "Could not open image");
        return;
    }

    if (state->original_pixbuf) g_object_unref(state->original_pixbuf);
    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);

    state->original_pixbuf = pixbuf;
    state->zoom_level = 1.0;

    /* Scale for display */
    GdkPixbuf *scaled = scale_pixbuf(pixbuf, 600, 400);
    state->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);

    char status[512];
    snprintf(status, sizeof(status), "%s - %dx%d pixels | Zoom: 100%%",
             g_path_get_basename(filename),
             gdk_pixbuf_get_width(pixbuf),
             gdk_pixbuf_get_height(pixbuf));

    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "image_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/**
 * Zoom in on image
 */
static void zoom_in(AppState *state)
{
    if (!state->original_pixbuf) return;

    state->zoom_level *= 1.2;
    if (state->zoom_level > 5.0) state->zoom_level = 5.0;

    int new_width = (int)(gdk_pixbuf_get_width(state->original_pixbuf) * state->zoom_level);
    int new_height = (int)(gdk_pixbuf_get_height(state->original_pixbuf) * state->zoom_level);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(state->original_pixbuf, new_width, new_height,
                                                 GDK_INTERP_BILINEAR);
    if (!scaled) return;

    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
    state->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);

    char status[256];
    snprintf(status, sizeof(status), "Zoom: %.0f%%", state->zoom_level * 100);
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "zoom");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/**
 * Zoom out on image
 */
static void zoom_out(AppState *state)
{
    if (!state->original_pixbuf) return;

    state->zoom_level /= 1.2;
    if (state->zoom_level < 0.1) state->zoom_level = 0.1;

    int new_width = (int)(gdk_pixbuf_get_width(state->original_pixbuf) * state->zoom_level);
    int new_height = (int)(gdk_pixbuf_get_height(state->original_pixbuf) * state->zoom_level);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(state->original_pixbuf, new_width, new_height,
                                                 GDK_INTERP_BILINEAR);
    if (!scaled) return;

    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
    state->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);

    char status[256];
    snprintf(status, sizeof(status), "Zoom: %.0f%%", state->zoom_level * 100);
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "zoom");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/**
 * Fit image to window
 */
static void fit_image_to_window(AppState *state)
{
    if (!state->original_pixbuf) return;

    state->zoom_level = 1.0;
    GdkPixbuf *scaled = scale_pixbuf(state->original_pixbuf, 600, 400);

    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
    state->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);

    char status[256];
    snprintf(status, sizeof(status), "Zoom: Fit to window");
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "zoom");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/* ========== TEXT EDITOR FUNCTIONS ========== */

/**
 * Callback when text buffer is modified
 */
static void on_text_buffer_changed(GtkTextBuffer *buffer, AppState *state)
{
    (void)buffer;
    if (!state->text_modified) {
        state->text_modified = TRUE;
        state->is_modified = TRUE;
        update_window_title(state);
    }
}

/**
 * Load and display text file (editable)
 */
static void load_text_file(AppState *state, const char *filename)
{
    if (!filename) return;

    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(filename, &contents, &length, &error)) {
        gchar error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Error loading file: %s",
                 error ? error->message : "Unknown error");
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                 error_msg, -1);
        if (error) g_error_free(error);
        return;
    }

    /* Make text view editable */
    gtk_text_view_set_editable(GTK_TEXT_VIEW(state->text_view), TRUE);
    
    /* Set text content */
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                             contents ? contents : "", -1);
    
    /* Reset modified flag */
    state->text_modified = FALSE;
    state->is_modified = FALSE;
    
    g_free(contents);

    char status[512];
    snprintf(status, sizeof(status), "Text File: %s | Size: %ld bytes | Editable",
             g_path_get_basename(filename), length);
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/**
 * Save current file using FileChooser dialog
 */
static void save_file(AppState *state)
{
    if (!state->current_filename) {
        save_file_as(state);
        return;
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    GError *error = NULL;
    if (!g_file_set_contents(state->current_filename, text, -1, &error)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Failed to save file: %s",
                                                   error ? error->message : "Unknown error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (error) g_error_free(error);
    } else {
        state->text_modified = FALSE;
        state->is_modified = FALSE;
        update_window_title(state);
        
        /* Show success status */
        guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
        gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
        gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, "File saved successfully");
    }
    
    g_free(text);
}

/**
 * Save file as using custom FileChooser dialog
 */
static void save_file_as(AppState *state)
{
    const char *initial_dir = state->current_folder ? state->current_folder : g_get_home_dir();
    
    /* Use SAVE action for Save As */
    FileChooser *chooser = file_chooser_new(CHOOSER_FILE, CHOOSER_ACTION_SAVE, initial_dir);
    if (!chooser) return;
    
    if (state->current_filename) {
        const char *basename = g_path_get_basename(state->current_filename);
        gtk_entry_set_text(GTK_ENTRY(chooser->filename_entry), basename);
        g_free((void*)basename);
    } else {
        gtk_entry_set_text(GTK_ENTRY(chooser->filename_entry), "untitled.txt");
    }
    
    char *selected_path = file_chooser_show(chooser);
    
    if (selected_path) {
        if (state->current_folder) g_free(state->current_folder);
        state->current_folder = g_path_get_dirname(selected_path);
        
        if (state->current_filename) g_free(state->current_filename);
        state->current_filename = g_strdup(selected_path);
        
        save_file(state);
        update_window_title(state);
        
        g_free(selected_path);
    }
    
    file_chooser_destroy(chooser);
}

/**
 * Open file using custom FileChooser dialog
 */
static void open_file_dialog(AppState *state)
{
    const char *initial_dir = state->current_folder ? state->current_folder : g_get_home_dir();
    
    /* Use CREATE action for opening (selecting existing file) */
    FileChooser *chooser = file_chooser_new(CHOOSER_FILE, CHOOSER_ACTION_CREATE, initial_dir);
    if (!chooser) return;
    
    char *selected_path = file_chooser_show(chooser);
    
    if (selected_path) {
        if (state->current_folder) g_free(state->current_folder);
        state->current_folder = g_path_get_dirname(selected_path);
        
        open_file(state, selected_path);
        
        g_free(selected_path);
    }
    
    file_chooser_destroy(chooser);
}
/**
 * Update window title based on current file and modified state
 */
static void update_window_title(AppState *state)
{
    char title[512];
    const char *basename = state->current_filename ? 
                           g_path_get_basename(state->current_filename) : "Untitled";
    
    if (state->is_modified) {
        snprintf(title, sizeof(title), "File Roller - %s* (%s)", 
                 basename, get_file_type_name(state->current_file_type));
    } else {
        snprintf(title, sizeof(title), "File Roller - %s (%s)", 
                 basename, get_file_type_name(state->current_file_type));
    }
    
    gtk_window_set_title(GTK_WINDOW(state->window), title);
    
    if (state->current_filename && basename != state->current_filename) {
        g_free((void*)basename);
    }
}

/* ========== PDF VIEWER FUNCTIONS ========== */

/**
 * Load PDF file (stub - shows info)
 */
static void load_pdf_file(AppState *state, const char *filename)
{
    if (!filename) return;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "PDF File: %s\n\nSize: %ld bytes\n\n"
                 "For PDF viewing, use:\n  • evince (GNOME Document Viewer)\n  • okular (KDE)\n  • qpdfview\n\n"
                 "To open this file:\n$ evince \"%s\"",
                 g_path_get_basename(filename),
                 statbuf.st_size,
                 filename);
        gtk_label_set_text(GTK_LABEL(state->pdf_label), msg);
        gtk_label_set_line_wrap(GTK_LABEL(state->pdf_label), TRUE);
    }

    char status[512];
    snprintf(status, sizeof(status), "PDF Reader - Open with external PDF viewer");
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/* ========== VIDEO VIEWER FUNCTIONS ========== */

/**
 * Load video file (stub - shows info)
 */
static void load_video_file(AppState *state, const char *filename)
{
    if (!filename) return;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "Video File: %s\n\nSize: %ld bytes\n\n"
                 "For video playback, use:\n  • vlc (VLC Media Player)\n  • mpv\n  • ffplay\n\n"
                 "To open this file:\n$ vlc \"%s\"",
                 g_path_get_basename(filename),
                 statbuf.st_size,
                 filename);
        gtk_label_set_text(GTK_LABEL(state->video_label), msg);
        gtk_label_set_line_wrap(GTK_LABEL(state->video_label), TRUE);
    }

    char status[512];
    snprintf(status, sizeof(status), "Video Player - Open with external media player");
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/* ========== ARCHIVE VIEWER FUNCTIONS ========== */

/**
 * Load archive file (stub - shows info)
 */
static void load_archive_file(AppState *state, const char *filename)
{
    if (!filename) return;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "Archive File: %s\n\nSize: %ld bytes\n\n"
                 "For archive extraction, use:\n  • file-roller (GNOME)\n  • ark (KDE)\n  • Xarchiver\n\n"
                 "To extract this file:\n$ tar -xf \"%s\"",
                 g_path_get_basename(filename),
                 statbuf.st_size,
                 filename);
        gtk_label_set_text(GTK_LABEL(state->archive_tree), msg);
    }

    char status[512];
    snprintf(status, sizeof(status), "Archive Viewer - Use external tool for extraction");
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/**
 * Load and display unknown/binary file
 */
static void load_unknown_file(AppState *state, const char *filename)
{
    if (!filename) return;

    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;

    if (g_file_get_contents(filename, &contents, &length, &error)) {
        gboolean is_text = TRUE;
        
        int null_count = 0;
        for (gsize i = 0; i < length && i < 8192; i++) {
            if (contents[i] == '\0') {
                null_count++;
            }
        }
        
        if (null_count > length / 100) {
            is_text = FALSE;
        }

        if (is_text && length > 0) {
            gtk_text_view_set_editable(GTK_TEXT_VIEW(state->text_view), FALSE);
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                     contents, -1);
        } else if (length == 0) {
            gtk_text_view_set_editable(GTK_TEXT_VIEW(state->text_view), FALSE);
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                     "[Empty file]", -1);
        } else {
            gtk_text_view_set_editable(GTK_TEXT_VIEW(state->text_view), FALSE);
            gchar *hex_preview = g_malloc(1024);
            gchar *ptr = hex_preview;
            int bytes_to_show = (length > 256) ? 256 : length;
            
            ptr += snprintf(ptr, 1024, "Binary File: %s\nSize: %ld bytes\n\nFirst %d bytes (hex):\n\n",
                           g_path_get_basename(filename), length, bytes_to_show);
            
            for (int i = 0; i < bytes_to_show; i += 16) {
                ptr += snprintf(ptr, 1024, "%04X: ", i);
                for (int j = 0; j < 16 && (i + j) < bytes_to_show; j++) {
                    ptr += snprintf(ptr, 1024, "%02X ", (unsigned char)contents[i + j]);
                }
                ptr += snprintf(ptr, 1024, "\n");
            }
            
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                     hex_preview, -1);
            g_free(hex_preview);
        }
        g_free(contents);
    } else if (error) {
        gchar error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Error loading file: %s",
                 error->message);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                 error_msg, -1);
        g_error_free(error);
        return;
    }

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        char status[512];
        snprintf(status, sizeof(status), "Unknown File: %s | Size: %ld bytes | Type: %s",
                 g_path_get_basename(filename), statbuf.st_size,
                 get_file_type_name(state->current_file_type));
        guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
        gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
        gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
    }
}

/**
 * Drag and drop callback for file handling
 */
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                   gint x, gint y, GtkSelectionData *data,
                                   guint info, guint time, gpointer user_data)
{
    AppState *state = (AppState *)user_data;
    
    const guchar *drop_data = gtk_selection_data_get_data(data);
    if (!drop_data) return;

    gchar *file_path = NULL;
    if (g_str_has_prefix((const gchar *)drop_data, "file://")) {
        file_path = g_filename_from_uri((const gchar *)drop_data, NULL, NULL);
    } else {
        file_path = g_strdup((const gchar *)drop_data);
    }

    if (file_path) {
        gchar *trimmed = g_strstrip(file_path);
        open_file(state, trimmed);
    }

    gtk_drag_finish(context, TRUE, FALSE, time);
}

/* ========== GENERIC FILE FUNCTIONS ========== */

/**
 * Open any type of file
 */
static void open_file(AppState *state, const char *filename)
{
    if (!filename) return;

    if (state->current_filename) g_free(state->current_filename);
    state->current_filename = g_strdup(filename);

    state->current_file_type = get_file_type(filename);
    state->is_modified = FALSE;
    state->text_modified = FALSE;

    switch (state->current_file_type) {
        case FILE_TYPE_IMAGE:
            gtk_stack_set_visible_child_name(GTK_STACK(state->stack), "image");
            load_image(state, filename);
            break;
        case FILE_TYPE_TEXT:
            gtk_stack_set_visible_child_name(GTK_STACK(state->stack), "text");
            load_text_file(state, filename);
            break;
        case FILE_TYPE_PDF:
            gtk_stack_set_visible_child_name(GTK_STACK(state->stack), "pdf");
            load_pdf_file(state, filename);
            break;
        case FILE_TYPE_VIDEO:
            gtk_stack_set_visible_child_name(GTK_STACK(state->stack), "video");
            load_video_file(state, filename);
            break;
        case FILE_TYPE_AUDIO:
        case FILE_TYPE_ARCHIVE:
            gtk_stack_set_visible_child_name(GTK_STACK(state->stack), "archive");
            load_archive_file(state, filename);
            break;
        default:
            gtk_stack_set_visible_child_name(GTK_STACK(state->stack), "text");
            load_unknown_file(state, filename);
            break;
    }
    
    update_window_title(state);
}

/* ========== TOOLBAR CALLBACKS ========== */

static void on_zoom_in(GtkWidget *widget, AppState *state)
{
    if (state->current_file_type == FILE_TYPE_IMAGE) {
        zoom_in(state);
    }
}

static void on_zoom_out(GtkWidget *widget, AppState *state)
{
    if (state->current_file_type == FILE_TYPE_IMAGE) {
        zoom_out(state);
    }
}

static void on_fit_window(GtkWidget *widget, AppState *state)
{
    if (state->current_file_type == FILE_TYPE_IMAGE) {
        fit_image_to_window(state);
    }
}

static void on_open_file(GtkWidget *widget, AppState *state)
{
    open_file_dialog(state);
}

static void on_save_file(GtkWidget *widget, AppState *state)
{
    save_file(state);
}

static void on_save_file_as(GtkWidget *widget, AppState *state)
{
    save_file_as(state);
}

/**
 * Show file properties dialog
 */
static void on_show_properties(GtkWidget *widget, AppState *state)
{
    if (!state->current_filename) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "No file is currently open");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    struct stat statbuf;
    if (stat(state->current_filename, &statbuf) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Could not get file information");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char msg[1024];
    char time_buf[64];
    struct tm *tm_info = localtime(&statbuf.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    snprintf(msg, sizeof(msg),
             "File: %s\n\n"
             "Size: %ld bytes (%.2f KB)\n"
             "Type: %s\n"
             "Modified: %s\n"
             "Permissions: %o\n"
             "Owner UID: %d\n"
             "Group GID: %d",
             g_path_get_basename(state->current_filename),
             statbuf.st_size,
             (double)statbuf.st_size / 1024.0,
             get_file_type_name(state->current_file_type),
             time_buf,
             statbuf.st_mode & 0777,
             statbuf.st_uid,
             statbuf.st_gid);

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "File Properties");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * Copy text to clipboard
 */
static void on_copy_text(GtkWidget *widget, AppState *state)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (text && strlen(text) > 0) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, text, -1);
        
        guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "clipboard");
        gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
        gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, "Text copied to clipboard");
    }

    if (text) g_free(text);
}

/* ========== WINDOW SETUP ========== */

/**
 * Create image viewer page
 */
static GtkWidget *create_image_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    state->image_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(state->image_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    state->image_viewer = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(state->image_scroll), state->image_viewer);

    gtk_box_pack_start(GTK_BOX(vbox), state->image_scroll, TRUE, TRUE, 0);

    return vbox;
}

/**
 * Create text viewer/editor page
 */
static GtkWidget *create_text_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    state->text_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(state->text_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    state->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(state->text_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->text_view), GTK_WRAP_WORD);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
    g_signal_connect(buffer, "changed", G_CALLBACK(on_text_buffer_changed), state);

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, 
                                    "textview { font-family: Monospace; font-size: 11pt; }", 
                                    -1, NULL);
    GtkStyleContext *style_context = gtk_widget_get_style_context(state->text_view);
    gtk_style_context_add_provider(style_context, 
                                   GTK_STYLE_PROVIDER(css_provider), 
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    gtk_container_add(GTK_CONTAINER(state->text_scroll), state->text_view);
    gtk_box_pack_start(GTK_BOX(vbox), state->text_scroll, TRUE, TRUE, 0);

    return vbox;
}

/**
 * Create PDF viewer page (stub)
 */
static GtkWidget *create_pdf_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    state->pdf_label = gtk_label_new("PDF Viewer\n\nSelect a PDF file to view");
    gtk_label_set_selectable(GTK_LABEL(state->pdf_label), TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), state->pdf_label, TRUE, TRUE, 0);

    return vbox;
}

/**
 * Create video viewer page (stub)
 */
static GtkWidget *create_video_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    state->video_label = gtk_label_new("Video Player\n\nSelect a video file to view");
    gtk_label_set_selectable(GTK_LABEL(state->video_label), TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), state->video_label, TRUE, TRUE, 0);

    return vbox;
}

/**
 * Create archive viewer page (stub)
 */
static GtkWidget *create_archive_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    state->archive_tree = gtk_label_new("Archive Viewer\n\nSelect an archive file to view");
    gtk_label_set_selectable(GTK_LABEL(state->archive_tree), TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), state->archive_tree, TRUE, TRUE, 0);

    return vbox;
}

/**
 * Create custom titlebar with window controls
 */
static GtkWidget *create_custom_titlebar(AppState *state)
{
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(header, -1, 30);
    gtk_widget_set_name(header, "file-roller-header");
    
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<b>File Roller</b>");
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(title_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header), title_label, TRUE, TRUE, 0);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(header), button_box, FALSE, FALSE, 5);
    
    GtkWidget *min_btn = gtk_button_new_with_label("—");
    gtk_widget_set_size_request(min_btn, 25, 20);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);
    
    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 25, 20);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), state->window);
    g_signal_connect(state->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(button_box), max_btn, FALSE, FALSE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 25, 20);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), state->window);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);
    
    return header;
}

/**
 * Create toolbar with modern styling
 */
static GtkWidget *create_toolbar(AppState *state)
{
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_widget_set_margin_bottom(toolbar, 5);
    gtk_widget_set_margin_start(toolbar, 10);
    gtk_widget_set_margin_end(toolbar, 10);
    
    GtkWidget *btn_open = gtk_button_new_with_label("📁 Open");
    gtk_widget_set_tooltip_text(btn_open, "Open a file");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_file), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_open, FALSE, FALSE, 0);
    
    GtkWidget *btn_save = gtk_button_new_with_label("💾 Save");
    gtk_widget_set_tooltip_text(btn_save, "Save file");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_file), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save, FALSE, FALSE, 0);
    
    GtkWidget *btn_save_as = gtk_button_new_with_label("📋 Save As");
    gtk_widget_set_tooltip_text(btn_save_as, "Save as new file");
    g_signal_connect(btn_save_as, "clicked", G_CALLBACK(on_save_file_as), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save_as, FALSE, FALSE, 0);
    
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), sep1, FALSE, FALSE, 5);
    
    GtkWidget *btn_zoom_in = gtk_button_new_with_label("🔍+");
    gtk_widget_set_tooltip_text(btn_zoom_in, "Zoom In");
    GtkWidget *btn_zoom_out = gtk_button_new_with_label("🔍-");
    gtk_widget_set_tooltip_text(btn_zoom_out, "Zoom Out");
    GtkWidget *btn_fit = gtk_button_new_with_label("📐 Fit");
    gtk_widget_set_tooltip_text(btn_fit, "Fit to Window");
    
    g_signal_connect(btn_zoom_in, "clicked", G_CALLBACK(on_zoom_in), state);
    g_signal_connect(btn_zoom_out, "clicked", G_CALLBACK(on_zoom_out), state);
    g_signal_connect(btn_fit, "clicked", G_CALLBACK(on_fit_window), state);
    
    gtk_box_pack_start(GTK_BOX(toolbar), btn_zoom_in, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_zoom_out, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_fit, FALSE, FALSE, 0);
    
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), sep2, FALSE, FALSE, 5);
    
    GtkWidget *btn_copy = gtk_button_new_with_label("📋 Copy");
    gtk_widget_set_tooltip_text(btn_copy, "Copy Text");
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_text), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_copy, FALSE, FALSE, 0);
    
    GtkWidget *btn_props = gtk_button_new_with_label("ℹ️ Props");
    gtk_widget_set_tooltip_text(btn_props, "File Properties");
    g_signal_connect(btn_props, "clicked", G_CALLBACK(on_show_properties), state);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_props, FALSE, FALSE, 0);
    
    return toolbar;
}

/**
 * Create main application window
 */
static void create_window(AppState *state)
{
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "File Roller");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 900, 700);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(state->window), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);
    
    gtk_widget_add_events(state->window, GDK_BUTTON_PRESS_MASK |
                                          GDK_BUTTON_RELEASE_MASK |
                                          GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->window, "button-press-event", G_CALLBACK(on_button_press), state);
    g_signal_connect(state->window, "button-release-event", G_CALLBACK(on_button_release), state);
    g_signal_connect(state->window, "motion-notify-event", G_CALLBACK(on_motion_notify), state);
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), main_vbox);
    
    GtkWidget *titlebar = create_custom_titlebar(state);
    gtk_box_pack_start(GTK_BOX(main_vbox), titlebar, FALSE, FALSE, 0);
    
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), sep, FALSE, FALSE, 0);
    
    GtkWidget *toolbar = create_toolbar(state);
    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
    
    state->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(state->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(state->stack), 200);
    
    gtk_stack_add_titled(GTK_STACK(state->stack), create_image_page(state), "image", "Image");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_text_page(state), "text", "Text");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_pdf_page(state), "pdf", "PDF");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_video_page(state), "video", "Video");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_archive_page(state), "archive", "Archive");
    
    gtk_box_pack_start(GTK_BOX(main_vbox), state->stack, TRUE, TRUE, 0);
    
    state->statusbar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(main_vbox), state->statusbar, FALSE, FALSE, 0);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; }"
        "#file-roller-header { background-color: #1a1e24; padding: 2px; }"
        "#file-roller-header button { background-color: #2a323a; color: #00ff88; border: none; min-height: 20px; min-width: 25px; }"
        "#file-roller-header button:hover { background-color: #3a424a; }"
        "#file-roller-header button:active { background-color: #00ff88; color: #0b0f14; }"
        "button { background-color: #1e2429; color: #00ff88; border: 1px solid #00ff88; }"
        "button:hover { background-color: #2a323a; }"
        "label { color: #ffffff; }"
        "textview { background-color: #1a1a1a; color: #ffffff; }"
        "textview text { background-color: #1a1a1a; color: #ffffff; }"
        "statusbar { background-color: #0b0f14; color: #00ff88; }"
        "frame { border-color: #2a323a; }"
        "scrolledwindow { border: none; background-color: #0b0f14; }"
        "scrollbar { background-color: #1e2429; }"
        "scrollbar slider { background-color: #62316b; border-radius: 4px; min-width: 8px; min-height: 8px; }"
        "scrollbar slider:hover { background-color: #7a3b8b; }"
        "scrollbar slider:active { background-color: #9a4bab; }"
        "scrollbar trough { background-color: #2a323a; border-radius: 4px; }",
        -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    g_signal_connect(state->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_key_press), state);
    
    static const GtkTargetEntry target_entries[] = {
        { "text/uri-list", 0, 0 }
    };
    gtk_drag_dest_set(state->window, GTK_DEST_DEFAULT_ALL, 
                      target_entries, G_N_ELEMENTS(target_entries), 
                      GDK_ACTION_COPY);
    
    g_signal_connect(state->window, "drag-data-received", 
                    G_CALLBACK(on_drag_data_received), state);
    
    gtk_widget_show_all(state->window);
}

/**
 * Handle keyboard shortcuts
 */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, AppState *state)
{
    if (event->state & GDK_CONTROL_MASK) {
        switch (event->keyval) {
            case GDK_KEY_plus:
            case GDK_KEY_equal:
                zoom_in(state);
                return TRUE;
            case GDK_KEY_minus:
                zoom_out(state);
                return TRUE;
            case GDK_KEY_0:
                fit_image_to_window(state);
                return TRUE;
            case GDK_KEY_o:
                on_open_file(NULL, state);
                return TRUE;
            case GDK_KEY_s:
                save_file(state);
                return TRUE;
            case GDK_KEY_c:
                on_copy_text(NULL, state);
                return TRUE;
            default:
                break;
        }
    }
    
    if (event->state & GDK_MOD1_MASK) {
        switch (event->keyval) {
            case GDK_KEY_F4:
                gtk_main_quit();
                return TRUE;
            default:
                break;
        }
    }
    
    return FALSE;
}

/* ========== MAIN APPLICATION ========== */

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    
    AppState state = {0};
    state.zoom_level = 1.0;
    state.current_folder = g_strdup(g_get_home_dir());
    state.text_modified = FALSE;
    
    create_window(&state);
    
    if (argc > 1) {
        open_file(&state, argv[1]);
    }
    
    gtk_main();
    
    if (state.current_filename) g_free(state.current_filename);
    if (state.current_folder) g_free(state.current_folder);
    if (state.current_pixbuf) g_object_unref(state.current_pixbuf);
    if (state.original_pixbuf) g_object_unref(state.original_pixbuf);
    
    return 0;
}