#include "p_settings.h"
#include "batary.h"
#include "mode.h"

/*
 * power_settings.c
 * 
 * Power settings tab combining battery info and power mode selection.
 */

GtkWidget *power_settings_tab_new(void) 

{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    
    /* Battery status section */
    GtkWidget *battery_frame = battery_widget_new();
    gtk_box_pack_start(GTK_BOX(vbox), battery_frame, FALSE, FALSE, 0);
    
    /* Power mode section */
    GtkWidget *mode_frame = mode_selector_widget_new();
    gtk_box_pack_start(GTK_BOX(vbox), mode_frame, FALSE, FALSE, 0);
    
    /* Spacer to push content up */
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), spacer, TRUE, TRUE, 0);
    
    return vbox;
}