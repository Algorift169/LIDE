// tools/network_stats.h
#ifndef NETWORK_STATS_H
#define NETWORK_STATS_H

void network_stats_init(void);
void network_stats_update(void);
void network_stats_cleanup(void);
double network_stats_get_upload(void);
double network_stats_get_download(void);

#endif