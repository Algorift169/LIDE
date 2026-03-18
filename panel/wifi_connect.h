#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <gtk/gtk.h>

void show_wifi_connect_dialog(GtkWidget *parent_window, const char *ssid);
void wifi_quick_connect(const char *ssid);
void wifi_forget_network(const char *ssid);

#endif
