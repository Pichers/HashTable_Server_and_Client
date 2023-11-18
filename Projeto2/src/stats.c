#include "stats.h"
#include <stdio.h>

void increment_operations(stats_t *stats) {
    if (stats) {
        stats->total_operations++;
    }
}

void update_time(stats_t *stats, struct timeval start, struct timeval end) {
    if (stats) {
        unsigned long long start_usec = start.tv_sec * 1000000LL + start.tv_usec;
        unsigned long long end_usec = end.tv_sec * 1000000LL + end.tv_usec;
        stats->total_time += (end_usec - start_usec);
    }
}

void connected_clients(stats_t *stats, int n) {

    if (stats) {
        if(n < 0){
            stats->connected_clients--;
        } else {
            stats->connected_clients++;
        }
    }
}
