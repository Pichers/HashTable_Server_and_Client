#ifndef stats_h
#define stats_h

#include <sys/time.h>

struct stats_t {   
    int total_operations; // Number of total operations executed
    int total_time;       // Total accumulated time spent in operations (in microseconds)
    int connected_clients;               // Number of currently connected clients
};

// Funcao que aumenta o # de operacoes

void increment_operations(struct stats_t *stats);

// Funcao que atualiza o tempo total
void update_time(struct stats_t *stats, struct timeval start, struct timeval end);

// Funcao que incrementa o # de clientes conectados, se n for 0 ou positivo, ou decrementa, se n for negativo
void connected_clients(struct stats_t *stats, int n);


#endif /* stats_h */