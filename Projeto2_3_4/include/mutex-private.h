#ifndef _MUTEX_PRIVATE_H
#define _MUTEX_PRIVATE_H

#include <pthread.h>

extern pthread_mutex_t table_mux;
extern pthread_cond_t table_cond;

extern pthread_mutex_t stats_mux;
extern pthread_cond_t stats_cond;

extern int table_counter;
extern int stats_counter;

#endif // MUTEX-PRIVATE_H