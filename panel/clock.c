#include "panel.h"

gboolean update_clock(gpointer data) {
    GtkLabel *label = GTK_LABEL(data);
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[64];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    gtk_label_set_text(label, buffer);
    return G_SOURCE_CONTINUE;
}