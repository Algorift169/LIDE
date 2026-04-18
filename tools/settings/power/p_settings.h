#ifndef LIDE_POWER_SETTINGS_H
#define LIDE_POWER_SETTINGS_H

#include <gtk/gtk.h>

/**
 * Creates the power settings tab widget.
 * Includes battery status display and power mode selection.
 *
 * @return GtkWidget containing the complete power settings UI.
 */
GtkWidget *power_settings_tab_new(void);

#endif 