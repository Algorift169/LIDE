#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Structure holding network interface receive statistics.
 * Stores current and previous byte counts to calculate download speed. */
typedef struct {
    unsigned long long rx_bytes;  /* Current total received bytes */
    unsigned long long rx_old;    /* Previous total received bytes for delta calculation */
    char interface[16];           /* Active network interface name (e.g., eth0, wlan0) */
} DownloadStats;

static DownloadStats download_stats = {0};
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
 * Reads current receive byte count for the active interface from /proc/net/dev.
 *
 * @param stats DownloadStats structure containing the interface name to monitor.
 *              The rx_bytes field will be updated with current byte count.
 *
 * @sideeffect Opens and reads /proc/net/dev. Updates stats->rx_bytes on success.
 */
static void read_download_stats(DownloadStats *stats)

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
            stats->rx_bytes = rx;
            break;
        }
    }
    fclose(fp);
}

/**
 * Initializes the download monitor.
 * Determines active interface and reads initial byte count.
 *
 * @sideeffect Sets up download_stats with active interface name and initial byte counts.
 * @sideeffect Prints initialization message to stdout.
 */
void download_init(void)

{
    strcpy(download_stats.interface, get_active_interface());
    read_download_stats(&download_stats);
    download_stats.rx_old = download_stats.rx_bytes;
    printf("Download monitor initialized on interface: %s\n", download_stats.interface);
}

/**
 * Calculates current download speed based on delta between successive reads.
 *
 * @return Download speed in kilobytes per second (KB/s).
 *         Always non-negative; negative deltas are clamped to 0.
 *
 * @sideeffect Updates rx_old to current rx_bytes for next calculation.
 */
double get_download_speed(void)

{
    read_download_stats(&download_stats);
    unsigned long long rx_new = download_stats.rx_bytes;
    double speed = (double)(rx_new - download_stats.rx_old) / 1024.0; /* Convert bytes to KB */
    download_stats.rx_old = rx_new;
    return speed;
}

/**
 * GLib timeout callback to refresh download statistics without calculating speed.
 * Keeps the internal byte count current for accurate speed calculation when queried.
 *
 * @param user_data Unused, but required for GLib callback signature.
 * @return G_SOURCE_CONTINUE to keep the timeout active.
 *
 * @sideeffect Updates download_stats.rx_bytes with current values.
 */
gboolean update_download_stats(gpointer user_data)

{
    (void)user_data;
    read_download_stats(&download_stats);
    return G_SOURCE_CONTINUE;
}

/**
 * Converts a download speed value to a human-readable string with appropriate unit.
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
 * Convenience function that returns current download speed as a formatted string.
 *
 * @return Pointer to static string containing formatted speed.
 *         Valid until next call to get_download_speed_formatted() or format_speed().
 *
 * @sideeffect Updates download_stats state via get_download_speed().
 */
const char* get_download_speed_formatted(void)

{
    double speed = get_download_speed();
    return format_speed(speed);
}

/**
 * Cleans up download monitor resources.
 * Removes any active GLib timeout source.
 *
 * @sideeffect Stops periodic statistics updates if timeout was active.
 */
void download_cleanup(void)

{
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
}
