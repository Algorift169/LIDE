#include "voidfox.h"

/**
 * voidfox.c
 * 
 * Web browser main implementation using WebKit2.
 * Provides multi-tab browsing, history, bookmarks, downloads,
 * and security features (passwords, certificates).
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**
 * Application activation callback.
 * Wrapper that forwards to voidfox_activate function.
 *
 * @param app        The GtkApplication instance.
 * @param user_data  User data passed during signal connection.
 */
static void activate(GtkApplication *app, gpointer user_data) {
    voidfox_activate(app, user_data);
}

/**
 * Application entry point.
 * Sets up environment variables for WebKit rendering and launches the browser.
 *
 * @param argc Argument count from command line.
 * @param argv Argument vector from command line.
 * @return     Exit status from g_application_run().
 *
 * @sideeffect Sets WEBKIT_DISABLE_COMPOSITING_MODE and LIBGL_ALWAYS_SOFTWARE
 *             environment variables to work around DRI3/EGL issues.
 */
int main(int argc, char **argv) {
    /* Set environment variables to help with DRI3/EGL issues */
    setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 1);
    setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1); /* Force software rendering */
    
    GtkApplication *app = gtk_application_new("org.lide.voidfox", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}