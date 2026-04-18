#ifndef LIDE_BATARY_H
#define LIDE_BATARY_H

#include <gtk/gtk.h>

/**
 * Battery information structure.
 */
typedef struct {
    int percentage;        /* Battery charge percentage (0-100) */
    int is_charging;       /* 1 if charging, 0 if discharging */
    int time_remaining;    /* Remaining time in minutes (discharging) or charging time */
    char *status_text;     /* Human-readable status string */
} BatteryInfo;

/**
 * Reads current battery status from sysfs.
 *
 * @return BatteryInfo structure with fresh data. Caller must free status_text.
 */
BatteryInfo battery_get_info(void);

/**
 * Creates a GTK widget displaying battery percentage and time remaining.
 *
 * @return GtkWidget containing battery information display.
 */
GtkWidget *battery_widget_new(void);

/**
 * Updates the battery information widget with current data.
 *
 * @param widget The battery widget to update.
 */
void battery_widget_update(GtkWidget *widget);

#endif /* LIDE_BATARY_H */