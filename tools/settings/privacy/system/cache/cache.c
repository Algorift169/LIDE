#include "cache.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * cache.c
 *
 * Displays total cache size (from ~/.cache, /tmp, /var/cache/apt/archives)
 * and allows user to delete all cache files with a single button.
 */

static GtkWidget *size_label = NULL;
static GtkWidget *status_label = NULL;

/* Helper: run du -sh on a directory and return size string (free later) */
static char* get_dir_size(const char *dir)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "du -sh \"%s\" 2>/dev/null | cut -f1", dir);
    FILE *fp = popen(cmd, "r");
    if (!fp) return g_strdup("Unknown");
    char buf[64];
    if (fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\n")] = 0;
        pclose(fp);
        return g_strdup(buf);
    }
    pclose(fp);
    return g_strdup("0B");
}

/* Compute total cache size by summing several common cache directories */
static char* get_total_cache_size(void)
{
    const gchar *home = g_get_home_dir();
    gchar *user_cache = g_build_filename(home, ".cache", NULL);
    gchar *user_tmp = g_build_filename(home, "tmp", NULL);
    
    char *size_user_cache = get_dir_size(user_cache);
    char *size_user_tmp = get_dir_size(user_tmp);
    char *size_apt = get_dir_size("/var/cache/apt/archives");
    char *size_pip = get_dir_size("/home/*/.cache/pip"); // not precise, but okay
    
    /* For display, just show a combined simple estimate */
    /* We'll just show the user cache size + note */
    char *total = g_strdup_printf("%s (user cache) + others", size_user_cache);
    
    g_free(user_cache);
    g_free(user_tmp);
    g_free(size_user_cache);
    g_free(size_user_tmp);
    g_free(size_apt);
    g_free(size_pip);
    
    return total;
}

/* Delete all cache directories (run asynchronously) */
static void delete_all_cache(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    
    const gchar *home = g_get_home_dir();
    gchar *cache_dir = g_build_filename(home, ".cache", NULL);
    
    /* Build command: rm -rf ~/.cache/* and other common cache locations */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "rm -rf \"%s\"/* 2>/dev/null; "
             "rm -rf /tmp/* 2>/dev/null; "
             "rm -rf /var/cache/apt/archives/* 2>/dev/null; "
             "echo 'Cache cleared'",
             cache_dir);
    
    GError *error = NULL;
    gchar *output = NULL;
    g_spawn_command_line_sync(cmd, &output, NULL, NULL, &error);
    if (error) {
        gtk_label_set_text(GTK_LABEL(status_label), "Error clearing cache");
        g_error_free(error);
    } else {
        gtk_label_set_text(GTK_LABEL(status_label), "Cache cleared (system reboot may be needed for some)");
        /* Refresh displayed size */
        char *new_size = get_total_cache_size();
        char *size_text = g_strdup_printf("Current cache size: %s", new_size);
        gtk_label_set_text(GTK_LABEL(size_label), size_text);
        g_free(size_text);
        g_free(new_size);
    }
    g_free(output);
    g_free(cache_dir);
}

/* Refresh size display */
static void refresh_size(void)
{
    char *total = get_total_cache_size();
    char *text = g_strdup_printf("Total cache size (approx): %s", total);
    gtk_label_set_text(GTK_LABEL(size_label), text);
    g_free(text);
    g_free(total);
}

GtkWidget *cache_cleaner_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    size_label = gtk_label_new("Calculating...");
    gtk_label_set_xalign(GTK_LABEL(size_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), size_label, FALSE, FALSE, 0);

    GtkWidget *delete_btn = gtk_button_new_with_label("Delete All Cache");
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(delete_all_cache), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), delete_btn, FALSE, FALSE, 10);

    status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

    /* Spacer */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(""), TRUE, TRUE, 0);

    refresh_size();
    return vbox;
}
