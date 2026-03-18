#include "network_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#include <ifaddrs.h>

// Helper function to scan WiFi networks using iwlist
static GList* scan_wifi_networks(void)
{
    GList *networks = NULL;
    FILE *fp = popen("iwlist wlan0 scan 2>/dev/null", "r");
    if (!fp) {
        // Try alternative interfaces
        fp = popen("iwlist wlp2s0 scan 2>/dev/null", "r");
        if (!fp) {
            fp = popen("iwlist wlp3s0 scan 2>/dev/null", "r");
            if (!fp) return NULL;
        }
    }

    char line[1024];
    WiFiNetwork *current_net = NULL;

    while (fgets(line, sizeof(line), fp)) {
        // Check for new cell
        if (strstr(line, "Cell ")) {
            if (current_net) {
                networks = g_list_append(networks, current_net);
            }
            current_net = g_new0(WiFiNetwork, 1);
            current_net->signal_strength = 0;
            current_net->is_secure = FALSE;
        }
        // Get ESSID
        else if (strstr(line, "ESSID:") && current_net) {
            char *start = strchr(line, '"');
            char *end = strrchr(line, '"');
            if (start && end && end > start) {
                *end = '\0';
                current_net->ssid = g_strdup(start + 1);
            }
        }
        // Get encryption status
        else if (strstr(line, "Encryption key:on") && current_net) {
            current_net->is_secure = TRUE;
        }
        else if (strstr(line, "Encryption key:off") && current_net) {
            current_net->is_secure = FALSE;
        }
        // Get signal quality
        else if (strstr(line, "Quality=") && current_net) {
            int qual, max;
            if (sscanf(line, " Quality=%d/%d", &qual, &max) == 2) {
                current_net->signal_strength = (qual * 100) / max;
            }
        }
    }

    if (current_net) {
        networks = g_list_append(networks, current_net);
    }

    pclose(fp);
    return networks;
}

// Check if connected to internet
static int check_internet_connectivity(void)
{
    return (system("ping -W 1 -c 1 8.8.8.8 > /dev/null 2>&1") == 0);
}

// Get current connection info
static ConnectionInfo get_current_connection_info(void)
{
    ConnectionInfo info;
    memset(&info, 0, sizeof(ConnectionInfo));
    
    info.is_connected = check_internet_connectivity();
    
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) return info;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        // Skip loopback
        if (strcmp(ifa->ifa_name, "lo") == 0) continue;
        
        // Check for IPv4
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            
            info.interface_name = g_strdup(ifa->ifa_name);
            info.ip_address = g_strdup(ip);
            
            // Determine connection type
            if (strncmp(ifa->ifa_name, "eth", 3) == 0) {
                info.type = CONN_TYPE_ETHERNET;
            } else if (strncmp(ifa->ifa_name, "wlan", 4) == 0 || 
                       strncmp(ifa->ifa_name, "wlp", 3) == 0) {
                info.type = CONN_TYPE_WIFI;
                
                // Get WiFi signal strength using iwconfig
                char cmd[256];
                char result[256];
                snprintf(cmd, sizeof(cmd), "iwconfig %s 2>/dev/null | grep 'Quality='", ifa->ifa_name);
                FILE *fp = popen(cmd, "r");
                if (fp) {
                    if (fgets(result, sizeof(result), fp)) {
                        int qual, max;
                        if (sscanf(result, " Quality=%d/%d", &qual, &max) == 2) {
                            info.signal_strength = (qual * 100) / max;
                        }
                    }
                    pclose(fp);
                }
            }
            
            // Get MAC address
            char mac_path[256];
            char mac_line[32];
            snprintf(mac_path, sizeof(mac_path), "/sys/class/net/%s/address", ifa->ifa_name);
            FILE *mac_fp = fopen(mac_path, "r");
            if (mac_fp) {
                if (fgets(mac_line, sizeof(mac_line), mac_fp)) {
                    char *newline = strchr(mac_line, '\n');
                    if (newline) *newline = '\0';
                    info.mac_address = g_strdup(mac_line);
                }
                fclose(mac_fp);
            }
            
            break;
        }
    }
    
    freeifaddrs(ifaddr);
    return info;
}

// Forward declaration for callback
static void on_connect_button_clicked(GtkButton *btn, gpointer user_data);

// Create WiFi network row widget
static GtkWidget* create_wifi_network_row(WiFiNetwork *net, NetworkManager *nm)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(row, 10);
    gtk_widget_set_margin_end(row, 10);
    gtk_widget_set_margin_top(row, 5);
    gtk_widget_set_margin_bottom(row, 5);
    
    // Signal strength icon
    const char *signal_icon;
    if (net->signal_strength > 80) signal_icon = "󰤨";
    else if (net->signal_strength > 60) signal_icon = "󰤥";
    else if (net->signal_strength > 40) signal_icon = "󰤢";
    else if (net->signal_strength > 20) signal_icon = "󰤟";
    else signal_icon = "󰤯";
    
    GtkWidget *signal_label = gtk_label_new(signal_icon);
    gtk_widget_set_name(signal_label, "network-icon");
    gtk_box_pack_start(GTK_BOX(row), signal_label, FALSE, FALSE, 0);
    
    // SSID
    GtkWidget *ssid_label = gtk_label_new(net->ssid ? net->ssid : "Unknown");
    gtk_label_set_xalign(GTK_LABEL(ssid_label), 0);
    gtk_box_pack_start(GTK_BOX(row), ssid_label, TRUE, TRUE, 0);
    
    // Security icon
    if (net->is_secure) {
        GtkWidget *lock_label = gtk_label_new("🔒");
        gtk_widget_set_name(lock_label, "network-icon");
        gtk_box_pack_start(GTK_BOX(row), lock_label, FALSE, FALSE, 5);
    }
    
    // Connect button
    GtkWidget *connect_btn = gtk_button_new_with_label("Connect");
    g_object_set_data_full(G_OBJECT(connect_btn), "ssid", g_strdup(net->ssid), g_free);
    g_object_set_data(G_OBJECT(connect_btn), "is_secure", GINT_TO_POINTER(net->is_secure));
    
    g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_button_clicked), nm);
    gtk_box_pack_start(GTK_BOX(row), connect_btn, FALSE, FALSE, 0);
    
    return row;
}

// Connect button callback
static void on_connect_button_clicked(GtkButton *btn, gpointer user_data)
{
    NetworkManager *nm = (NetworkManager*)user_data;
    const char *ssid = (const char*)g_object_get_data(G_OBJECT(btn), "ssid");
    gboolean is_secure = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "is_secure"));
    
    if (ssid) {
        if (nm->selected_ssid) g_free(nm->selected_ssid);
        nm->selected_ssid = g_strdup(ssid);
        
        if (is_secure) {
            // Show password dialog
            GtkWidget *dialog = gtk_dialog_new_with_buttons(
                "WiFi Password",
                NULL,
                GTK_DIALOG_MODAL,
                "_Cancel", GTK_RESPONSE_CANCEL,
                "_Connect", GTK_RESPONSE_ACCEPT,
                NULL);
            
            GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            gtk_container_set_border_width(GTK_CONTAINER(box), 10);
            
            GtkWidget *label = gtk_label_new("Enter password for:");
            GtkWidget *ssid_label = gtk_label_new(ssid);
            char *markup = g_strdup_printf("<b>%s</b>", ssid);
            gtk_label_set_markup(GTK_LABEL(ssid_label), markup);
            g_free(markup);
            
            GtkWidget *entry = gtk_entry_new();
            gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
            gtk_entry_set_invisible_char(GTK_ENTRY(entry), '*');
            
            gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 5);
            gtk_box_pack_start(GTK_BOX(box), ssid_label, FALSE, FALSE, 5);
            gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 5);
            
            gtk_container_add(GTK_CONTAINER(content), box);
            gtk_widget_show_all(dialog);
            
            gint response = gtk_dialog_run(GTK_DIALOG(dialog));
            if (response == GTK_RESPONSE_ACCEPT) {
                const char *password = gtk_entry_get_text(GTK_ENTRY(entry));
                char cmd[512];
                snprintf(cmd, sizeof(cmd), 
                         "nmcli dev wifi connect '%s' password '%s' 2>/dev/null", 
                         ssid, password);
                system(cmd);
            }
            
            gtk_widget_destroy(dialog);
        } else {
            // Connect to open network
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "nmcli dev wifi connect '%s' 2>/dev/null", ssid);
            system(cmd);
        }
        
        network_manager_refresh(nm);
    }
}

// Create the network manager popover
static GtkWidget* create_network_popover(NetworkManager *nm)
{
    GtkWidget *popover = gtk_popover_new(NULL);
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 15);
    
    // Header with airplane mode toggle
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Network</b>");
    gtk_box_pack_start(GTK_BOX(header_box), title, TRUE, TRUE, 0);
    
    nm->airplane_mode_switch = gtk_switch_new();
    gtk_box_pack_start(GTK_BOX(header_box), nm->airplane_mode_switch, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(main_box), header_box, FALSE, FALSE, 0);
    
    // Separator
    gtk_box_pack_start(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);
    
    // Current connection status
    nm->status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(nm->status_box, 5);
    gtk_widget_set_margin_end(nm->status_box, 5);
    
    nm->network_icon = gtk_label_new("󰤨");
    gtk_widget_set_name(nm->network_icon, "network-status-icon");
    gtk_box_pack_start(GTK_BOX(nm->status_box), nm->network_icon, FALSE, FALSE, 0);
    
    nm->network_label = gtk_label_new("Not connected");
    gtk_label_set_xalign(GTK_LABEL(nm->network_label), 0);
    gtk_box_pack_start(GTK_BOX(nm->status_box), nm->network_label, TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(main_box), nm->status_box, FALSE, FALSE, 0);
    
    // Refresh button
    nm->refresh_button = gtk_button_new_with_label("↻ Refresh");
    gtk_box_pack_start(GTK_BOX(main_box), nm->refresh_button, FALSE, FALSE, 5);
    
    // WiFi networks list
    GtkWidget *wifi_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(wifi_label), "<b>Available Networks</b>");
    gtk_widget_set_margin_top(wifi_label, 10);
    gtk_box_pack_start(GTK_BOX(main_box), wifi_label, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 200);
    
    nm->wifi_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(scrolled), nm->wifi_list_box);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled, TRUE, TRUE, 0);
    
    // Connection details
    nm->details_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(nm->details_box, 10);
    
    nm->connection_details_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(nm->connection_details_label), 0);
    gtk_box_pack_start(GTK_BOX(nm->details_box), nm->connection_details_label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(main_box), nm->details_box, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(popover), main_box);
    
    return popover;
}

// Update network info display
static void update_network_display(NetworkManager *nm)
{
    ConnectionInfo info = get_current_connection_info();
    
    // Free old data
    if (nm->current_connection.interface_name) g_free(nm->current_connection.interface_name);
    if (nm->current_connection.ip_address) g_free(nm->current_connection.ip_address);
    if (nm->current_connection.mac_address) g_free(nm->current_connection.mac_address);
    if (nm->current_connection.ssid) g_free(nm->current_connection.ssid);
    
    nm->current_connection = info;
    
    // Update icon based on connection type and strength
    const char *icon;
    if (nm->airplane_mode) {
        icon = "󰀝";
    } else if (!info.is_connected) {
        icon = "󰤮";
    } else if (info.type == CONN_TYPE_ETHERNET) {
        icon = "󰈀";
    } else if (info.type == CONN_TYPE_WIFI) {
        if (info.signal_strength > 80) icon = "󰤨";
        else if (info.signal_strength > 60) icon = "󰤥";
        else if (info.signal_strength > 40) icon = "󰤢";
        else if (info.signal_strength > 20) icon = "󰤟";
        else icon = "󰤯";
    } else {
        icon = "󰤨";
    }
    
    gtk_label_set_text(GTK_LABEL(nm->network_icon), icon);
    
    // Update connection status text
    char status_text[256];
    if (nm->airplane_mode) {
        snprintf(status_text, sizeof(status_text), "Airplane Mode");
    } else if (!info.is_connected) {
        snprintf(status_text, sizeof(status_text), "Not connected");
    } else if (info.type == CONN_TYPE_ETHERNET) {
        snprintf(status_text, sizeof(status_text), "Ethernet - %s", 
                 info.ip_address ? info.ip_address : "Connected");
    } else if (info.type == CONN_TYPE_WIFI && info.ssid) {
        snprintf(status_text, sizeof(status_text), "%s - %s", 
                 info.ssid, info.ip_address ? info.ip_address : "");
    } else {
        snprintf(status_text, sizeof(status_text), "Connected");
    }
    gtk_label_set_text(GTK_LABEL(nm->network_label), status_text);
    
    // Update details
    char details[1024] = "";
    if (info.interface_name) {
        snprintf(details + strlen(details), sizeof(details) - strlen(details),
                "Interface: %s\n", info.interface_name);
    }
    if (info.ip_address) {
        snprintf(details + strlen(details), sizeof(details) - strlen(details),
                "IP Address: %s\n", info.ip_address);
    }
    if (info.mac_address) {
        snprintf(details + strlen(details), sizeof(details) - strlen(details),
                "MAC: %s\n", info.mac_address);
    }
    if (info.type == CONN_TYPE_WIFI && info.signal_strength > 0) {
        snprintf(details + strlen(details), sizeof(details) - strlen(details),
                "Signal: %d%%\n", info.signal_strength);
    }
    
    gtk_label_set_text(GTK_LABEL(nm->connection_details_label), details);
}

// Refresh WiFi networks list
static void refresh_wifi_list(NetworkManager *nm)
{
    // Clear existing list
    GList *children = gtk_container_get_children(GTK_CONTAINER(nm->wifi_list_box));
    for (GList *child = children; child; child = child->next) {
        gtk_widget_destroy(GTK_WIDGET(child->data));
    }
    g_list_free(children);
    
    // Scan for networks
    GList *networks = scan_wifi_networks();
    
    if (!networks) {
        GtkWidget *no_networks = gtk_label_new("No networks found");
        gtk_box_pack_start(GTK_BOX(nm->wifi_list_box), no_networks, FALSE, FALSE, 5);
    } else {
        for (GList *iter = networks; iter; iter = iter->next) {
            WiFiNetwork *net = (WiFiNetwork*)iter->data;
            GtkWidget *row = create_wifi_network_row(net, nm);
            gtk_box_pack_start(GTK_BOX(nm->wifi_list_box), row, FALSE, FALSE, 0);
            
            g_free(net->ssid);
            g_free(net);
        }
        g_list_free(networks);
    }
    
    gtk_widget_show_all(nm->wifi_list_box);
}

// Refresh callback
static void on_refresh_clicked(GtkButton *button, NetworkManager *nm)
{
    (void)button;
    network_manager_refresh(nm);
}

// Airplane mode toggle callback
static void on_airplane_mode_toggled(GtkSwitch *sw, gboolean state, NetworkManager *nm)
{
    (void)sw;
    network_manager_toggle_airplane_mode(nm, state);
}

// Initialize network manager
NetworkManager* network_manager_new(void)
{
    NetworkManager *nm = g_new0(NetworkManager, 1);
    
    nm->popover = create_network_popover(nm);
    nm->main_box = gtk_bin_get_child(GTK_BIN(nm->popover));
    nm->airplane_mode = FALSE;
    nm->selected_ssid = NULL;
    
    // Connect signals
    g_signal_connect(nm->refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), nm);
    g_signal_connect(nm->airplane_mode_switch, "state-set", G_CALLBACK(on_airplane_mode_toggled), nm);
    
    // Initial refresh
    network_manager_refresh(nm);
    
    // Set up periodic updates
    nm->update_timeout_id = g_timeout_add_seconds(2, (GSourceFunc)network_manager_refresh, nm);
    
    return nm;
}

// Show popover
void network_manager_show_popover(NetworkManager *nm, GtkWidget *relative_to)
{
    gtk_popover_set_relative_to(GTK_POPOVER(nm->popover), relative_to);
    gtk_popover_popup(GTK_POPOVER(nm->popover));
}

// Refresh all network info
gboolean network_manager_refresh(NetworkManager *nm)
{
    if (!nm) return G_SOURCE_REMOVE;
    
    update_network_display(nm);
    
    if (!nm->airplane_mode) {
        refresh_wifi_list(nm);
    }
    
    return G_SOURCE_CONTINUE;
}

// Get status icon for panel
const char* network_manager_get_status_icon(NetworkManager *nm)
{
    if (!nm) return "󰤮";
    
    if (nm->airplane_mode) return "󰀝";
    
    ConnectionInfo info = nm->current_connection;
    
    if (!info.is_connected) return "󰤮";
    if (info.type == CONN_TYPE_ETHERNET) return "󰈀";
    if (info.type == CONN_TYPE_WIFI) {
        if (info.signal_strength > 80) return "󰤨";
        if (info.signal_strength > 60) return "󰤥";
        if (info.signal_strength > 40) return "󰤢";
        if (info.signal_strength > 20) return "󰤟";
        return "󰤯";
    }
    return "󰤨";
}

// Get status text for panel
const char* network_manager_get_status_text(NetworkManager *nm)
{
    if (!nm) return "No Connection";
    
    if (nm->airplane_mode) return "Airplane Mode";
    
    ConnectionInfo info = nm->current_connection;
    
    if (!info.is_connected) return "No Connection";
    if (info.type == CONN_TYPE_ETHERNET) return "Ethernet";
    if (info.type == CONN_TYPE_WIFI && info.ssid) return info.ssid;
    
    return "Connected";
}

// Toggle airplane mode
void network_manager_toggle_airplane_mode(NetworkManager *nm, gboolean enabled)
{
    if (!nm) return;
    
    nm->airplane_mode = enabled;
    
    if (enabled) {
        system("nmcli radio wifi off 2>/dev/null");
        system("nmcli networking off 2>/dev/null");
    } else {
        system("nmcli radio wifi on 2>/dev/null");
        system("nmcli networking on 2>/dev/null");
    }
    
    network_manager_refresh(nm);
}

// Cleanup
void network_manager_cleanup(NetworkManager *nm)
{
    if (!nm) return;
    
    if (nm->update_timeout_id > 0) {
        g_source_remove(nm->update_timeout_id);
    }
    
    if (nm->popover) {
        gtk_widget_destroy(nm->popover);
    }
    
    if (nm->current_connection.interface_name) g_free(nm->current_connection.interface_name);
    if (nm->current_connection.ip_address) g_free(nm->current_connection.ip_address);
    if (nm->current_connection.mac_address) g_free(nm->current_connection.mac_address);
    if (nm->current_connection.ssid) g_free(nm->current_connection.ssid);
    if (nm->selected_ssid) g_free(nm->selected_ssid);
    
    g_free(nm);
}