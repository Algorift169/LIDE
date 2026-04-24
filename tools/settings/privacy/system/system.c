#include "system.h"
#include "cache/cache.h"
#include "file_history/history.h"
#include "location/location.h"
#include "screen_lock/screen_lock.h"

/*
 * system.c
 *
 * Organises system‑level privacy controls in a notebook:
 * - Cache management
 * - File usage history
 * - Location services
 * - Screen lock (idle time + lock timer)
 */

GtkWidget *system_settings_widget_new(void)
{
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 5);

    /* Cache tab */
    GtkWidget *cache_tab = cache_cleaner_widget_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cache_tab,
                             gtk_label_new("Cache"));

    /* File History tab */
    GtkWidget *history_tab = file_history_widget_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), history_tab,
                             gtk_label_new("File History"));

    /* Location tab */
    GtkWidget *location_tab = location_settings_widget_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), location_tab,
                             gtk_label_new("Location"));

    /* Screen Lock tab */
    GtkWidget *lock_tab = screen_lock_widget_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), lock_tab,
                             gtk_label_new("Screen Lock"));

    return notebook;
}
