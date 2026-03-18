#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glib.h>

// Network stats structure
typedef struct {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    double upload_speed;
    double download_speed;
    time_t last_update;
} NetworkStats;

static NetworkStats stats = {0};
static int initialized = 0;

// Read network stats from /proc/net/dev
static int read_network_stats(unsigned long long *rx, unsigned long long *tx)
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return -1;

    char line[256];
    unsigned long long total_rx = 0, total_tx = 0;

    // Skip first two header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char iface[32];
        unsigned long long r_bytes, r_packets, r_errs, r_drop, r_fifo, r_frame, r_compressed, r_multicast;
        unsigned long long t_bytes, t_packets, t_errs, t_drop, t_fifo, t_colls, t_carrier, t_compressed;

        sscanf(line, "%31s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               iface, &r_bytes, &r_packets, &r_errs, &r_drop, &r_fifo, &r_frame, &r_compressed, &r_multicast,
               &t_bytes, &t_packets, &t_errs, &t_drop, &t_fifo, &t_colls, &t_carrier, &t_compressed);

        // Skip loopback interface
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

// Initialize network stats
void network_stats_init(void)
{
    read_network_stats(&stats.rx_bytes, &stats.tx_bytes);
    stats.upload_speed = 0;
    stats.download_speed = 0;
    stats.last_update = time(NULL);
    initialized = 1;
}

// Update network stats (calculate speeds)
void network_stats_update(void)
{
    if (!initialized) return;

    unsigned long long new_rx, new_tx;
    if (read_network_stats(&new_rx, &new_tx) == 0) {
        time_t now = time(NULL);
        double elapsed = difftime(now, stats.last_update);

        if (elapsed > 0 && stats.last_update > 0) {
            // Calculate speeds in bytes per second
            stats.download_speed = (new_rx - stats.rx_bytes) / elapsed;
            stats.upload_speed = (new_tx - stats.tx_bytes) / elapsed;
        }

        stats.rx_bytes = new_rx;
        stats.tx_bytes = new_tx;
        stats.last_update = now;
    }
}

// Get upload speed (KB/s)
double network_stats_get_upload(void)
{
    return stats.upload_speed / 1024.0; // Convert to KB/s
}

// Get download speed (KB/s)
double network_stats_get_download(void)
{
    return stats.download_speed / 1024.0; // Convert to KB/s
}

// Cleanup network stats
void network_stats_cleanup(void)
{
    initialized = 0;
}