#include "fm.h"
#include "../image-viewer/image-viewer.h"
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper functions for formatting
static gchar *format_size(goffset size) 

{
    if (size < 1024) return g_strdup_printf("%ld B", (long)size);
    if (size < 1024 * 1024) return g_strdup_printf("%.1f KB", size / 1024.0);
    if (size < 1024 * 1024 * 1024) return g_strdup_printf("%.1f MB", size / (1024.0 * 1024.0));
    return g_strdup_printf("%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
}

static gchar *format_time(GDateTime *dt) 

{
    return g_date_time_format(dt, "%Y-%m-%d %H:%M");
}

// Check if file is an image based on extension
static gboolean is_image_file(const gchar *filename) 

{
    const gchar *ext = strrchr(filename, '.');
    if (!ext) return FALSE;
    
    // Common image file extensions
    const gchar *image_extensions[] = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp",
        ".tiff", ".tif", ".ico", ".svg", ".svgz",
        ".psd", ".xcf", ".jp2", ".j2k", ".jpf", ".jpx",
        ".jpm", ".mj2", ".heif", ".heic", ".raw",
        ".dng", ".cr2", ".crw", ".nef", ".nrw",
        ".raf", ".rw2", ".orf", ".dcs", ".dcr",
        ".arw", ".mos", ".ptx", ".pef", ".exif",
        ".jfif", ".ppm", ".pgm", ".pbm", ".pnm",
        ".qoi", ".pcx", ".cur", ".ani", NULL
    };
    
    for (int i = 0; image_extensions[i] != NULL; i++) {
        if (g_str_has_suffix(filename, image_extensions[i])) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// Check if file is a text file based on extension
static gboolean is_text_file(const gchar *filename) 

{
    const gchar *ext = strrchr(filename, '.');
    if (!ext) return FALSE;
    
    // Common text file extensions
    const gchar *text_extensions[] = {
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
        ".proto", ".thrift", ".capnp", ".fbs", NULL
    };
    
    for (int i = 0; text_extensions[i] != NULL; i++) {
        if (g_str_has_suffix(filename, text_extensions[i])) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// Open file with appropriate application - FIXED to use BlackLine editor
static void open_file_with_app(GtkWindow *parent, const gchar *path) 

{
    (void)parent; // Unused parameter
    
    // Check if it's an image file
    if (is_image_file(path)) {
        // Use image viewer for image files
        launch_image_viewer(path);
    } else if (is_text_file(path)) {
        // Use BlackLine editor for text files - launch in background
        gchar *command = g_strdup_printf("blackline-editor \"%s\" &", path);
        system(command);
        g_free(command);
    } else {
        // For other files, use xdg-open
        gchar *command = g_strdup_printf("xdg-open \"%s\" &", path);
        system(command);
        g_free(command);
    }
}

// This function is called when a file is double-clicked or opened from the context menu
void browser_open_file(FileManager *fm, const gchar *path)

{
    if (!path || !fm) return;
    
    GFile *file = g_file_new_for_path(path);
    GFileType type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
    
    if (type == G_FILE_TYPE_DIRECTORY) {
        // If it's a directory, navigate to it using fm_open_location from fm.c
        fm_open_location(fm, path);
    } else {
        // If it's a file, open it with the appropriate application
        open_file_with_app(GTK_WINDOW(fm->window), path);
    }
    
    g_object_unref(file);
}

// Browser-specific initialization
void browser_init(FileManager *fm)

{
    // Any browser-specific setup can go here
    (void)fm;
}

// Optional: If you need to connect any browser-specific signals
void browser_connect_signals(FileManager *fm)

{
    (void)fm;
    // To add any browser-specific signal connections here
    // The main navigation signals are already connected in fm.c
}