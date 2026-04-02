#include "displaySettings.h"
#include "orientation.h"
#include "refresh_rate.h"
#include "resolution.h"
#include "wallpaper_settings.h"


/*
 * display_settings.c
 * 
 * Display configuration UI (resolution, refresh rate, orientation)
Provides interface for monitor configuration via xrandr.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**

 * Creates the display settings tab widget.
 * Provides controls for orientation, refresh rate, and resolution settings.
 *
 * @return GtkWidget containing the complete display settings UI.
 *         The widget is a vertically packed GtkBox containing three framed sections.
 *
 * @sideeffect Initializes all display configuration widgets.
 * @note Caller is responsible for managing the returned widget's lifecycle.
 */
GtkWidget *display_settings_tab_new(void)

{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Orientation section - screen rotation settings */
    GtkWidget *orient_frame = gtk_frame_new("Orientation");
    GtkWidget *orient_box = orientation_widget_new();
    gtk_container_add(GTK_CONTAINER(orient_frame), orient_box);
    gtk_box_pack_start(GTK_BOX(vbox), orient_frame, FALSE, FALSE, 0);

    /* Refresh rate section - display frequency settings */
    GtkWidget *refresh_frame = gtk_frame_new("Refresh Rate");
    GtkWidget *refresh_box = refresh_rate_widget_new();
    gtk_container_add(GTK_CONTAINER(refresh_frame), refresh_box);
    gtk_box_pack_start(GTK_BOX(vbox), refresh_frame, FALSE, FALSE, 0);

    /* Resolution section - screen size and scaling settings */
    GtkWidget *res_frame = gtk_frame_new("Resolution");
    GtkWidget *res_box = resolution_widget_new();
    gtk_container_add(GTK_CONTAINER(res_frame), res_box);
    gtk_box_pack_start(GTK_BOX(vbox), res_frame, FALSE, FALSE, 0);

    /* Wallpaper section - allows user to select and apply wallpapers from ./images/wallpapers/ */
    // See wallpaper_settings_widget_new() for implementation details.
    GtkWidget *wallpaper_frame = gtk_frame_new("Wallpaper");
    GtkWidget *wallpaper_box = wallpaper_settings_widget_new();
    gtk_container_add(GTK_CONTAINER(wallpaper_frame), wallpaper_box);
    gtk_box_pack_start(GTK_BOX(vbox), wallpaper_frame, FALSE, FALSE, 0);

    return vbox;
}