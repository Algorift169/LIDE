#include "device_privacy.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * device_privacy.c
 *
 * Camera control: list available cameras and allow toggling access globally.
 * Uses polkit to change /dev/video* permissions.
 */

static GtkWidget *status_label = NULL;

/* Check if any camera device exists */
static gboolean camera_exists(void)
{
    for (int i = 0; i < 10; i++) {
        gchar *path = g_strdup_printf("/dev/video%d", i);
        if (g_file_test(path, G_FILE_TEST_EXISTS)) {
            g_free(path);
            return TRUE;
        }
        g_free(path);
    }
    return FALSE;
}

/* Get current camera permission (readable by user?) */
static gboolean is_camera_enabled(void)
{
    /* Check first video device permissions */
    for (int i = 0; i < 10; i++) {
        gchar *path = g_strdup_printf("/dev/video%d", i);
        if (g_file_test(path, G_FILE_TEST_EXISTS)) {
            struct stat st;
            if (stat(path, &st) == 0) {
                /* If readable by owner (root) and group video, and user in video group? */
                /* Simpler: try to open device read-only */
                FILE *f = fopen(path, "r");
                g_free(path);
                if (f) {
                    fclose(f);
                    return TRUE;
                }
                g_free(path);
                return FALSE;
            }
            g_free(path);
        }
        g_free(path);
    }
    return FALSE; /* no camera? treat as disabled */
}

/* Enable camera: use pkexec to chmod 666 on all video devices */
static void enable_camera(void)
{
    if (!camera_exists()) return;
    char cmd[1024] = "pkexec bash -c '";
    strcat(cmd, "for dev in /dev/video*; do chmod 666 $dev 2>/dev/null; done'");
    system(cmd);
    if (status_label)
        gtk_label_set_text(GTK_LABEL(status_label), "Camera enabled (may require logout/restart)");
}

/* Disable camera: remove read/write for others */
static void disable_camera(void)
{
    if (!camera_exists()) return;
    char cmd[1024] = "pkexec bash -c '";
    strcat(cmd, "for dev in /dev/video*; do chmod 600 $dev 2>/dev/null; done'");
    system(cmd);
    if (status_label)
        gtk_label_set_text(GTK_LABEL(status_label), "Camera disabled (may require logout/restart)");
}

static void on_camera_toggled(GtkToggleButton *btn, gpointer data)
{
    (void)data;
    gboolean active = gtk_toggle_button_get_active(btn);
    if (active)
        enable_camera();
    else
        disable_camera();
}

/* Refresh UI to reflect current state */
static void refresh_camera_state(GtkSwitch *sw)
{
    gboolean enabled = is_camera_enabled();
    g_signal_handlers_block_by_func(sw, G_CALLBACK(on_camera_toggled), NULL);
    gtk_switch_set_active(sw, enabled);
    g_signal_handlers_unblock_by_func(sw, G_CALLBACK(on_camera_toggled), NULL);
}

/* Timer to refresh camera state periodically (in case external changes) */
static gboolean refresh_timer(gpointer data)
{
    GtkSwitch *sw = GTK_SWITCH(data);
    refresh_camera_state(sw);
    return TRUE;
}

GtkWidget *device_privacy_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    /* Camera section */
    GtkWidget *camera_frame = gtk_frame_new("Camera");
    GtkWidget *camera_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(camera_box), 10);
    gtk_container_add(GTK_CONTAINER(camera_frame), camera_box);

    GtkWidget *camera_toggle_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *camera_label = gtk_label_new("Allow camera access for applications");
    GtkWidget *camera_switch = gtk_switch_new();

    gtk_box_pack_start(GTK_BOX(camera_toggle_box), camera_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(camera_toggle_box), camera_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(camera_box), camera_toggle_box, FALSE, FALSE, 0);

    /* Info about camera presence */
    if (!camera_exists()) {
        GtkWidget *no_cam = gtk_label_new("No camera detected on this system.");
        gtk_widget_set_sensitive(camera_switch, FALSE);
        gtk_box_pack_start(GTK_BOX(camera_box), no_cam, FALSE, FALSE, 0);
    } else {
        refresh_camera_state(GTK_SWITCH(camera_switch));
        g_signal_connect(camera_switch, "notify::active", G_CALLBACK(on_camera_toggled), NULL);
        /* Refresh every 5 seconds to catch external changes (e.g., other apps) */
        g_timeout_add_seconds(5, refresh_timer, camera_switch);
    }

    gtk_box_pack_start(GTK_BOX(vbox), camera_frame, FALSE, FALSE, 0);

    /* Status label */
    status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 5);

    /* Note for polkit */
    GtkWidget *note = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(note), "<small>Camera access changes require administrative privileges (polkit).</small>");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), note, FALSE, FALSE, 0);

    /* Spacer */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), TRUE, TRUE, 0);

    return vbox;
}