#ifndef stats_h
#define stats_h

#include <sys/time.h>

struct stats_t {
    int total_operations;    // Number of total operations executed
    int total_time;          // Total accumulated time spent in operations (in milliseconds)
    int connected_clients;   // Number of currently connected clients
};

/* Function to initialize stats structure
*/ 
struct stats_t stats_t_init();

/* Function to increment the number of operations
*/ 
void increment_operations(struct stats_t *stats);

/* Function to increment by v the total number of operations in stats.
*/
void increment_operations_by(struct stats_t *stats, int v);

/* Function to update the total time
*/
void update_time(struct stats_t *stats, struct timeval start, struct timeval end);

/* Function to increment or decrement the number of connected clients
*/
void connected_clients(struct stats_t *stats, int n);

/* Function to copy stats structure from stats to copy
*/
int copyStats(struct stats_t *stats, struct stats_t *copy);

#endif /* stats_h */