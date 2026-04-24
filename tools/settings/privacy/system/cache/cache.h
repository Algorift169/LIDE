#ifndef LIDE_CACHE_CLEANER_H
#define LIDE_CACHE_CLEANER_H

#include <gtk/gtk.h>

/**
 * Creates a widget that displays all cache files in a tree view,
 * allows browsing, and provides delete functionality.
 *
 * @return GtkWidget.
 */
GtkWidget *cache_cleaner_widget_new(void);

#endif /* LIDE_CACHE_CLEANER_H */