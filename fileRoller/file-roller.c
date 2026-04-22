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

/**
 * file-roller.c
 *
 * Universal file viewer for LIDE desktop environment.
 * Supports images, text, PDFs, videos, archives, and more.
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
    double rotation;                /* Rotation in degrees */

    /* Text viewer widgets */
    GtkWidget *text_view;           /* Text display area */
    GtkWidget *text_scroll;         /* Scroll container for text */

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

} AppState;

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
    state->rotation = 0.0;

    /* Scale for display */
    GdkPixbuf *scaled = scale_pixbuf(pixbuf, 600, 400);
    state->current_pixbuf = scaled;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, scaled);

    char status[512];
    snprintf(status, sizeof(status), "%s - %dx%d pixels | Zoom: 100%% | Rotation: 0°",
             g_path_get_basename(filename),
             gdk_pixbuf_get_width(pixbuf),
             gdk_pixbuf_get_height(pixbuf));

    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "image_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
}

/**
 * Rotate image 90 degrees clockwise
 */
static void rotate_image_cw(AppState *state)
{
    if (!state->original_pixbuf) return;

    state->rotation += 90.0;
    if (state->rotation >= 360.0) state->rotation = 0.0;

    GdkPixbuf *rotated = gdk_pixbuf_rotate_simple(state->original_pixbuf,
                                                  (GdkPixbufRotation)(state->rotation / 90));
    if (!rotated) return;

    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
    state->current_pixbuf = rotated;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, rotated);
}

/**
 * Rotate image 90 degrees counter-clockwise
 */
static void rotate_image_ccw(AppState *state)
{
    if (!state->original_pixbuf) return;

    state->rotation -= 90.0;
    if (state->rotation < 0.0) state->rotation = 270.0;

    GdkPixbuf *rotated = gdk_pixbuf_rotate_simple(state->original_pixbuf,
                                                  (GdkPixbufRotation)(state->rotation / 90));
    if (!rotated) return;

    if (state->current_pixbuf) g_object_unref(state->current_pixbuf);
    state->current_pixbuf = rotated;

    GtkImage *img = GTK_IMAGE(state->image_viewer);
    gtk_image_set_from_pixbuf(img, rotated);
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

/* ========== TEXT VIEWER FUNCTIONS ========== */

/**
 * Load and display text file
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

    if (length > 1000000) {
        /* File is too large, show warning */
        gchar warning[512];
        snprintf(warning, sizeof(warning),
                 "File is very large (%ld bytes)\nShowing first 100KB only",
                 length);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                 warning, -1);
        g_free(contents);
    } else {
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                 contents, -1);
        g_free(contents);
    }

    char status[512];
    snprintf(status, sizeof(status), "Text File: %s | Size: %ld bytes",
             g_path_get_basename(filename), length);
    guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(state->statusbar), "file_info");
    gtk_statusbar_pop(GTK_STATUSBAR(state->statusbar), context_id);
    gtk_statusbar_push(GTK_STATUSBAR(state->statusbar), context_id, status);
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

/* ========== GENERIC FILE FUNCTIONS ========== */

/**
 * Open any type of file
 */
static void open_file(AppState *state, const char *filename)
{
    if (!filename) return;

    /* Update state */
    if (state->current_filename) g_free(state->current_filename);
    state->current_filename = g_strdup(filename);

    state->current_file_type = get_file_type(filename);
    state->is_modified = FALSE;

    /* Update window title */
    gchar title[512];
    snprintf(title, sizeof(title), "File Roller - %s (%s)",
             g_path_get_basename(filename),
             get_file_type_name(state->current_file_type));
    gtk_window_set_title(GTK_WINDOW(state->window), title);

    /* Load file based on type */
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
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view)),
                                      "File type not recognized.\n\nThis file cannot be displayed in File Roller.",
                                      -1);
            break;
    }
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

static void on_rotate_cw(GtkWidget *widget, AppState *state)
{
    if (state->current_file_type == FILE_TYPE_IMAGE) {
        rotate_image_cw(state);
    }
}

static void on_rotate_ccw(GtkWidget *widget, AppState *state)
{
    if (state->current_file_type == FILE_TYPE_IMAGE) {
        rotate_image_ccw(state);
    }
}

static void on_open_file(GtkWidget *widget, AppState *state)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open File",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (state->current_folder) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state->current_folder);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            if (state->current_folder) g_free(state->current_folder);
            state->current_folder = g_path_get_dirname(filename);
            open_file(state, filename);
            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
}

/* ========== WINDOW SETUP ========== */

/**
 * Create image viewer page
 */
static GtkWidget *create_image_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Toolbar for image controls */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_widget_set_margin_bottom(toolbar, 5);
    gtk_widget_set_margin_start(toolbar, 5);
    gtk_widget_set_margin_end(toolbar, 5);

    GtkWidget *btn_zoom_in = gtk_button_new_with_label("Zoom In");
    GtkWidget *btn_zoom_out = gtk_button_new_with_label("Zoom Out");
    GtkWidget *btn_fit = gtk_button_new_with_label("Fit Window");
    GtkWidget *btn_rotate_cw = gtk_button_new_with_label("Rotate →");
    GtkWidget *btn_rotate_ccw = gtk_button_new_with_label("Rotate ←");

    g_signal_connect(btn_zoom_in, "clicked", G_CALLBACK(on_zoom_in), state);
    g_signal_connect(btn_zoom_out, "clicked", G_CALLBACK(on_zoom_out), state);
    g_signal_connect(btn_fit, "clicked", G_CALLBACK(on_fit_window), state);
    g_signal_connect(btn_rotate_cw, "clicked", G_CALLBACK(on_rotate_cw), state);
    g_signal_connect(btn_rotate_ccw, "clicked", G_CALLBACK(on_rotate_ccw), state);

    gtk_box_pack_start(GTK_BOX(toolbar), btn_zoom_in, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_zoom_out, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_fit, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_rotate_cw, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_rotate_ccw, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* Scrollable image area */
    state->image_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(state->image_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    state->image_viewer = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(state->image_scroll), state->image_viewer);

    gtk_box_pack_start(GTK_BOX(vbox), state->image_scroll, TRUE, TRUE, 0);

    return vbox;
}

/**
 * Create text viewer page
 */
static GtkWidget *create_text_page(AppState *state)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    state->text_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(state->text_scroll),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    state->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(state->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->text_view), GTK_WRAP_WORD);

    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
    gtk_widget_override_font(state->text_view, font_desc);
    pango_font_description_free(font_desc);

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
 * Create main application window
 */
static void create_window(AppState *state)
{
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "File Roller");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);

    /* Main vbox */
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), main_vbox);

    /* Top toolbar */
    GtkWidget *top_toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_top(top_toolbar, 5);
    gtk_widget_set_margin_bottom(top_toolbar, 5);
    gtk_widget_set_margin_start(top_toolbar, 5);
    gtk_widget_set_margin_end(top_toolbar, 5);

    GtkWidget *btn_open = gtk_button_new_with_label("Open File");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_file), state);

    gtk_box_pack_start(GTK_BOX(top_toolbar), btn_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), top_toolbar, FALSE, FALSE, 0);

    /* Stack for different views */
    state->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(state->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(state->stack), 200);

    gtk_stack_add_titled(GTK_STACK(state->stack), create_image_page(state), "image", "Image");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_text_page(state), "text", "Text");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_pdf_page(state), "pdf", "PDF");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_video_page(state), "video", "Video");
    gtk_stack_add_titled(GTK_STACK(state->stack), create_archive_page(state), "archive", "Archive");

    gtk_box_pack_start(GTK_BOX(main_vbox), state->stack, TRUE, TRUE, 0);

    /* Status bar */
    state->statusbar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(main_vbox), state->statusbar, FALSE, FALSE, 0);

    /* Signals */
    g_signal_connect(state->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(state->window);
}

/* ========== MAIN APPLICATION ========== */

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    AppState state = {0};
    state.zoom_level = 1.0;
    state.rotation = 0.0;
    state.current_folder = g_strdup(g_get_home_dir());

    create_window(&state);

    /* Open file if provided as argument */
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
