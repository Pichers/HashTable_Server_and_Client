#include "stats.h"
#include <stdio.h>

struct stats_t stats_t_init() {
    struct stats_t stats;
    stats.total_operations = 0;
    stats.total_time = 0;
    stats.connected_clients = 0;
    return stats;
}

void increment_operations(struct stats_t *stats) {
    if (stats) {
        stats->total_operations++;
    }
}

void update_time(struct stats_t *stats, struct timeval start, struct timeval end) {
    if (stats) {
        stats->total_time += (end.tv_usec - start.tv_usec) + (end.tv_sec * 1000000 - start.tv_sec * 1000000);
    }
}

void connected_clients(struct stats_t *stats, int n) {

    if (stats) {
        if(n < 0){
            stats->connected_clients--;
        } else {
            stats->connected_clients++;
        }
    }
}

int copyStats(struct stats_t *stats, struct stats_t *copy) {
    if (stats && copy) {
        copy->total_operations = stats->total_operations;
        copy->total_time = stats->total_time;
        copy->connected_clients = stats->connected_clients;
        return 0;
    }
    return -1;
}
