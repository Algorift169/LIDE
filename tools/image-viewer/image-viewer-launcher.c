#include "image-viewer.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

// Function to launch the image viewer with an optional filename
void launch_image_viewer(const char *filename)
{
    if (!filename) return;
    
    // Use g_spawn_async to properly pass arguments without shell interpretation issues
    gchar *argv[] = {
        (gchar *)"blackline-image-viewer",
        (gchar *)filename,
        NULL
    };
    
    GError *error = NULL;
    g_spawn_async(
        NULL,                           // working directory
        argv,                           // arguments
        NULL,                           // environment
        G_SPAWN_SEARCH_PATH,            // flags - search PATH for executable
        NULL,                           // child setup function
        NULL,                           // user data
        NULL,                           // child PID output
        &error                          // error
    );
    
    if (error) {
        g_error_free(error);
    }
}
