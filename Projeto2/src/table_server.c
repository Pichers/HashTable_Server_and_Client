#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table_skel.h"
#include "network_server.h"

int main(int argc, char const *argv[]){
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <port> <n_lists>\n", argv[0]);
        exit(1);
    }

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

    //chamar o main_loop
    int loopI = network_main_loop(skt, table);
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


