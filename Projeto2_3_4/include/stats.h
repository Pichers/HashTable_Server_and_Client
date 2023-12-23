#ifndef stats_h
#define stats_h

#include <sys/time.h>

struct stats_t {
    int total_operations;    // Number of total operations executed
    int total_time;          // Total accumulated time spent in operations (in milliseconds)
    int connected_clients;   // Number of currently connected clients
};

// Function to initialize stats structure
struct stats_t stats_t_init();

// Function to increment the number of operations
void increment_operations(struct stats_t *stats);

// Function to update the total time
void update_time(struct stats_t *stats, struct timeval start, struct timeval end);

// Function to increment or decrement the number of connected clients
void connected_clients(struct stats_t *stats, int n);

#endif /* stats_h */