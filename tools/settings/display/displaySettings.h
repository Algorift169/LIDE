#ifndef DISPLAY_SETTINGS_H
#define DISPLAY_SETTINGS_H

#include <gtk/gtk.h>

/**
 * Creates the display settings tab widget.
 * Provides a composite UI containing orientation, refresh rate, and resolution controls.
 *
 * @return GtkWidget containing the complete display settings UI.
 *         The widget is a vertically packed GtkBox with framed sections.
 *         Caller assumes ownership of the returned widget.
 */
GtkWidget *display_settings_tab_new(void);

#endif /* DISPLAY_SETTINGS_H */