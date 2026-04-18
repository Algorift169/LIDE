#include "batary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/*
 * batary.c
 * 
 * Battery status reader for power settings.
 * Reads from /sys/class/power_supply/BAT* sysfs entries.
 */

/**
 * Reads a sysfs file and returns its content as integer.
 */
static int read_sysfs_int(const char *path) 

{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    int val;
    if (fscanf(fp, "%d", &val) != 1) val = -1;
    fclose(fp);
    return val;
}

/**
 * Reads a sysfs file and returns its content as string (allocated).
 */
static char* read_sysfs_str(const char *path) 

{
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    char buf[64];
    if (fgets(buf, sizeof(buf), fp)) {
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        fclose(fp);
        return strdup(buf);
    }
    fclose(fp);
    return NULL;
}

/**
 * Finds the first battery directory in /sys/class/power_supply/.
 */
static char* find_battery_path(void) 

{
    const char *base = "/sys/class/power_supply/";
    DIR *dir = opendir(base);
    if (!dir) return NULL;
    
    struct dirent *entry;
    char *battery_path = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (strncmp(entry->d_name, "BAT", 3) == 0) {
            battery_path = g_build_filename(base, entry->d_name, NULL);
            break;
        }
    }
    closedir(dir);
    return battery_path;
}

BatteryInfo battery_get_info(void) 

{
    BatteryInfo info = {0};
    info.percentage = -1;
    info.time_remaining = -1;
    info.is_charging = -1;
    info.status_text = NULL;
    
    char *bat_path = find_battery_path();
    if (!bat_path) {
        info.status_text = strdup("No battery detected");
        return info;
    }
    
    char path[512];
    /* Read capacity */
    snprintf(path, sizeof(path), "%s/capacity", bat_path);
    info.percentage = read_sysfs_int(path);
    if (info.percentage < 0) info.percentage = 0;
    
    /* Read status */
    snprintf(path, sizeof(path), "%s/status", bat_path);
    char *status = read_sysfs_str(path);
    if (status) {
        info.is_charging = (strcmp(status, "Charging") == 0);
        free(status);
    } else {
        info.is_charging = 0;
    }
    
    /* Read energy_now and energy_full or charge_now/charge_full */
    int energy_now = -1, energy_full = -1;
    snprintf(path, sizeof(path), "%s/energy_now", bat_path);
    energy_now = read_sysfs_int(path);
    if (energy_now < 0) {
        snprintf(path, sizeof(path), "%s/charge_now", bat_path);
        energy_now = read_sysfs_int(path);
    }
    snprintf(path, sizeof(path), "%s/energy_full", bat_path);
    energy_full = read_sysfs_int(path);
    if (energy_full < 0) {
        snprintf(path, sizeof(path), "%s/charge_full", bat_path);
        energy_full = read_sysfs_int(path);
    }
    
    /* Read power_now for time estimation */
    int power_now = -1;
    snprintf(path, sizeof(path), "%s/power_now", bat_path);
    power_now = read_sysfs_int(path);
    if (power_now < 0) {
        snprintf(path, sizeof(path), "%s/current_now", bat_path);
        power_now = read_sysfs_int(path);
    }
    
    if (power_now > 0 && energy_now > 0) {
        /* power_now is in µW, energy_now in µWh, convert to hours */
        double hours = (double)energy_now / power_now;
        info.time_remaining = (int)(hours * 60); /* minutes */
        if (info.time_remaining < 0) info.time_remaining = -1;
    }
    
    /* Format status text */
    if (info.is_charging) {
        if (info.time_remaining > 0)
            info.status_text = g_strdup_printf("Charging (%d%%, %d min left)", info.percentage, info.time_remaining);
        else
            info.status_text = g_strdup_printf("Charging (%d%%)", info.percentage);
    } else {
        if (info.time_remaining > 0)
            info.status_text = g_strdup_printf("Discharging (%d%%, ~%d min left)", info.percentage, info.time_remaining);
        else
            info.status_text = g_strdup_printf("Discharging (%d%%)", info.percentage);
    }
    
    g_free(bat_path);
    return info;
}

/* GTK widget for battery display */
typedef struct {
    GtkWidget *label;
    guint timeout_id;
} BatteryWidgetPrivate;

static gboolean battery_refresh_cb(gpointer user_data)

{
    GtkWidget *widget = GTK_WIDGET(user_data);
    battery_widget_update(widget);
    return TRUE;
}

GtkWidget *battery_widget_new(void)

{
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), "Battery Status");
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    BatteryWidgetPrivate *priv = g_new0(BatteryWidgetPrivate, 1);
    priv->label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(priv->label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), priv->label, FALSE, FALSE, 0);
    
    /* Progress bar for battery percentage */
    GtkWidget *progress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), progress, FALSE, FALSE, 5);
    
    /* Store references in object data */
    g_object_set_data(G_OBJECT(frame), "battery-label", priv->label);
    g_object_set_data(G_OBJECT(frame), "battery-progress", progress);
    g_object_set_data_full(G_OBJECT(frame), "battery-private", priv, g_free);
    
    /* Initial update */
    battery_widget_update(frame);
    
    /* Start periodic refresh every 30 seconds */
    priv->timeout_id = g_timeout_add_seconds(30, battery_refresh_cb, frame);
    
    /* Cleanup on destroy */
    g_signal_connect_swapped(frame, "destroy", G_CALLBACK(g_source_remove), GUINT_TO_POINTER(priv->timeout_id));
    
    return frame;
}

void battery_widget_update(GtkWidget *widget)

{
    GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "battery-label"));
    GtkWidget *progress = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "battery-progress"));
    if (!label || !progress) return;
    
    BatteryInfo info = battery_get_info();
    gtk_label_set_text(GTK_LABEL(label), info.status_text);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), info.percentage / 100.0);
    
    /* Set color based on percentage */
    GtkCssProvider *provider = gtk_css_provider_new();
    char css[128];
    if (info.percentage < 15)
        snprintf(css, sizeof(css), "progressbar trough progress { background-color: #ff4444; }");
    else if (info.percentage < 30)
        snprintf(css, sizeof(css), "progressbar trough progress { background-color: #ffaa44; }");
    else
        snprintf(css, sizeof(css), "progressbar trough progress { background-color: #44ff44; }");
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(progress);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    free(info.status_text);
}