#include "voidfox.h"

static void activate(GtkApplication *app, gpointer user_data) {
    voidfox_activate(app, user_data);
}

int main(int argc, char **argv) {
    // Set environment variables to help with DRI3/EGL issues
    setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", 1);
    setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1); // Force software rendering
    
    GtkApplication *app = gtk_application_new("org.lide.voidfox", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}