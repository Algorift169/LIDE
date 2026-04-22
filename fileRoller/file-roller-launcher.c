#include "file-roller.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

/**
 * file-roller-launcher.c
 *
 * File roller application launcher.
 * Handles command-line file arguments and spawns the main application.
 *
 * This module is part of the LIDE desktop environment system.
 */

/**
 * Launches the BlackLine file roller as a separate process.
 * Uses g_spawn_async to safely pass filename arguments without shell interpretation.
 *
 * @param filename Path to the file to open.
 *                 If NULL, function returns without launching.
 *
 * @note Forks a child process and executes blackline-file-roller.
 * @note Any spawn errors are silently ignored (error freed internally).
 */
void launch_file_roller(const char *filename)
{
    if (!filename) return;

    /* Use g_spawn_async to properly pass arguments without shell interpretation issues */
    gchar *argv[] = {
        (gchar *)"blackline-file-roller",
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
