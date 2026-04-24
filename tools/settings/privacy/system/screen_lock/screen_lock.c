#include "screen_lock.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * screen_lock.c
 *
 * Provides a toggle to enable automatic screen locking and a spin button
 * to set idle timeout in minutes. Uses xdg-screensaver to monitor idle time.
 */

static GtkWidget *status_label = NULL;
static guint idle_check_source = 0;
static gboolean lock_enabled = FALSE;
static int lock_timeout_min = 5;   // default 5 minutes

static void lock_screen(void)
{
    /* Use multiple lock methods */
    system("xdg-screensaver lock 2>/dev/null || "
           "gnome-screensaver-command -l 2>/dev/null || "
           "loginctl lock-session 2>/dev/null || "
           "dm-tool lock 2>/dev/null");
}

/* Check idle time and lock if needed */
static gboolean check_idle_and_lock(gpointer data)
{
    (void)data;
    if (!lock_enabled) return TRUE;
    
    /* Use xprintidle if available, else fallback */
    FILE *fp = popen("xprintidle 2>/dev/null", "r");
    if (fp) {
        int idle_ms = 0;
        if (fscanf(fp, "%d", &idle_ms) == 1) {
            int idle_min = idle_ms / 60000;
            if (idle_min >= lock_timeout_min) {
                lock_screen();
            }
        }
        pclose(fp);
    } else {
        /* Fallback: use xssstate -i (from suckless-tools) */
        fp = popen("xssstate -i 2>/dev/null", "r");
        if (fp) {
            int idle_sec = 0;
            if (fscanf(fp, "%d", &idle_sec) == 1) {
                if (idle_sec >= lock_timeout_min * 60) {
                    lock_screen();
                }
            }
            pclose(fp);
        }
    }
    return TRUE;
}

static void start_idle_monitor(void)
{
    if (idle_check_source) return;
    idle_check_source = g_timeout_add_seconds(10, check_idle_and_lock, NULL);
}

static void stop_idle_monitor(void)
{
    if (idle_check_source) {
        g_source_remove(idle_check_source);
        idle_check_source = 0;
    }
}

static void on_lock_toggled(GtkToggleButton *btn, gpointer data)
{
    (void)data;
    lock_enabled = gtk_toggle_button_get_active(btn);
    if (lock_enabled) {
        start_idle_monitor();
        if (status_label) {
            gchar *msg = g_strdup_printf("Screen lock enabled (timeout: %d min)", lock_timeout_min);
            gtk_label_set_text(GTK_LABEL(status_label), msg);
            g_free(msg);
        }
    } else {
        stop_idle_monitor();
        if (status_label)
            gtk_label_set_text(GTK_LABEL(status_label), "Screen lock disabled");
    }
}

static void on_timeout_changed(GtkSpinButton *spin, gpointer data)
{
    (void)data;
    lock_timeout_min = gtk_spin_button_get_value_as_int(spin);
    if (lock_enabled) {
        if (status_label) {
            gchar *msg = g_strdup_printf("Screen lock enabled (timeout: %d min)", lock_timeout_min);
            gtk_label_set_text(GTK_LABEL(status_label), msg);
            g_free(msg);
        }
    }
}

/* Load saved settings */
static void load_saved(void)
{
    const char *home = g_get_home_dir();
    gchar *config = g_build_filename(home, ".config/blackline", "screenlock.conf", NULL);
    FILE *f = fopen(config, "r");
    if (f) {
        char line[64];
        if (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%d %d", &lock_timeout_min, &lock_enabled) != 2) {
                lock_enabled = 0;
                lock_timeout_min = 5;
            }
        }
        fclose(f);
    }
    g_free(config);
}

static void save_settings(void)
{
    const char *home = g_get_home_dir();
    gchar *config = g_build_filename(home, ".config/blackline", "screenlock.conf", NULL);
    FILE *f = fopen(config, "w");
    if (f) {
        fprintf(f, "%d %d", lock_timeout_min, lock_enabled ? 1 : 0);
        fclose(f);
    }
    g_free(config);
}

/* Called when any setting changes */
static void settings_changed(void)
{
    save_settings();
    if (lock_enabled)
        start_idle_monitor();
}

GtkWidget *screen_lock_widget_new(void)
{
    load_saved();
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    
    /* Enable lock toggle */
    GtkWidget *toggle_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *toggle_label = gtk_label_new("Enable automatic screen lock");
    GtkWidget *toggle = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(toggle), lock_enabled);
    g_signal_connect(toggle, "notify::active", G_CALLBACK(on_lock_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(toggle_box), toggle_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toggle_box), toggle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toggle_box, FALSE, FALSE, 0);
    
    /* Timeout spin button */
    GtkWidget *timeout_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *timeout_label = gtk_label_new("Lock after (minutes):");
    GtkWidget *spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), lock_timeout_min);
    g_signal_connect(spin, "value-changed", G_CALLBACK(on_timeout_changed), NULL);
    gtk_box_pack_start(GTK_BOX(timeout_box), timeout_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeout_box), spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), timeout_box, FALSE, FALSE, 5);
    
    status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
    
    /* Spacer */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), TRUE, TRUE, 0);
    
    /* Initialize monitor if enabled */
    if (lock_enabled) start_idle_monitor();
    
    /* Connect saved settings on exit (not needed as we save immediately) */
    /* But we can also connect to destroy signal to save final state */
    
    return vbox;
}
