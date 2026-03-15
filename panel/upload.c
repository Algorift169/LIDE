#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long long tx_bytes;
    unsigned long long tx_old;
    char interface[16];
} UploadStats;

static UploadStats upload_stats = {0};
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

// Read upload stats (tx_bytes only)
static void read_upload_stats(UploadStats *stats)
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
        
        // Store tx stats for the active interface
        if (strcmp(iface, stats->interface) == 0) {
            stats->tx_bytes = tx;
            break;
        }
    }
    fclose(fp);
}

// Initialize upload stats
void upload_init(void)
{
    strcpy(upload_stats.interface, get_active_interface());
    read_upload_stats(&upload_stats);
    upload_stats.tx_old = upload_stats.tx_bytes;
    printf("Upload monitor initialized on interface: %s\n", upload_stats.interface);
}

// Get upload speed only (outgoing data) in KB/s
double get_upload_speed(void)
{
    read_upload_stats(&upload_stats);
    unsigned long long tx_new = upload_stats.tx_bytes;
    double speed = (double)(tx_new - upload_stats.tx_old) / 1024.0; // KB/s
    upload_stats.tx_old = tx_new;
    return speed;
}

// Update upload stats (call this periodically)
gboolean update_upload_stats(gpointer user_data)
{
    (void)user_data;
    read_upload_stats(&upload_stats);
    return G_SOURCE_CONTINUE;
}

// Format speed with appropriate unit
const char* format_upload_speed(double speed_kb)
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

// Get upload speed as formatted string
const char* get_upload_speed_formatted(void)
{
    double speed = get_upload_speed();
    return format_upload_speed(speed);
}

// Clean up
void upload_cleanup(void)
{
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
}