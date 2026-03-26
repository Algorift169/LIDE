/* tools/network_stats.h */
#ifndef NETWORK_STATS_H
#define NETWORK_STATS_H

/**
 * Initializes network statistics monitoring.
 * Captures baseline byte counts from /proc/net/dev.
 * Must be called before any other functions in this module.
 *
 * @sideeffect Sets up internal state for speed calculations.
 */
void network_stats_init(void);

/**
 * Updates network statistics and calculates current speeds.
 * Should be called periodically (recommended interval: 1 second)
 * to compute accurate upload/download rates.
 *
 * @sideeffect Reads /proc/net/dev and updates internal byte counters.
 * @sideeffect Recalculates upload_speed and download_speed based on delta.
 */
void network_stats_update(void);

/**
 * Cleans up network statistics monitoring.
 * Resets internal state to uninitialized.
 */
void network_stats_cleanup(void);

/**
 * Retrieves current upload speed.
 *
 * @return Upload speed in kilobytes per second (KB/s).
 *         Returns 0.0 if not initialized or no data available.
 */
double network_stats_get_upload(void);

/**
 * Retrieves current download speed.
 *
 * @return Download speed in kilobytes per second (KB/s).
 *         Returns 0.0 if not initialized or no data available.
 */
double network_stats_get_download(void);

#endif /* NETWORK_STATS_H */