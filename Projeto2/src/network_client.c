#include <stddef.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "../include/client_stub.h"
#include "../include/client_stub-private.h"
#include "../sdmessage.pb-c.h"

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

    int s; 

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(server_port);


    if (inet_aton(server_addr, &myaddr.sin_addr) == 0) {
        perror("Erro ao converter endereço IP");
        return -1;
    }

    s = socket(PF_INET, SOCK_STREAM, 0);
    if (bind(s, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1) {
        perror("Erro ao vincular o socket");
        return -1;
    }


    struct sockaddr server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr = *(struct sockaddr *)&myaddr;

    if (connect(s, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1) {
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
    void *buf = malloc(msg_size);
    message_t__pack(msg, buf);

    if (write(sockfd, buf, msg_size) < 0){
        perror("Erro ao enviar mensagem");
        free(buf);
        return NULL;
    }

    MessageT *msg_resposta = malloc(sizeof(MessageT));
    if (read(sockfd, msg_resposta, sizeof(MessageT)) < 0){
        perror("Erro ao receber mensagem");
        free(buf);
        free(msg_resposta);
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
    
    close(rtable->sockfd);
    return 0;
}
