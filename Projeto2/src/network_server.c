#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "table.h"
#include "sdmessage.pb-c.h"
#include "network_server.h"
#include "table_skel.h"

/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port){
    int skt;
    struct sockaddr_in server;

    if(port < 0)
        return -1;
    
    if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("Erro ao criar socket");
        return -1;
    }

    // Preenche estrutura server para bind
    server.sin_family = AF_INET;
    server.sin_port = htons(port); /* port é a porta TCP */
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Faz bind
    if (bind(skt, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Erro ao fazer bind");
        close(skt);
        return -1;
    };

     // Faz listen
    if (listen(skt, 0) < 0){
        perror("Erro ao executar listen");
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
int network_main_loop(int listening_socket, struct table_t *table){

    if(listening_socket == -1)
        return -1;
    if(table == NULL)
        return -1;

    int client_socket;
    struct sockaddr_in client;

    socklen_t size_client;
    MessageT *msg;
    
    printf("Servidor 'a espera de dados\n");

    // Bloqueia a espera de pedidos de conexão
    while ((client_socket = accept(listening_socket,(struct sockaddr *) &client, &size_client)) != -1) {
        
        msg = network_receive(client_socket);

        if(msg == NULL){
            perror("Erro ao de-serializar mensagem");
            close(client_socket);
            close(listening_socket);
            return -1;
        }

        int response = invoke(msg, table);

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

        close(client_socket);
    }
    close(listening_socket);
    return -1;
}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket){


    short response_size_short;

    if(read(client_socket, &response_size_short, sizeof(short)) == -1){
        perror("Erro ao ler tamanho da mensagem");
        return NULL;
    }

    int response_size = ntohs(response_size_short);
    if (response_size <= 0) {
        perror("Invalid response size");
        return NULL;
    }


    char *buffer = malloc(sizeof(char) * response_size);
    if(buffer == NULL){
        perror("Erro ao alocar memória para mensagem");
        return NULL;
    }

    if (read(client_socket, buffer, response_size) < 0) {
        perror("Erro ao receber mensagem");
        free(buffer);
        return NULL;
    }

    MessageT* msg = message_t__unpack(NULL, response_size, (uint8_t*)buffer);

    free(buffer);
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

    short msg_size = message_t__get_packed_size(msg);
    char *buffer = malloc(sizeof(char) * msg_size);
    message_t__pack(msg, (uint8_t*)buffer);


    if (write(client_socket,buffer, sizeof(short)) < 0) {
        perror("Error sending message size");
        free(buffer);
        return -1;
    }
    if (buffer == NULL) {
        perror("??wtf??");
        return -1;
    }

    if (write(client_socket, buffer, msg_size) < 0){
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
    if(socket < 0)
        return -1;

    if(close(socket) == -1){
        perror("Erro ao fechar socket");
        return -1;
    }

    return 0;
}