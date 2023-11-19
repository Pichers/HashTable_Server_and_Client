#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "table.h"
#include "sdmessage.pb-c.h"
#include "network_server.h"
#include "mutex-private.h"
#include "table_skel.h"
#include "stats.h"

struct thread_args{
    int client_socket;
    struct table_t* table;
    struct stats_t* stats;
};

void* client_handler(void* arg);

//initialize mutex-private.h variables
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int counter = 1;

/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port){
    int skt;

    if(port < 0)
        return -1;
    
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        printf("Erro ao criar socket");
        return -1;
    }

    int yes = 1;

    if (setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        printf("Erro ao configurar socket");
        return -1;
    }



    // Preenche estrutura server para bind
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port); /* port é a porta TCP */
    server.sin_addr.s_addr = INADDR_ANY;

    // Faz bind
    if (bind(skt, (struct sockaddr *) &server, sizeof(server)) < 0){
        printf("Erro ao fazer bind");
        close(skt);
        return -1;
    };

    // Faz listen
    if (listen(skt, 0) < 0){
        printf("Erro ao executar listen");
        close(skt);
        return -1;
    };

    return skt;
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
int network_main_loop(int listening_socket, struct table_t *table, struct stats_t *stats){

    if(listening_socket == -1)
        return -1;
    if(table == NULL)
        return -1;

    int client_socket;
    struct sockaddr_in client;

    socklen_t size_client = sizeof(client);
    
    printf("Servidor pronto\n");

    while(1){ 
        printf("À espera de conexão cliente\n");   
        // Bloqueia a espera de pedidos de conexão
        client_socket = accept(listening_socket,(struct sockaddr *) &client, &size_client);
        connected_clients(stats, 1);
        printf("Conexão de cliente estabelecida\n");

        pthread_t thr;

        struct thread_args* targs = malloc(sizeof(struct thread_args));
        if(targs == NULL){
            printf("Error allocating memory for thread args");
            continue;
        }
        targs->client_socket = client_socket;
        targs->table = table;
        targs->stats = stats;

        pthread_create(&thr, NULL, &client_handler, targs);
        pthread_detach(thr);
    }
    //fechar as threads?
    //////////////////////////////////////////////////////////////////
    network_server_close(listening_socket);
    return -1;
}

void* client_handler(void* arg){
    int client_socket;
    struct table_t* table;
    struct stats_t* stats;
    struct thread_args *args;


    while (1) {
        args = (struct thread_args *) arg;

        client_socket = args->client_socket;
        table = args->table;
        stats = args->stats;

        MessageT* msg = network_receive(client_socket);

        if(msg == NULL){
            close(client_socket);
            connected_clients(stats, -1);
            break;
        }
        if(invoke(msg, table, stats) < 0){
            close(client_socket);
            connected_clients(stats, -1);
            break;
        }
        if(network_send(client_socket, msg) == -1){
            close(client_socket);
            connected_clients(stats, -1);
            break;
        }
    }
    printf("Conexão com o cliente encerrada. Socket: %d\n", client_socket);
    fflush(stdout);

    close(client_socket);
    free(args);
    return NULL;
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
    ssize_t nbytes = recv(client_socket, &msg_size, sizeof(uint16_t), 0);
    if (nbytes != sizeof(uint16_t) && nbytes != 0) {
        printf("Error receiving response size from the server\n");
        return NULL;
    }
    int msg_size2 = ntohs(msg_size);

    /* Receber a resposta colocando-a num buffer de dimensão apropriada; */
    uint8_t *response_buffer = (uint8_t *)malloc(msg_size2);
    nbytes = recv(client_socket, response_buffer, msg_size2, 0);
    if (nbytes < 0) {
        printf("Error receiving response from the server\n");
        free(response_buffer);
        return NULL;
    } else {
        msg = message_t__unpack(NULL, msg_size2, response_buffer);
        // if (msg == NULL) {
        //     fprintf(stderr, "Error unpacking the request message.\n");
        // }
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
    if(client_socket == -1)
        return -1;
    if(msg == NULL)
        return -1;


    size_t msg_size = message_t__get_packed_size(msg);
    // size_t msg_size_short = msg_size;
    uint16_t msg_size_network = htons((uint16_t)msg_size);

    uint8_t *buffer = (uint8_t *)malloc(msg_size);
    message_t__pack(msg, (uint8_t*)buffer);


    if (send(client_socket,&msg_size_network, sizeof(uint16_t),0) < 0) {
        printf("Error sending message size");
        free(buffer);
        return -1;
    }
    if (buffer == NULL) {
        printf("??wtf??");
        return -1;
    }

    if (send(client_socket, buffer, msg_size,0) < 0){
        printf("Erro ao enviar mensagem");
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

/* Liberta os recursos alocados por network_server_init(), nomeadamente
 * fechando o socket passado como argumento.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_server_close(int socket){
    if(socket < 0)
        return -1;

    if(close(socket) == -1){
        printf("Erro ao fechar socket");
        return -1;
    }

    return 0;
}