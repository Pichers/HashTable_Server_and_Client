/*
Grupo 02
Simão Quintas 58190
Manuel Lourenço 58215
Renato Ferreira 58238
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "table.h"
#include "table_skel.h"
#include "network_server.h"
#include "sdmessage.pb-c.h"

int server_socket; 
struct table_t *table; 

void handle_ctrl_c() {
    network_server_close(server_socket);
    table_skel_destroy(table);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <n_lists>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int n_lists = atoi(argv[2]);

    signal(SIGPIPE, SIG_IGN);
    // Set up a Ctrl+C signal handler.
    signal(SIGINT, handle_ctrl_c);
    

    // Initialize the table
    table = table_skel_init(n_lists);
    if (table == NULL) {
        fprintf(stderr, "Error initializing the table.\n");
        exit(1);
    }

    server_socket = network_server_init(port);
    if (server_socket == -1) {
        fprintf(stderr, "Error initializing the socket.\n");
        exit(1);
    }

    network_main_loop(server_socket, table);

    table_skel_destroy(table);

    return 0;
}
