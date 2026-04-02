#include "upload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * upload.c
 * 
 * Upload statistics tracking for network transfers. Mirrors download tracking
 * for outbound transfers. Maintains upload history and throughput metrics.
 */

/* Structure holding network interface transmit statistics.
 * Stores current and previous byte counts to calculate upload speed. */
typedef struct {
    unsigned long long tx_bytes;  /* Current total transmitted bytes */
    unsigned long long tx_old;    /* Previous total transmitted bytes for delta calculation */
    char interface[16];           /* Active network interface name (e.g., eth0, wlan0) */
} UploadStats;

static UploadStats upload_stats = {0};
static guint timeout_id = 0;     /* GLib timeout source ID for periodic updates */

/**
 * Identifies the first non-loopback network interface from /proc/net/dev.
 *
 * @return Pointer to static string containing interface name.
 *         Falls back to "eth0" if no active interface found or file cannot be read.
 *
 * @sideeffect Reads /proc/net/dev to parse interface names.
 * @note The returned pointer is valid until the next call to this function.
 */
static const char* get_active_interface(void)
{
    static char interface[16] = "";
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return "eth0";
    
    char line[256];
    /* Skip header lines */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        char iface[16];
        sscanf(line, "%s", iface);
        /* Remove trailing colon from interface name */
        char *colon = strchr(iface, ':');
        if (colon) *colon = '\0';
        
        /* Exclude loopback interface */
        if (strcmp(iface, "lo") != 0) {
            strcpy(interface, iface);
            break;
        }
    }
    fclose(fp);
    
    return (interface[0] != '\0') ? interface : "eth0";
}

/**
 * Reads current transmit byte count for the active interface from /proc/net/dev.
 *
 * @param stats UploadStats structure containing the interface name to monitor.
 *              The tx_bytes field will be updated with current byte count.
 *
 * @sideeffect Opens and reads /proc/net/dev. Updates stats->tx_bytes on success.
 */
static void read_upload_stats(UploadStats *stats)
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return;
    
    char line[256];
    /* Skip header lines */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        char iface[16];
        unsigned long long rx, tx;
        
        /* Parse: interface rx_bytes (and subsequent fields, then tx_bytes) */
        sscanf(line, "%s %llu %*d %*d %*d %*d %*d %*d %*d %llu", 
               iface, &rx, &tx);
        
        /* Remove trailing colon from interface name */
        char *colon = strchr(iface, ':');
        if (colon) *colon = '\0';
        
        /* Update stats for the monitored interface */
        if (strcmp(iface, stats->interface) == 0) {
            stats->tx_bytes = tx;
            break;
        }
    }
    fclose(fp);
}

/**
 * Initializes the upload monitor.
 * Determines active interface and reads initial byte count.
 *
 * @sideeffect Sets up upload_stats with active interface name and initial byte counts.
 * @sideeffect Prints initialization message to stdout.
 */
void upload_init(void)
{
    strcpy(upload_stats.interface, get_active_interface());
    read_upload_stats(&upload_stats);
    upload_stats.tx_old = upload_stats.tx_bytes;
    printf("Upload monitor initialized on interface: %s\n", upload_stats.interface);
}

/**
 * Calculates current upload speed based on delta between successive reads.
 *
 * @return Upload speed in kilobytes per second (KB/s).
 *         Always non-negative; negative deltas are clamped to 0.
 *
 * @sideeffect Updates tx_old to current tx_bytes for next calculation.
 */
double get_upload_speed(void)
{
    read_upload_stats(&upload_stats);
    unsigned long long tx_new = upload_stats.tx_bytes;
    double speed = (double)(tx_new - upload_stats.tx_old) / 1024.0; /* Convert bytes to KB */
    upload_stats.tx_old = tx_new;
    return speed;
}

/**
 * GLib timeout callback to refresh upload statistics without calculating speed.
 * Keeps the internal byte count current for accurate speed calculation when queried.
 *
 * @param user_data Unused, but required for GLib callback signature.
 * @return G_SOURCE_CONTINUE to keep the timeout active.
 *
 * @sideeffect Updates upload_stats.tx_bytes with current values.
 */
gboolean update_upload_stats(gpointer user_data)
{
    (void)user_data;
    read_upload_stats(&upload_stats);
    return G_SOURCE_CONTINUE;
}

/**
 * Converts an upload speed value to a human-readable string with appropriate unit.
 *
 * @param speed_kb Speed in kilobytes per second.
 * @return Pointer to static string buffer containing formatted speed.
 *         Valid until next call to this function.
 */
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

/**
 * Convenience function that returns current upload speed as a formatted string.
 *
 * @return Pointer to static string containing formatted speed.
 *         Valid until next call to get_upload_speed_formatted() or format_speed().
 *
 * @sideeffect Updates upload_stats state via get_upload_speed().
 */
const char* get_upload_speed_formatted(void)
{
    double speed = get_upload_speed();
    return format_speed(speed);
}

/**
 * Cleans up upload monitor resources.
 * Removes any active GLib timeout source.
 *
 * @sideeffect Stops periodic statistics updates if timeout was active.
 */
void upload_cleanup(void)
{
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
}