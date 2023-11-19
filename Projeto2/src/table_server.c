#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "table_skel.h"
#include "network_server.h"
#include "stats.h"

int main(int argc, char const *argv[]){

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <port> <n_lists>\n", argv[0]);
        exit(1);
    }

    // if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    //     perror("Error setting up signal handler");
    //     return -1;
    // }
    signal(SIGPIPE, SIG_IGN);

    int port = atoi(argv[1]);
    int n_lists = atoi(argv[2]);

    //iniciar a tabela com n_lists listas
    struct table_t* table = table_skel_init(n_lists);
    if (table == NULL) {
        printf("Error creating table");
        exit(1);
    }

    //inicializar/associar ao socket
    int skt = network_server_init(port);
    if(skt == -1){
        printf("Error binding to port");
        exit(1);
    }
    struct stats_t state = stats_t_init();
    struct stats_t *state_ptr = &state;

    //chamar o main_loop
    int loopI = network_main_loop(skt, table, state_ptr);
    if(loopI == -1){
        printf("Error in main loop");
        exit(1);
    }

    int closeI = network_server_close(skt);
    if(closeI == -1){
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


