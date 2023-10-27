#ifndef _NETWORK_CLIENT_H
#define _NETWORK_CLIENT_H

#include "client_stub.h"
#include "sdmessage.pb-c.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rtable;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtable;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtable_t *rtable){
    struct sockaddr_in server;
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Erro ao criar socket");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(rtable->port);
    server.sin_addr.s_addr = inet_addr(rtable->ip);

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("Erro ao conectar-se ao servidor");
        return -1;
    }

    rtable->socket = sockfd;
    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtable_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Tratar de forma apropriada erros de comunicação;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg){
    if(rtable == NULL || msg == NULL)
        return NULL;
    
    int sockfd = rtable->socket;
    int msg_size = message_t__get_packed_size(msg);
    void *buf = malloc(msg_size);
    message_t__pack(msg, buf);

    if (write(sockfd, buf, msg_size) < 0){
        perror("Erro ao enviar mensagem");
        return NULL;
    }

    MessageT *msg_resposta = malloc(sizeof(MessageT));
    if (read(sockfd, msg_resposta, sizeof(MessageT)) < 0){
        perror("Erro ao receber mensagem");
        return NULL;
    }

    return msg_resposta;
}

/* Fecha a ligação estabelecida por network_connect().
 * Retorna 0 (OK) ou -1 (erro).
 */
int network_close(struct rtable_t *rtable){
    if(rtable == NULL)
        return -1;
    
    close(rtable->socket);
    return 0;
}

#endif