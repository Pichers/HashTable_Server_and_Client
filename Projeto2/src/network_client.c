#include <stddef.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rtable;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtable;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtable_t *rtable) {
    // Criar um socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Erro ao criar o socket");
        return -1;
    }
    int server_port = rtable->server_port;
    const char *server_addr = rtable->server_address;
    struct sockaddr_in myaddr;


    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(server_port);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    myaddr.sin_addr.s_addr = INADDR_ANY;

    if (inet_aton(server_addr, &myaddr.sin_addr) == 0) {
        perror("Erro ao converter endereço IP");
        return -1;
    }

    struct sockaddr server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr = *(struct sockaddr *)&myaddr;

    if (connect(sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1) {
        perror("Erro ao conectar ao servidor");
        close(sockfd);
        return -1;
    }

    rtable->sockfd = sockfd;
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
    
    int sockfd = rtable->sockfd;
    int msg_size = message_t__get_packed_size(msg);


    //send short
    short msg_size_short = msg_size;
    short msg_size_network = htons(msg_size_short);

    if (write(sockfd, &msg_size_network, sizeof(short)) < 0) {
        perror("Error sending message size");
        return NULL;
    }

    //send msg
    void *bufW = malloc(msg_size);
    if (bufW == NULL) {
        perror("??wtf??");
        return NULL;
    }
    message_t__pack(msg, bufW);

    if (write(sockfd, bufW, msg_size) < 0){
        perror("Erro ao enviar mensagem");
        free(bufW);
        return NULL;
    }

    free(bufW);


    //receive short
    short response_size_short;

    if (read(sockfd, &response_size_short, sizeof(short)) < 0) {
        perror("Error receiving response size");
        return NULL;
    }

    int response_size = ntohs(response_size_short);

    if (response_size <= 0) {
        perror("Invalid response size");
        return NULL;
    }

    //receive msg
    void *bufR = malloc(response_size);
    if (bufR == NULL) {
        perror("??wtf??");
        free(bufR);
        return NULL;
    }

    if (read(sockfd, bufR, response_size) < 0){
        perror("Erro ao receber mensagem");
        free(bufR);
        return NULL;
    }

    MessageT msg_resposta = MESSAGE_T__INIT;
    MessageT* response = &msg_resposta;

    response = message_t__unpack(NULL, response_size, bufR);
    
    free(bufR);
    return response;
}

/* Fecha a ligação estabelecida por network_connect().
 * Retorna 0 (OK) ou -1 (erro).
 */
int network_close(struct rtable_t *rtable) {
    if (rtable == NULL || rtable->sockfd == -1) {
        // Invalid rtable or sockfd, nothing to close
        return -1;
    }

    // Close the socket
    int s = rtable->sockfd;
    if (close(s) == -1) {
        printf("Erro ao fechar a ligação");
        return -1;
    }

    // Reset the sockfd in the rtable structure
    rtable->sockfd = -1;

    return 0;
}
