#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

// Define IFNAMSIZ if not defined
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    CONN_TYPE_UNKNOWN,
    CONN_TYPE_ETHERNET,
    CONN_TYPE_WIFI,
    CONN_TYPE_CELLULAR
} ConnectionType;

typedef struct {
    char *ssid;
    int signal_strength;
    gboolean is_secure;
    char *security_type;
} WiFiNetwork;

typedef struct {
    gboolean is_connected;
    ConnectionType type;
    char *interface_name;
    char *ip_address;
    char *mac_address;
    char *ssid;
    int signal_strength;
} ConnectionInfo;

typedef struct {
    GtkWidget *popover;
    GtkWidget *main_box;
    GtkWidget *status_box;
    GtkWidget *network_icon;
    GtkWidget *network_label;
    GtkWidget *wifi_list_box;
    GtkWidget *refresh_button;
    GtkWidget *details_box;
    GtkWidget *connection_details_label;
    GtkWidget *airplane_mode_switch;
    
    gboolean airplane_mode;
    ConnectionInfo current_connection;
    char *selected_ssid;
    
    guint update_timeout_id;
} NetworkManager;

NetworkManager* network_manager_new(void);
void network_manager_show_popover(NetworkManager *nm, GtkWidget *relative_to);
gboolean network_manager_refresh(NetworkManager *nm);
const char* network_manager_get_status_icon(NetworkManager *nm);
const char* network_manager_get_status_text(NetworkManager *nm);
void network_manager_toggle_airplane_mode(NetworkManager *nm, gboolean enabled);
void network_manager_cleanup(NetworkManager *nm);

#endif