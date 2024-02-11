#include "stats.h"
#include <stdio.h>

/**
 * Initializes a stats_t structure to be used by a server and returns it.
 * This stats_t structure contains:
 * - total_operations: Number of total operartions.
 * - total_time: Integer representing the number of time passed (usually miliseconds).
 * - connected_clients: Number of connected clients
*/
struct stats_t stats_t_init() {
    struct stats_t stats;
    stats.total_operations = 0;
    stats.total_time = 0;
    stats.connected_clients = 0;
    return stats;
}

/**
 * Increment by 1 the total number of operations in stats.
*/
void increment_operations(struct stats_t *stats) {
    if (stats) {
        stats->total_operations++;
    }
}

/**
 * Increment by v the total number of operations in stats.
*/
void increment_operations_by(struct stats_t *stats, int v) {
    if (stats) {
        stats->total_operations = stats->total_operations + v;
    }
}

/**
 * Updates the time in stats by the given interval
 * calculated with end - start
*/
void update_time(struct stats_t *stats, struct timeval start, struct timeval end) {
    if (stats) {
        stats->total_time += (end.tv_usec - start.tv_usec) + (end.tv_sec * 1000000 - start.tv_sec * 1000000);
    }
}

/**
 * Increments or decrements the number of connected clients.
 * Increments by 1 if n is positive,
 * and decrements by 1 otherwise.
*/
void connected_clients(struct stats_t *stats, int n) {

    if (stats) {
        if(n < 0){
            stats->connected_clients--;
        } else {
            stats->connected_clients++;
        }
    }
}

/**
 * Copies the stats values to the copy
*/
int copyStats(struct stats_t *stats, struct stats_t *copy) {
    if (stats && copy) {
        copy->total_operations = stats->total_operations;
        copy->total_time = stats->total_time;
        copy->connected_clients = stats->connected_clients;
        return 0;
    }
    return -1;
}
