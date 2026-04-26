#include "fm.h"
#include "../image-viewer/image-viewer.h"
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Forward declaration for file roller launcher.
 * Launches the custom BlackLine file roller for handling various file types.
 *
 * @param filename Path to the file to open with the file roller.
 */
extern void launch_file_roller(const char *filename);

/**
 * Stub for fm_open_location when fm.c is not linked.
 * Used by the window manager's integrated browser.
 */
__attribute__((weak))
void fm_open_location(FileManager *fm, const gchar *path)
{
    /* Stub: This is overridden by fm.c when the file manager is fully linked */
    (void)fm;
    (void)path;
}

/* Helper functions for formatting */

/**
 * browser.c
 * 
 * Browser rendering and navigation engine.
 * Core WebKit2 integration for page loading, DOM access, and
 * JavaScript execution.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**
 * Formats file size to human-readable string.
 *
 * @param size Size in bytes.
 * @return Newly allocated string with formatted size (e.g., "1.5 MB").
 *         Caller must free with g_free().
 */
static gchar *format_size(goffset size) 
{
    if (size < 1024) return g_strdup_printf("%ld B", (long)size);
    if (size < 1024 * 1024) return g_strdup_printf("%.1f KB", size / 1024.0);
    if (size < 1024 * 1024 * 1024) return g_strdup_printf("%.1f MB", size / (1024.0 * 1024.0));
    return g_strdup_printf("%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
}

/**
 * Formats GDateTime to standard date-time string.
 *
 * @param dt GDateTime object.
 * @return Newly allocated string formatted as "YYYY-MM-DD HH:MM".
 *         Caller must free with g_free().
 */
static gchar *format_time(GDateTime *dt) 
{
    return g_date_time_format(dt, "%Y-%m-%d %H:%M");
}

/**
 * Checks if a file is supported by the file roller.
 * Supports images, text, PDFs, videos, audio, and archive formats.
 *
 * @param filename File name to check.
 * @return TRUE if file is supported by file roller, FALSE otherwise.
 */
static gboolean is_file_roller_supported(const gchar *filename) 
{
    const gchar *ext = strrchr(filename, '.');
    if (!ext) return FALSE;
    
    /* Supported file extensions for the file roller */
    const gchar *supported_extensions[] = {
        /* Images */
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp",
        ".tiff", ".tif", ".ico", ".svg", ".svgz",
        ".psd", ".xcf", ".jp2", ".j2k", ".jpf", ".jpx",
        ".jpm", ".mj2", ".heif", ".heic", ".raw",
        ".dng", ".cr2", ".crw", ".nef", ".nrw",
        ".raf", ".rw2", ".orf", ".dcs", ".dcr",
        ".arw", ".mos", ".ptx", ".pef", ".exif",
        ".jfif", ".ppm", ".pgm", ".pbm", ".pnm",
        ".qoi", ".pcx", ".cur", ".ani",
        /* Text */
        ".txt", ".c", ".h", ".cpp", ".hpp", ".cc", ".cxx",
        ".py", ".pl", ".rb", ".sh", ".bash", ".zsh",
        ".js", ".html", ".htm", ".css", ".xml", ".json",
        ".md", ".markdown", ".ini", ".conf", ".cfg",
        ".log", ".csv", ".tsv", ".sql", ".java", ".kt",
        ".go", ".rs", ".swift", ".m", ".mm", ".php",
        ".asp", ".aspx", ".jsp", ".tcl", ".lua", ".r",
        ".yaml", ".yml", ".toml", ".makefile", ".mk",
        ".cmake", ".diff", ".patch", ".tex", ".bib",
        ".rst", ".asciidoc", ".pod", ".1", ".2", ".3",
        ".man", ".nfo", ".diz", ".gitignore", ".gitattributes",
        ".editorconfig", ".vim", ".el", ".scm", ".ss",
        ".clj", ".cljs", ".coffee", ".litcoffee", ".hs",
        ".lhs", ".erl", ".hrl", ".ex", ".exs", ".eex",
        ".leex", ".slim", ".haml", ".pug", ".jade",
        ".scss", ".sass", ".less", ".styl", ".vue",
        ".jsx", ".tsx", ".dart", ".groovy", ".gradle",
        ".properties", ".plist", ".xib", ".storyboard",
        ".strings", ".po", ".mo", ".json5", ".jsonc",
        ".proto", ".thrift", ".capnp", ".fbs",
        /* PDF */
        ".pdf",
        /* Video */
        ".mp4", ".webm", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".m4v",
        /* Audio */
        ".mp3", ".wav", ".flac", ".aac", ".ogg", ".m4a", ".wma",
        /* Archives */
        ".zip", ".tar", ".gz", ".7z", ".rar", ".xz", ".bz2",
        NULL
    };
    
    for (int i = 0; supported_extensions[i] != NULL; i++) {
        if (g_str_has_suffix(filename, supported_extensions[i])) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * Opens a file with the appropriate application.
 * Routes all supported file types through the custom BlackLine file roller,
 * which intelligently handles images, text, PDFs, videos, audio, and archives.
 * Falls back to xdg-open for unsupported formats.
 *
 * @param parent Parent window for dialogs (unused).
 * @param path   Full path to the file to open.
 *
 * @sideeffect Launches external process in background.
 */
static void open_file_with_app(GtkWindow *parent, const gchar *path) 
{
    (void)parent; /* Unused parameter */
    
    /* Check if the file is supported by the custom file roller */
    if (is_file_roller_supported(path)) {
        /* Use the custom BlackLine file roller for all supported file types */
        launch_file_roller(path);
    } else {
        /* For unsupported file types, fall back to xdg-open */
        gchar *argv[] = {
            (gchar *)"xdg-open",
            (gchar *)path,
            NULL
        };
        
        GError *error = NULL;
        g_spawn_async(
            NULL,
            argv,
            NULL,
            G_SPAWN_SEARCH_PATH,
            NULL,
            NULL,
            NULL,
            &error
        );
        
        if (error) {
            g_error_free(error);
        }
    }
}

/**
 * Opens a file or directory in the file manager.
 * Directories trigger navigation; files open with appropriate application.
 *
 * @param fm   FileManager instance.
 * @param path Path to the file or directory.
 *
 * @sideeffect Navigates to directory or launches external application.
 */
void browser_open_file(FileManager *fm, const gchar *path)

{
    if (!path || !fm) return;
    
    GFile *file = g_file_new_for_path(path);
    GFileType type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
    
    if (type == G_FILE_TYPE_DIRECTORY) {
        /* If it's a directory, navigate to it */
        fm_open_location(fm, path);
    } else {
        /* If it's a file, open it with the appropriate application */
        open_file_with_app(GTK_WINDOW(fm->window), path);
    }
    
    g_object_unref(file);
}

/**
 * Initializes browser-specific components.
 *
 * @param fm FileManager instance.
 */
void browser_init(FileManager *fm)

{
    /* Any browser-specific setup can go here */
    (void)fm;
}

/**
 * Connects browser-specific signals to the file manager.
 * Currently a placeholder for future extensions.
 *
 * @param fm FileManager instance.
 */
void browser_connect_signals(FileManager *fm)

{
    (void)fm;
    /* To add any browser-specific signal connections here */
    /* The main navigation signals are already connected in fm.c */
}