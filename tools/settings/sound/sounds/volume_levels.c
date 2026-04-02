#include "sound.h"
#include <gtk/gtk.h>


/*
 * volume_levels.c
 * 
 * Volume level presets and standardized values
Defines standard volume levels for consistent audio experience.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

GtkWidget *volume_levels_widget_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>System Sound Volume</b>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // This could be a volume slider for system sound events, but often they follow master volume.
    // We'll just put a placeholder.
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_widget_set_sensitive(scale, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 5);

    GtkWidget *note = gtk_label_new("System sounds follow master volume.");
    gtk_box_pack_start(GTK_BOX(vbox), note, FALSE, FALSE, 0);

    return vbox;
}