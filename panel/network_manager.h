#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

/* Define IFNAMSIZ if not defined */
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Network connection type enumeration.
 * Identifies the medium used for network connectivity.
 */
typedef enum {
    CONN_TYPE_UNKNOWN,    /* Connection type could not be determined */
    CONN_TYPE_ETHERNET,   /* Wired Ethernet connection */
    CONN_TYPE_WIFI,       /* Wireless 802.11 connection */
    CONN_TYPE_CELLULAR    /* Mobile/cellular connection (reserved) */
} ConnectionType;

/**
 * WiFi network information structure.
 * Represents a discovered wireless network during scanning.
 */
typedef struct {
    char *ssid;                  /* Network name (Service Set Identifier) */
    int signal_strength;         /* Signal quality as percentage (0-100) */
    gboolean is_secure;          /* TRUE if network requires authentication */
    char *security_type;         /* Security protocol (WPA2, WPA3, etc.) - currently unused */
} WiFiNetwork;

/**
 * Current network connection information.
 * Describes the active network connection state and details.
 */
typedef struct {
    gboolean is_connected;       /* TRUE if internet connectivity is confirmed */
    ConnectionType type;         /* Network medium type */
    char *interface_name;        /* Network interface name (e.g., eth0, wlan0) */
    char *ip_address;            /* IPv4 address assigned to the interface */
    char *mac_address;           /* MAC/hardware address of the interface */
    char *ssid;                  /* WiFi SSID (only for wireless connections) */
    int signal_strength;         /* WiFi signal strength as percentage (only for wireless) */
} ConnectionInfo;

/**
 * Network Manager main structure.
 * Encapsulates all state and UI components for network management.
 */
typedef struct {
    GtkWidget *popover;                 /* Main popover container widget */
    GtkWidget *main_box;                /* Vertical box containing all controls */
    GtkWidget *status_box;              /* Horizontal box for status display */
    GtkWidget *network_icon;            /* Label showing network status icon */
    GtkWidget *network_label;           /* Label showing network status text */
    GtkWidget *wifi_list_box;           /* Container for available WiFi network rows */
    GtkWidget *refresh_button;          /* Button to manually refresh network scan */
    GtkWidget *details_box;             /* Container for connection details */
    GtkWidget *connection_details_label;/* Label showing detailed connection info */
    GtkWidget *airplane_mode_switch;    /* Switch toggling airplane mode */
    
    gboolean airplane_mode;             /* TRUE if airplane mode is enabled */
    ConnectionInfo current_connection;  /* Current active connection information */
    char *selected_ssid;                /* SSID of selected network (during connection attempt) */
    
    guint update_timeout_id;            /* GLib timeout ID for periodic refresh */
} NetworkManager;

/**
 * Creates and initializes a new NetworkManager instance.
 * Sets up the popover UI, starts periodic refresh timer, and initializes connection state.
 *
 * @return Newly allocated NetworkManager structure.
 *         Must be freed with network_manager_cleanup() when no longer needed.
 *
 * @sideeffect Creates GTK widgets and schedules periodic network updates.
 */
NetworkManager* network_manager_new(void);

/**
 * Displays the network manager popover anchored to a widget.
 *
 * @param nm          NetworkManager instance.
 * @param relative_to Widget to which the popover will be anchored.
 *                    Must be a valid visible GTK widget.
 */
void network_manager_show_popover(NetworkManager *nm, GtkWidget *relative_to);

/**
 * Refreshes all network information.
 * Updates connection status, current connection details, and WiFi network list.
 *
 * @param nm NetworkManager instance.
 * @return  G_SOURCE_CONTINUE to keep the calling timeout active,
 *          G_SOURCE_REMOVE if nm is NULL.
 *
 * @sideeffect Updates UI widgets in the popover.
 * @sideeffect Scans for WiFi networks if not in airplane mode.
 */
gboolean network_manager_refresh(NetworkManager *nm);

/**
 * Returns the current network status icon for panel display.
 *
 * @param nm NetworkManager instance.
 * @return Static string containing a Unicode icon character.
 *         Icon varies based on connection type and signal strength.
 *
 * @note The returned string is a pointer to static data and should not be freed.
 */
const char* network_manager_get_status_icon(NetworkManager *nm);

/**
 * Returns the current network status text for panel display.
 *
 * @param nm NetworkManager instance.
 * @return Static string containing connection status description.
 *         Returns "Airplane Mode", "Ethernet", SSID name, or connection state.
 *
 * @note The returned string is a pointer to static data and should not be freed.
 */
const char* network_manager_get_status_text(NetworkManager *nm);

/**
 * Toggles airplane mode.
 * Enables or disables WiFi and all networking via NetworkManager.
 *
 * @param nm      NetworkManager instance.
 * @param enabled TRUE to enable airplane mode, FALSE to disable.
 *
 * @sideeffect Executes nmcli commands to control network state.
 * @sideeffect Triggers network refresh after state change.
 */
void network_manager_toggle_airplane_mode(NetworkManager *nm, gboolean enabled);

/**
 * Cleans up NetworkManager resources.
 * Removes timeout source, destroys popover widget, frees all allocated memory.
 *
 * @param nm NetworkManager instance to clean up.
 *           After this call, nm is invalid and should not be used.
 */
void network_manager_cleanup(NetworkManager *nm);

#endif /* NETWORK_MANAGER_H */