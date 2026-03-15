#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_old;
    unsigned long long tx_old;
    char interface[16];
} NetStats;

static NetStats net_stats = {0};
static guint timeout_id = 0;

// Get active network interface (non-loopback)
static const char* get_active_interface(void)
{
    static char interface[16] = "";
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return "eth0";
    
    char line[256];
    // Skip first two header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        char iface[16];
        sscanf(line, "%s", iface);
        // Remove trailing colon
        char *colon = strchr(iface, ':');
        if (colon) *colon = '\0';
        
        // Skip loopback
        if (strcmp(iface, "lo") != 0) {
            strcpy(interface, iface);
            break;
        }
    }
    fclose(fp);
    
    return (interface[0] != '\0') ? interface : "eth0";
}

// Read network stats
static void read_net_stats(NetStats *stats)
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return;
    
    char line[256];
    // Skip first two header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        char iface[16];
        unsigned long long rx, tx;
        
        // Parse interface name and stats
        sscanf(line, "%s %llu %*d %*d %*d %*d %*d %*d %*d %llu", 
               iface, &rx, &tx);
        
        // Remove trailing colon from interface name
        char *colon = strchr(iface, ':');
        if (colon) *colon = '\0';
        
        // Store stats for the active interface
        if (strcmp(iface, stats->interface) == 0) {
            stats->rx_bytes = rx;
            stats->tx_bytes = tx;
            break;
        }
    }
    fclose(fp);
}

// Initialize network stats
void network_stats_init(void)
{
    strcpy(net_stats.interface, get_active_interface());
    read_net_stats(&net_stats);
    net_stats.rx_old = net_stats.rx_bytes;
    net_stats.tx_old = net_stats.tx_bytes;
    printf("Network monitor initialized on interface: %s\n", net_stats.interface);
}

// Get download speed (incoming) in KB/s
double get_download_speed(void)
{
    read_net_stats(&net_stats);
    unsigned long long rx_new = net_stats.rx_bytes;
    double speed = (double)(rx_new - net_stats.rx_old) / 1024.0; // KB/s
    net_stats.rx_old = rx_new;
    return speed;
}

// Get upload speed (outgoing) in KB/s
double get_upload_speed(void)
{
    read_net_stats(&net_stats);
    unsigned long long tx_new = net_stats.tx_bytes;
    double speed = (double)(tx_new - net_stats.tx_old) / 1024.0; // KB/s
    net_stats.tx_old = tx_new;
    return speed;
}

// Update network stats (call this periodically)
gboolean update_network_stats(gpointer user_data)
{
    (void)user_data;
    read_net_stats(&net_stats);
    return G_SOURCE_CONTINUE;
}

// Format speed with appropriate unit
const char* format_speed(double speed_kb)
{
    static char buffer[32];
    if (speed_kb < 0) speed_kb = 0;
    
    if (speed_kb < 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB/s", speed_kb);
    } else if (speed_kb < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f MB/s", speed_kb / 1024.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f GB/s", speed_kb / (1024.0 * 1024.0));
    }
    return buffer;
}

// Clean up
void network_stats_cleanup(void)
{
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
}