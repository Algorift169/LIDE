#include "image-viewer.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

/**
 * image-viewer-launcher.c
 * 
 * Image viewer application launcher and file association.
 * Handles command-line image file arguments and desktop integration.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**
 * Launches the BlackLine image viewer as a separate process.
 * Uses g_spawn_async to safely pass filename arguments without shell interpretation.
 *
 * @param filename Path to the image file to open.
 *                 If NULL, function returns without launching.
 *
 * @note Forks a child process and executes blackline-image-viewer.
 * @note Any spawn errors are silently ignored (error freed internally).
 */
void launch_image_viewer(const char *filename)
{
    if (!filename) return;
    
    /* Use g_spawn_async to properly pass arguments without shell interpretation issues */
    gchar *argv[] = {
        (gchar *)"blackline-image-viewer",
        (gchar *)filename,
        NULL
    };
    
    GError *error = NULL;
    g_spawn_async(
        NULL,                           /* working directory - use current */
        argv,                           /* argument vector */
        NULL,                           /* environment - inherit from parent */
        G_SPAWN_SEARCH_PATH,            /* flags - search PATH for executable */
        NULL,                           /* child setup function */
        NULL,                           /* user data */
        NULL,                           /* child PID output - not needed */
        &error                          /* error output */
    );
    
    if (error) {
        g_error_free(error);
    }
}