#ifndef LIDE_NETWORK_SETTINGS_H
#define LIDE_NETWORK_SETTINGS_H

#include <gtk/gtk.h>

/**
 * Creates the network settings tab widget.
 * Reuses existing panel network components.
 *
 * @return GtkWidget containing the complete network settings UI.
 */
GtkWidget *network_settings_tab_new(void);

#endif