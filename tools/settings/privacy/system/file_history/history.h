#ifndef LIDE_FILE_HISTORY_H
#define LIDE_FILE_HISTORY_H

#include <gtk/gtk.h>

/**
 * Creates a widget that displays recently opened files history
 * and allows clearing the history.
 *
 * @return GtkWidget.
 */
GtkWidget *file_history_widget_new(void);

#endif /* LIDE_FILE_HISTORY_H */
