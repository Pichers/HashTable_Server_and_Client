#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../include/table.h"
#include "../sdmessage.pb-c.h"

/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port){
    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Erro ao criar socket");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("Erro ao vincular socket");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 0) < 0){
        perror("Erro ao colocar-se à escuta");
        close(sockfd);
        return -1;
    }

    return sockfd;
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
int network_main_loop(int listening_socket, struct table_t *table){
    if(listening_socket == -1 || table == NULL)
        return -1;

    int client_socket;
    struct sockaddr_in client;
    socklen_t size = sizeof(client);

    if ((client_socket = accept(listening_socket, (struct sockaddr *)&client, &size)) < 0){
        perror("Erro ao aceitar conexão");
        close(listening_socket);
        return -1;
    }

    MessageT *msg = network_receive(client_socket);
    if(msg == NULL){
        perror("Erro ao receber mensagem");
        close(listening_socket);
        close(client_socket);
        return -1;
    }

    int response = invoke(table, msg);
    if(response == -1){
        perror("Erro ao invocar função");
        close(listening_socket);
        close(client_socket);
        return -1;
    }

    if(network_send(client_socket, msg) == -1){
        perror("Erro ao enviar mensagem");
        close(listening_socket);
        close(client_socket);
        return -1;
    }

    close(listening_socket);
    close(client_socket);
}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket){
    if(client_socket == -1)
        return NULL;

    char *buffer = malloc(sizeof(char) * 1024);
    int bytes_read = 0;
    int total_bytes_read = 0;

    while((bytes_read = read(client_socket, buffer + total_bytes_read, 1024)) > 0){
        total_bytes_read += bytes_read;
        buffer = realloc(buffer, sizeof(char) * (total_bytes_read + 1024));
    }

    if(bytes_read == -1){
        perror("Erro ao ler mensagem");
        free(buffer);
        return NULL;
    }

    MessageT *msg = message_t__unpack(NULL, total_bytes_read, buffer);
    free(buffer);
    return msg;
}

/* A função network_send() deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Enviar a mensagem serializada, através do client_socket.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_send(int client_socket, MessageT *msg){
    if(client_socket == -1 || msg == NULL)
        return -1;

    int msg_size = message_t__get_packed_size(msg);
    char *buffer = malloc(sizeof(char) * msg_size);
    message_t__pack(msg, buffer);

    if(write(client_socket, buffer, msg_size) == -1){
        perror("Erro ao enviar mensagem");
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
    if(socket == -1)
        return -1;

    if(close(socket) == -1){
        perror("Erro ao fechar socket");
        return -1;
    }

    return 0;
}
