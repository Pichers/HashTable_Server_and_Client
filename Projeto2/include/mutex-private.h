#ifndef _MUTEX_PRIVATE_H
#define _MUTEX_PRIVATE_H

#include <pthread.h>

extern pthread_mutex_t mux;
extern pthread_cond_t cond;
extern int counter;

#endif // MUTEX-PRIVATE_H