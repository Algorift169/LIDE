#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long long rx_bytes;
    unsigned long long rx_old;
    char interface[16];
} DownloadStats;

static DownloadStats download_stats = {0};
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

// Read download stats (rx_bytes only)
static void read_download_stats(DownloadStats *stats)
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
        
        // Store rx stats for the active interface
        if (strcmp(iface, stats->interface) == 0) {
            stats->rx_bytes = rx;
            break;
        }
    }
    fclose(fp);
}

// Initialize download stats
void download_init(void)
{
    strcpy(download_stats.interface, get_active_interface());
    read_download_stats(&download_stats);
    download_stats.rx_old = download_stats.rx_bytes;
    printf("Download monitor initialized on interface: %s\n", download_stats.interface);
}

// Get download speed only (incoming data) in KB/s
double get_download_speed(void)
{
    read_download_stats(&download_stats);
    unsigned long long rx_new = download_stats.rx_bytes;
    double speed = (double)(rx_new - download_stats.rx_old) / 1024.0; // KB/s
    download_stats.rx_old = rx_new;
    return speed;
}

// Update download stats (call this periodically)
gboolean update_download_stats(gpointer user_data)
{
    (void)user_data;
    read_download_stats(&download_stats);
    return G_SOURCE_CONTINUE;
}

// Format speed with appropriate unit
static const char* format_speed(double speed_kb)
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

// Get download speed as formatted string
const char* get_download_speed_formatted(void)
{
    double speed = get_download_speed();
    return format_speed(speed);
}

// Clean up
void download_cleanup(void)
{
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
}