#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "table_skel.h"
#include "network_server.h"
#include "stats.h"

int skt;
struct table_t* table;

void server_shutdown(){
    network_server_close(skt);
    table_skel_destroy(table);
    exit(0);
}

int main(int argc, char const *argv[]){

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <port> <n_lists> <ZooKeeper IP>:<ZooKeeper port>\n", argv[0]);
        exit(1);
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Error setting up signal handler");
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, server_shutdown);

    const char *porta = argv[1];
    int port = atoi(porta);
    int n_lists = atoi(argv[2]);

    //iniciar a tabela com n_lists listas
    table = table_skel_init(n_lists);
    if(table == NULL) {
        printf("Error creating table");
        exit(1);
    }

    //inicializar/associar ao socket
    skt = network_server_init(port, (char *) argv[3]);
    if(skt == -1){
        printf("Error binding to port");
        table_skel_destroy(table);
        exit(1);
    }
    struct stats_t state = stats_t_init();
    struct stats_t *state_ptr = &state;

    //chamar o main_loop
    int loopI = network_main_loop(skt, table, state_ptr);
    if(loopI == -1){
        table_skel_destroy(table);
        printf("Error in main loop");
        exit(1);
    }

    int closeI = network_server_close(skt);
    if(closeI == -1){
        table_skel_destroy(table);
        printf("Error closing socket");
        exit(1);
    }

    int tableDestroyI = table_skel_destroy(table);
    if(tableDestroyI == -1){
        printf("Error destroying table");
        exit(1);
    }

    return 0;
}

struct table_t* get_table(){
    return table;
}


