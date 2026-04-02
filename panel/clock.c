#include "panel.h"

/*
 * clock.c
 * 
 * System clock widget with continuous time display. GLib timeout-driven
 * updates reading system time formatted to GtkLabel.
 */

/**
 * Timer callback that updates a label with the current system time.
 * Formats the time as HH:MM:SS using local time.
 *
 * @param data Pointer to a GtkLabel widget to update
 * @return     G_SOURCE_CONTINUE to keep the timer running
 *
 * @sideeffect Updates the text of the provided label widget.
 *             Called repeatedly from a GLib timeout source.
 * @threadsafe Assumes label is owned by the main thread.
 */
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