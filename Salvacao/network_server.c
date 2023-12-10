/*
Grupo 02
Simão Quintas 58190
Manuel Lourenço 58215
Renato Ferreira 58238
*/

#include <stdint.h>
#include <sys/time.h>	
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "stats.h"
#include "table_skel.h"
#include "network_server.h"
#include "mensage-private.h"

struct statistics_t* stats;

int network_server_init(short port) {
    // Create the socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating the server socket");
        return -1;
    }

    // Set the SO_REUSEADDR socket option to allow reusing the port
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Error setting SO_REUSEADDR");
        return -1;
    }

    // Set up the server address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // Use htons to convert the port to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding the socket");
        return -1;
    }

    // Put the server in listening mode
    if (listen(server_socket, 1) == -1) {
        perror("Error putting the server in listening mode");
        exit(1);
    }

    printf("Server is running on port %d.\n", port);

    return server_socket;
}

void *handle_client(void *args) {
    struct timeval start;	
	struct timeval end;	
	unsigned long e_usec;

    struct ThreadArgs* argsAux = (struct ThreadArgs*)args; 
    int sockfd = *(int *)argsAux->client_socket;
    startWrite();
    stats_client_add(argsAux->stats);
    stopWrite();
    MessageT *request;
    while ((request = network_receive(sockfd)) != NULL) {
        gettimeofday(&start, 0);	/* mark the start time */
        if (invoke(request, argsAux->table, argsAux->stats) == -1) {
            printf("Error in processing request\n");
        }
        else if (request->c_type != MESSAGE_T__C_TYPE__CT_STATS){
            gettimeofday(&end, 0);		/* mark the end time */
            e_usec = ((end.tv_sec * 1000000) + end.tv_usec) - ((start.tv_sec * 1000000) + start.tv_usec);
            if (request->c_type != MESSAGE_T__C_TYPE__CT_TABLE){
                startWrite();
                if(stats_add_op(argsAux->stats) == -1){
                    fprintf(stderr, "Error adding operation.\n");
                    break;
                }
            
                if(stats_add_time(argsAux->stats, e_usec) == -1){
                    fprintf(stderr, "Error adding operation time.\n");
                    break;
                }
                stopWrite();
            }
        }

        if (network_send(sockfd, request) == -1) {
            fprintf(stderr, "Error sending response.\n");
            break; // Exit the loop and disconnect if there's an error sending the response.
        }
    }
    printf("Client disconnected.\n");

    startWrite();
    stats_client_remove(argsAux->stats);
    stopWrite();
    
    free(argsAux->client_socket);
    free(argsAux);
    return NULL;
}

/* A função network_main_loop() deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada
     na tabela table;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 * A função não deve retornar, a menos que ocorra algum erro. Nesse
 * caso retorna -1.
 */
int network_main_loop(int listening_socket, struct table_t *table) {
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    stats = stats_create(0, 0, 0);

    while((client_socket = accept(listening_socket, (struct sockaddr *)&client_addr, &client_len)) >= 0) {
        
        struct ThreadArgs* args = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
        args->table = table;
        args->stats = stats;

        printf("Client connected.\n");
        pthread_t thr;
        int *i = malloc(sizeof(int));   //NOTA simplificar
        *i = client_socket;
        args->client_socket = i;
        pthread_create(&thr, NULL, &handle_client, args);
        pthread_detach(thr); /* eliminar zombie threads */
    }
    
    if (client_socket == -1) {
        perror("Error accepting client connection");
        return -1;
    }


}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket){
    MessageT *msg;

    /* Receber um short indicando a dimensão do buffer onde será recebida a resposta; */
    uint16_t msg_size;
    ssize_t nbytes = read_all(client_socket, &msg_size, sizeof(uint16_t)); //implementar read all e write all (aula 5); se for 0, cliente desligou
    if (nbytes != sizeof(uint16_t) && nbytes != 0) {
        perror("Error receiving response size from the client\n");
        return NULL;
    }
    
    int msg_size2 = ntohs(msg_size);

    if(msg_size2 == 0){ //Cliente fechou
        close(client_socket);
        return NULL;
    }
    /* Receber a resposta colocando-a num buffer de dimensão apropriada; */
    uint8_t *response_buffer = (uint8_t *)malloc(msg_size2);
    nbytes = read_all(client_socket, response_buffer, msg_size2);//NOTA read_all
    if (nbytes < 0) {
        perror("Error receiving response from the client\n");
        free(response_buffer);
        return NULL;
    } else {
        msg = message_t__unpack(NULL, msg_size2, response_buffer);
        if (msg == NULL) {
            fprintf(stderr, "Error unpacking the request message.\n");
        }
    }
    free(response_buffer);
    return msg;
}


/* A função network_send() deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Enviar a mensagem serializada, através do client_socket.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_send(int client_socket, MessageT *msg){
    size_t msg_len = message_t__get_packed_size(msg);

    uint8_t *buffer = (uint8_t *)malloc(msg_len);
    message_t__pack(msg, buffer);

    /* Enviar um short (2 bytes) indicando a dimensão do buffer que será enviado; */
    uint16_t msg_size = htons((uint16_t)msg_len);
    ssize_t nbytes = write_all(client_socket, &msg_size, sizeof(uint16_t));
    if (nbytes != sizeof(uint16_t)) {
        perror("Error sending message size to the client");
        free(buffer);
        return -1;
    }

    // Enviar o buffer; 
    nbytes = write_all(client_socket, buffer, msg_len); //NOTA write_all
    if (nbytes != msg_len) {
        perror("Error sending message to the client");
        free(buffer);
        return -1;
    }

    free(buffer);

    message_t__free_unpacked(msg, NULL);
    return 0;
}


/* Liberta os recursos alocados por network_server_init(), nomeadamente
 * fechando o socket passado como argumento.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_server_close(int socket){
    if (close(socket) == -1){
        return -1;
    }
    if(stats)
        free(stats);
    return 0;
}