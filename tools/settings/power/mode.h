#ifndef LIDE_MODE_H
#define LIDE_MODE_H

#include <gtk/gtk.h>

/**
 * Power mode types.
 */
typedef enum {
    POWER_MODE_LOW,
    POWER_MODE_BALANCED,
    POWER_MODE_HIGH
} PowerMode;

/**
 * Gets the current system power profile.
 *
 * @return Current PowerMode.
 */
PowerMode mode_get_current(void);

/**
 * Sets the system power profile.
 *
 * @param mode New power mode.
 * @return 1 on success, 0 on failure.
 */
int mode_set(PowerMode mode);

/**
 * Creates a GTK widget for power mode selection (radio buttons).
 *
 * @return GtkWidget containing mode selection UI.
 */
GtkWidget *mode_selector_widget_new(void);

#endif /* LIDE_MODE_H */