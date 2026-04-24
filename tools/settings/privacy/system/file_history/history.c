#include "history.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * history.c
 *
 * Maintains a plain‑text log of file accesses in ~/.config/blackline/file_history.log
 * Shows last 20 entries and allows clearing.
 * Also logs new file opens via a function that can be called by other modules.
 */

#define HISTORY_FILE "/.config/blackline/file_history.log"

static GtkListStore *history_store = NULL;
static GtkWidget *history_view = NULL;
static GtkWidget *status_label = NULL;

/* Get path to history file */
static char* get_history_path(void)
{
    const char *home = g_get_home_dir();
    if (!home) home = "/root";
    return g_build_filename(home, ".config/blackline", "file_history.log", NULL);
}

/* Load history entries into the list store */
static void load_history(void)
{
    if (!history_store) return;
    gtk_list_store_clear(history_store);
    
    char *path = get_history_path();
    FILE *fp = fopen(path, "r");
    if (!fp) {
        g_free(path);
        return;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        GtkTreeIter iter;
        gtk_list_store_append(history_store, &iter);
        gtk_list_store_set(history_store, &iter, 0, line, -1);
    }
    fclose(fp);
    g_free(path);
    
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "History loaded");
    }
}

/* Append a new file entry (called by other modules) */
void file_history_log(const char *filepath)
{
    char *path = get_history_path();
    FILE *fp = fopen(path, "a");
    if (fp) {
        fprintf(fp, "%s\n", filepath);
        fclose(fp);
    }
    g_free(path);
    load_history(); // refresh view
}

/* Clear history */
static void clear_history(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    char *path = get_history_path();
    unlink(path);
    /* Also remove the file entirely */
    g_free(path);
    load_history();
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "History cleared");
    }
}

GtkWidget *file_history_widget_new(void)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);

    /* List store with one column: file path */
    history_store = gtk_list_store_new(1, G_TYPE_STRING);
    history_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(history_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(history_view), TRUE);
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Recent Files", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(history_view), col);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 200);
    gtk_container_add(GTK_CONTAINER(scrolled), history_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear History");
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(clear_history), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), clear_btn, FALSE, FALSE, 5);
    
    status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
    
    load_history();
    
    return vbox;
}
