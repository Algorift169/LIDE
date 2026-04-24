#ifndef LIDE_PRIVACY_SETTINGS_H
#define LIDE_PRIVACY_SETTINGS_H

#include <gtk/gtk.h>

/**
 * Creates the privacy settings tab widget.
 * Contains two sub‑tabs: System and Devices.
 *
 * @return GtkWidget containing the complete privacy settings UI.
 */
GtkWidget *privacy_settings_tab_new(void);

#endif /* LIDE_PRIVACY_SETTINGS_H */
