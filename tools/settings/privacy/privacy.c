#include "privacy.h"
#include "system/system.h"
#include "devices/device_privacy.h"

/*
 * privacy.c
 *
 * Parent container for privacy settings.
 * Holds a notebook with two tabs: System (system settings) and Devices.
 */

GtkWidget *privacy_settings_tab_new(void)
{
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 5);

    /* System tab (centralised system privacy features) */
    GtkWidget *system_tab = system_settings_widget_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), system_tab,
                             gtk_label_new("System"));

    /* Devices tab (camera, etc.) */
    GtkWidget *devices_tab = device_privacy_widget_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), devices_tab,
                             gtk_label_new("Devices"));

    return notebook;
}
