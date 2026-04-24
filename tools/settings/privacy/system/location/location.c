#include "location.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * location.c
 *
 * Provides a toggle to enable/disable location services.
 * On real systems, it toggles the geoclue service (if installed)
 * or uses a mock mechanism with a config file.
 */

static GtkWidget *status_label = NULL;

static gboolean is_location_enabled(void)
{
    /* Check if geoclue is running */
    int ret = system("pgrep -x geoclue >/dev/null 2>&1");
    if (ret == 0) return TRUE;
    
    /* Fallback: check our own flag file */
    const char *home = g_get_home_dir();
    gchar *flag_file = g_build_filename(home, ".config/blackline/location_disabled", NULL);
    gboolean disabled = g_file_test(flag_file, G_FILE_TEST_EXISTS);
    g_free(flag_file);
    return !disabled;
}

static void set_location_enabled(gboolean enabled)
{
    if (enabled) {
        /* Try to start geoclue service */
        system("systemctl --user start geoclue 2>/dev/null || "
               "sudo systemctl start geoclue 2>/dev/null");
        /* Remove our own disable flag */
        const char *home = g_get_home_dir();
        gchar *flag_file = g_build_filename(home, ".config/blackline/location_disabled", NULL);
        unlink(flag_file);
        g_free(flag_file);
        if (status_label)
            gtk_label_set_text(GTK_LABEL(status_label), "Location services enabled");
    } else {
        /* Stop geoclue service and mark disabled */
        system("systemctl --user stop geoclue 2>/dev/null || "
               "sudo systemctl stop geoclue 2>/dev/null");
        const char *home = g_get_home_dir();
        gchar *flag_file = g_build_filename(home, ".config/blackline/location_disabled", NULL);
        FILE *f = fopen(flag_file, "w");
        if (f) fclose(f);
        g_free(flag_file);
        if (status_label)
            gtk_label_set_text(GTK_LABEL(status_label), "Location services disabled");
    }
}

static void on_location_toggled(GtkToggleButton *btn, gpointer data)
{
    (void)data;
    gboolean active = gtk_toggle_button_get_active(btn);
    set_location_enabled(active);
}

GtkWidget *location_settings_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Location Services</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);

    GtkWidget *switch_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *label = gtk_label_new("Allow applications to access your location");
    GtkWidget *toggle = gtk_switch_new();
    gboolean enabled = is_location_enabled();
    gtk_switch_set_active(GTK_SWITCH(toggle), enabled);
    g_signal_connect(toggle, "notify::active", G_CALLBACK(on_location_toggled), NULL);
    
    gtk_box_pack_start(GTK_BOX(switch_box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(switch_box), toggle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), switch_box, FALSE, FALSE, 5);
    
    status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
    
    GtkWidget *info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info), "<small>Requires geoclue service to be installed.</small>");
    gtk_label_set_xalign(GTK_LABEL(info), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), info, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), TRUE, TRUE, 0);
    
    return vbox;
}
