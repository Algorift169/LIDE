#ifndef LIDE_PANEL_H
#define LIDE_PANEL_H

#include <gtk/gtk.h>
#include <time.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Clock update function
gboolean update_clock(gpointer label);

// Battery functions
int get_battery_percent(void);
gboolean check_battery_charging(void);

// Network functions
void check_internet_connectivity(void);
void check_wifi_status(void);
void launch_network_tab(GtkButton *button, gpointer data);

#endif