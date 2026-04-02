#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glib.h>

/*
 * network_stats.c
 * 
 * Network statistics collection and display. Measures bandwidth utilization
 * by parsing /proc/net/dev. Periodically updates byte counters for active interfaces.
 */

/* Network interface statistics structure.
 * Holds byte counts and calculated speeds for all active interfaces. */
typedef struct {
    unsigned long long rx_bytes;     /* Total received bytes across all active interfaces */
    unsigned long long tx_bytes;     /* Total transmitted bytes across all active interfaces */
    double upload_speed;             /* Current upload speed in bytes per second */
    double download_speed;           /* Current download speed in bytes per second */
    time_t last_update;              /* Timestamp of last statistics update */
} NetworkStats;

static NetworkStats stats = {0};
static int initialized = 0;          /* Flag indicating whether stats have been initialized */

/**
 * Reads network byte counts from /proc/net/dev.
 * Aggregates receive and transmit bytes across all non-loopback interfaces.
 *
 * @param rx Output parameter for total received bytes.
 * @param tx Output parameter for total transmitted bytes.
 * @return   0 on success, -1 on file open failure.
 *
 * @sideeffect Opens and reads /proc/net/dev.
 */
static int read_network_stats(unsigned long long *rx, unsigned long long *tx)
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return -1;

    char line[256];
    unsigned long long total_rx = 0, total_tx = 0;

    /* Skip header lines */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char iface[32];
        unsigned long long r_bytes, r_packets, r_errs, r_drop, r_fifo, r_frame, r_compressed, r_multicast;
        unsigned long long t_bytes, t_packets, t_errs, t_drop, t_fifo, t_colls, t_carrier, t_compressed;

        /* Parse all 16 fields from /proc/net/dev line.
         * Format: interface: rx_bytes rx_packets rx_errs rx_drop rx_fifo rx_frame rx_compressed rx_multicast
         *          tx_bytes tx_packets tx_errs tx_drop tx_fifo tx_colls tx_carrier tx_compressed */
        sscanf(line, "%31s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               iface, &r_bytes, &r_packets, &r_errs, &r_drop, &r_fifo, &r_frame, &r_compressed, &r_multicast,
               &t_bytes, &t_packets, &t_errs, &t_drop, &t_fifo, &t_colls, &t_carrier, &t_compressed);

        /* Exclude loopback interface from total calculation */
        if (strcmp(iface, "lo:") != 0) {
            total_rx += r_bytes;
            total_tx += t_bytes;
        }
    }

    fclose(fp);

    *rx = total_rx;
    *tx = total_tx;
    return 0;
}

/**
 * Initializes network statistics monitoring.
 * Captures baseline byte counts and sets initial timestamp.
 * Must be called before any other network_stats functions.
 *
 * @sideeffect Sets initialized flag to 1.
 * @sideeffect Reads initial byte counts from /proc/net/dev.
 */
void network_stats_init(void)
{
    read_network_stats(&stats.rx_bytes, &stats.tx_bytes);
    stats.upload_speed = 0;
    stats.download_speed = 0;
    stats.last_update = time(NULL);
    initialized = 1;
}

/**
 * Updates network statistics and calculates current speeds.
 * Should be called periodically (e.g., every second) for accurate speed measurements.
 *
 * @sideeffect Updates rx_bytes, tx_bytes, last_update, upload_speed, and download_speed.
 * @sideeffect Reads /proc/net/dev to obtain new byte counts.
 */
void network_stats_update(void)
{
    if (!initialized) return;

    unsigned long long new_rx, new_tx;
    if (read_network_stats(&new_rx, &new_tx) == 0) {
        time_t now = time(NULL);
        double elapsed = difftime(now, stats.last_update);

        /* Calculate speeds in bytes per second if elapsed time is positive */
        if (elapsed > 0 && stats.last_update > 0) {
            stats.download_speed = (new_rx - stats.rx_bytes) / elapsed;
            stats.upload_speed = (new_tx - stats.tx_bytes) / elapsed;
        }

        stats.rx_bytes = new_rx;
        stats.tx_bytes = new_tx;
        stats.last_update = now;
    }
}

/**
 * Retrieves current upload speed.
 *
 * @return Upload speed in kilobytes per second (KB/s).
 *         Returns 0 if not initialized.
 */
double network_stats_get_upload(void)
{
    return stats.upload_speed / 1024.0; /* Convert bytes to KB */
}

/**
 * Retrieves current download speed.
 *
 * @return Download speed in kilobytes per second (KB/s).
 *         Returns 0 if not initialized.
 */
double network_stats_get_download(void)
{
    return stats.download_speed / 1024.0; /* Convert bytes to KB */
}

/**
 * Cleans up network statistics monitoring.
 * Resets initialization flag.
 */
void network_stats_cleanup(void)
{
    initialized = 0;
}