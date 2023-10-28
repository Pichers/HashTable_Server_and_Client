#include "../include/table.h"
#include "../sdmessage.pb-c.h"
#include <arpa/inet.h>

int const MAX_MSG = 32767;

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
    server.sin_port = htons(atoi(port)); /* port é a porta TCP */
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Faz bind
    if (bind(skt, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Erro ao fazer bind");
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

    int client_socket;
    struct sockaddr_in client;

    socklen_t size_client;
    MessageT *msg;
    
    // Faz listen
    if (listen(listening_socket, 0) < 0){
        perror("Erro ao executar listen");
        close(listening_socket);
        return -1;
    };
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

        close(client_socket);
    }
    close(listening_socket);
}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket){

    char *buffer = malloc(sizeof(char) * 1024);
    int bytes_read = 0;
    int total_bytes_read = 0;

    MessageT* msg;

     
    if(client_socket == -1)
        return NULL;

    while((bytes_read = read(client_socket, buffer + total_bytes_read, 1024)) > 0){
        total_bytes_read += bytes_read;
        buffer = realloc(buffer, sizeof(char) * (total_bytes_read + 1024));
    }

    if(bytes_read == -1){
        perror("Erro ao ler mensagem");
        free(buffer);
        return NULL;
    }

    msg = message_t__unpack(NULL, total_bytes_read, buffer);
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