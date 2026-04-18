#include "mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/*
 * mode.c
 * 
 * Power mode control using power-profiles-daemon or sysfs.
 */

/**
 * Executes a command and returns the output (first line).
 */
static char* exec_command_output(const char *cmd) 

{
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    char buf[256];
    if (fgets(buf, sizeof(buf), fp)) {
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        pclose(fp);
        return strdup(buf);
    }
    pclose(fp);
    return NULL;
}

/**
 * Executes a system command and returns exit status.
 */
static int exec_command_status(const char *cmd) 

{
    return system(cmd);
}

PowerMode mode_get_current(void) 

{
    /* Try power-profiles-daemon first */
    char *profile = exec_command_output("powerprofilesctl get 2>/dev/null");
    if (profile) {
        PowerMode mode;
        if (strcmp(profile, "power-saver") == 0)
            mode = POWER_MODE_LOW;
        else if (strcmp(profile, "balanced") == 0)
            mode = POWER_MODE_BALANCED;
        else if (strcmp(profile, "performance") == 0)
            mode = POWER_MODE_HIGH;
        else
            mode = POWER_MODE_BALANCED;
        free(profile);
        return mode;
    }
    
    /* Fallback: read CPU scaling governor */
    FILE *fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
    if (fp) {
        char gov[64];
        if (fgets(gov, sizeof(gov), fp)) {
            char *nl = strchr(gov, '\n');
            if (nl) *nl = '\0';
            fclose(fp);
            if (strcmp(gov, "powersave") == 0) return POWER_MODE_LOW;
            if (strcmp(gov, "performance") == 0) return POWER_MODE_HIGH;
            return POWER_MODE_BALANCED;
        }
        fclose(fp);
    }
    return POWER_MODE_BALANCED;
}

int mode_set(PowerMode mode) {
    const char *profile_name = NULL;
    switch (mode) {
        case POWER_MODE_LOW: profile_name = "power-saver"; break;
        case POWER_MODE_BALANCED: profile_name = "balanced"; break;
        case POWER_MODE_HIGH: profile_name = "performance"; break;
    }
    
    /* Try power-profiles-daemon */
    if (profile_name) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "powerprofilesctl set %s 2>/dev/null", profile_name);
        if (exec_command_status(cmd) == 0) return 1;
    }
    
    /* Fallback: set scaling governor directly */
    const char *gov = NULL;
    switch (mode) {
        case POWER_MODE_LOW: gov = "powersave"; break;
        case POWER_MODE_BALANCED: gov = "ondemand"; break;
        case POWER_MODE_HIGH: gov = "performance"; break;
    }
    if (gov) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "echo %s | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null 2>&1", gov);
        return (exec_command_status(cmd) == 0) ? 1 : 0;
    }
    return 0;
}

/* GTK widget for mode selection */
static GtkWidget *mode_radio_low = NULL;
static GtkWidget *mode_radio_balanced = NULL;
static GtkWidget *mode_radio_high = NULL;

static void on_mode_toggled(GtkToggleButton *button, gpointer data) {
    if (!gtk_toggle_button_get_active(button)) return;
    
    PowerMode mode;
    if (button == GTK_TOGGLE_BUTTON(mode_radio_low))
        mode = POWER_MODE_LOW;
    else if (button == GTK_TOGGLE_BUTTON(mode_radio_balanced))
        mode = POWER_MODE_BALANCED;
    else
        mode = POWER_MODE_HIGH;
    
    if (!mode_set(mode)) {
        /* Show error in status bar if needed, but for now just print */
        g_printerr("Failed to set power mode\n");
    }
}

GtkWidget *mode_selector_widget_new(void) 

{
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), "Power Mode");
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    /* Radio buttons group */
    mode_radio_low = gtk_radio_button_new_with_label(NULL, "Low Power (Power Saver)");
    mode_radio_balanced = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mode_radio_low), "Balanced");
    mode_radio_high = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mode_radio_low), "High Performance");
    
    gtk_box_pack_start(GTK_BOX(vbox), mode_radio_low, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), mode_radio_balanced, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), mode_radio_high, FALSE, FALSE, 0);
    
    /* Set current mode */
    PowerMode current = mode_get_current();
    switch (current) {
        case POWER_MODE_LOW:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mode_radio_low), TRUE);
            break;
        case POWER_MODE_BALANCED:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mode_radio_balanced), TRUE);
            break;
        case POWER_MODE_HIGH:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mode_radio_high), TRUE);
            break;
    }
    
    g_signal_connect(mode_radio_low, "toggled", G_CALLBACK(on_mode_toggled), NULL);
    g_signal_connect(mode_radio_balanced, "toggled", G_CALLBACK(on_mode_toggled), NULL);
    g_signal_connect(mode_radio_high, "toggled", G_CALLBACK(on_mode_toggled), NULL);
    
    return frame;
}