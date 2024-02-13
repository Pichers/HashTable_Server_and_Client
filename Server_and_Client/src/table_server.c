#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "table_skel.h"
#include "network_server.h"
#include "stats.h"

int skt;
struct table_t* table;

/**
 * Shuts down the server, releasing all used resources.
 * Return -1 in case of error, and 0 (OK) otherwise.
*/
void server_shutdown(){

    if(network_server_close(skt) == -1)
        exit(1);
    if(table_skel_destroy(table) == -1)
        exit(1);
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

    //initializethe table with n_lists lists
    table = table_skel_init(n_lists);
    if(table == NULL) {
        printf("Error creating table");
        exit(1);
    }

    //initialize the socket
    skt = network_server_init(port, (char *) argv[3]);
    if(skt == -1){
        printf("Error binding to port");
        table_skel_destroy(table);
        exit(1);
    }

    //Set the table to the one in the server chain
    if(setTable(table) == -1){
        printf("Error setting table");
        table_skel_destroy(table);
        network_server_close(skt);
        exit(1);
    }

    //Initialize stats structure
    struct stats_t stats = stats_t_init();
    struct stats_t *stats_ptr = &stats;

    //Call the main_loop
    if(network_main_loop(skt, table, stats_ptr) == -1){
        table_skel_destroy(table);
        printf("Error in main loop");
        exit(1);
    }

    server_shutdown();

    return 0;
}
