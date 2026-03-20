#include "displaySettings.h"
#include "orientation.h"
#include "refresh_rate.h"
#include "resolution.h"

GtkWidget *display_settings_tab_new(void)

{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    // Orientation section
    GtkWidget *orient_frame = gtk_frame_new("Orientation");
    GtkWidget *orient_box = orientation_widget_new();
    gtk_container_add(GTK_CONTAINER(orient_frame), orient_box);
    gtk_box_pack_start(GTK_BOX(vbox), orient_frame, FALSE, FALSE, 0);

    // Refresh rate section
    GtkWidget *refresh_frame = gtk_frame_new("Refresh Rate");
    GtkWidget *refresh_box = refresh_rate_widget_new();
    gtk_container_add(GTK_CONTAINER(refresh_frame), refresh_box);
    gtk_box_pack_start(GTK_BOX(vbox), refresh_frame, FALSE, FALSE, 0);

    // Resolution section
    GtkWidget *res_frame = gtk_frame_new("Resolution");
    GtkWidget *res_box = resolution_widget_new();
    gtk_container_add(GTK_CONTAINER(res_frame), res_box);
    gtk_box_pack_start(GTK_BOX(vbox), res_frame, FALSE, FALSE, 0);

    return vbox;
}