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
#include "entry.h"
#include "network_client.h"
#include "stats.h"
#include <zookeeper/zookeeper.h>
#include "client_stub.h"
#include "network_client.h"

struct thread_args{
    int client_socket;
    struct table_t* table;
    struct stats_t* stats;
};

struct String_vector* children_list;
zhandle_t *zh;
int is_connected;
static char *watcher_ctx = "ZooKeeper Data Watcher";
char *zoo_path = "/chain";

struct rtable_t* next_server;
char* node_id;
char* next_node_id;
char serverADDR[120] = "";

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

int compare_func(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void create_current_znode(zhandle_t *zh) {

    // Create the chain if it does not exist
    if (ZNONODE == zoo_exists(zh, zoo_path, 0, NULL)) {
        int length = 1024;
        if(ZNONODE == zoo_create(zh, zoo_path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0 , NULL, length)) {
            fprintf(stderr, "Error creating znode in ZooKeeper: %s\n", zoo_path);
            exit(EXIT_FAILURE);
        }
    }

    char path_buffer[120] = "";
    strcat(path_buffer, zoo_path);
    strcat(path_buffer, "/node");
    int path_buffer_len = sizeof(path_buffer);

    if (ZOK != zoo_create(zh, "/chain/node", serverADDR, strlen(serverADDR) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, path_buffer, path_buffer_len)) {
        printf("Error creating znode in ZooKeeper: %s\n", path_buffer);
        exit(EXIT_FAILURE);
    }
    sleep(3);


    free(children_list);
    children_list =	malloc(sizeof(struct String_vector));
    if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list))  {
        printf("Error listing children of node 2 %s!\n", zoo_path);
        exit(EXIT_FAILURE);
    }
    if(children_list != NULL && children_list->count >= 1){
        qsort(children_list->data, children_list->count, sizeof(char *), compare_func);
    }

    node_id = strdup(path_buffer);
}

void connection(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx){
    if (type == ZOO_SESSION_EVENT){
        if (state == ZOO_CONNECTED_STATE){
            is_connected = 1;
        }
        else {
            is_connected = 0;
        }
    }
}

void getIP (int socket_fd, char *ip_address){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    if (getsockname(socket_fd, (struct sockaddr *)&addr, &addr_size) == 0){
        inet_ntop(AF_INET, &(addr.sin_addr), ip_address, INET_ADDRSTRLEN);
    } else {
        printf("Error getting IP address\n");
    }
}

int setTable(struct table_t* table){
    if(children_list == NULL){
        printf("Null children?\n");
        return -1;
    }

    if(children_list->count <= 1 ){
        printf("There is no previous table to get\n");
        return 0;
    }

    if (is_connected){

        char prev_server_path[120] = "";
        strcat(prev_server_path, zoo_path);
        strcat(prev_server_path, "/");
        strcat(prev_server_path, children_list->data[children_list->count-2]);
        char prevIP[INET_ADDRSTRLEN];
        int prevIP_len = sizeof(prevIP);

        if (ZOK != zoo_get(zh, prev_server_path, 0, prevIP, &prevIP_len, NULL)) {
            printf("Error getting data of node %s!\n", prev_server_path);
            return -1;
        }

        //previne que o servidor se desligue
        signal(SIGPIPE, SIG_IGN);
        
        struct rtable_t *prevServer = rtable_connect(prevIP);
        if (prevServer == NULL) {
            printf("Error connecting to server %s!\n", prevIP);
            return -1;
        }
        struct entry_t** entries = rtable_get_table(prevServer);

        if (entries == NULL) {
            printf("Error getting entries from server %s!\n", prevIP);
            return -1;
        }

        int entries_size = rtable_size(prevServer);
        if(entries_size <= 0){
            return -1;
        }

        for(int i = 0; i < entries_size; i++){
            struct entry_t* e = entries[i];

            if(table_put(table, e->key, e->value) == -1){
                printf("Error putting entry in table!\n");
                return -1;
            }
        }

        rtable_free_entries(entries);
        rtable_disconnect(prevServer);
    }
    return 0;
}



void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx){
    if(state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
        if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list)) {
            printf("Error listing children of node %s!\n", zoo_path);
            return;
        }
        qsort(children_list->data, children_list->count, sizeof(char *), compare_func);

        char iPath[24];
        for(int i = 0; i < children_list->count; i++){
            snprintf(iPath, sizeof(iPath), "%s/%s", zoo_path, children_list->data[i]);

            if(strcmp(iPath, node_id) == 0){
                if(children_list->count > (i + 1)){

                    char buf[24];
                    int bufLen = sizeof(buf);
                    
                    char nextPath[24];
                    snprintf(nextPath, sizeof(iPath), "%s/%s", zoo_path, children_list->data[i + 1]);

                    if(ZOK != zoo_get(zh, nextPath, 0, buf, &bufLen, NULL)){
                        printf("Error getting Zoo meta-data");
                        return;
                    }


                    if(next_server){
                        if(rtable_disconnect(next_server) == -1){
                            printf("error disconnecting old server");
                            return;
                        }
                    }
                    //maybe works
                    next_server = rtable_connect(buf);

                    if(next_server == NULL){
                        printf("error connecting to new server");
                        return;
                    }
                }
            }
        }

        for (int i = 0; i < children_list->count; i++)
            fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);

        fprintf(stderr, "\n=== done ===\n");
    }

}


void* client_handler(void* arg);

//initialize mutex-private.h variables
pthread_mutex_t table_mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t table_cond = PTHREAD_COND_INITIALIZER;
int table_counter = 1;

pthread_mutex_t stats_mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t stats_cond = PTHREAD_COND_INITIALIZER;
int stats_counter = 1;

//aquires the stats lock, and waits for cond
void aquire_stats_lock(){
    pthread_mutex_lock(&stats_mux);
    while(stats_counter <= 0)
        pthread_cond_wait(&stats_cond, &stats_mux);
    stats_counter--;
}

//releases the stats lock, and signals cond
void release_stats_lock(){
    stats_counter++;
    pthread_cond_signal(&stats_cond);
    pthread_mutex_unlock(&stats_mux);
}

//handles synchronization and changes the number of connected clients according to change
void change_connected(int change, struct stats_t* stats){
    
    aquire_stats_lock();
    
    //critical section
    connected_clients(stats, change);

    release_stats_lock();
}

/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port, char* ZKADDR){
    int skt;
    children_list =	malloc(sizeof(struct String_vector));

    if(port < 0){
        printf("Porto inválido");
        return -1;
    }
    
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

    //zookeeper

    char portaSTR[20];
    sprintf(portaSTR, "%d", port);

    char ip[INET_ADDRSTRLEN];
    getIP(skt, ip);

    strcat(serverADDR,ip); 
    strcat(serverADDR,":");
    strcat(serverADDR,portaSTR);
    strcat(serverADDR,"\0");


    zh = zookeeper_init(ZKADDR, connection, 2000, 0, NULL, 0);
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}
    sleep(3);
    create_current_znode(zh);

    if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list))  {
        printf("Error listing children of node %s!\n", zoo_path);
        return -1;
    }

    if(children_list != NULL && children_list->count > 1){
        qsort(children_list->data, children_list->count, sizeof(char *), compare_func);
    }

    if (is_connected) {

        if (children_list == NULL) {
            printf("Error allocating memory for children_list!\n");
            return -1;
        }
        if (ZNONODE == zoo_exists(zh, zoo_path, 0, NULL)) {
            printf("Path %s does not exist!\n", zoo_path);
            return -1;
        } 
        if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list)) {
            printf("Error listing children of node %s!\n", zoo_path);
            return -1;
        }
        qsort(children_list->data, children_list->count, sizeof(char *), compare_func);


        for (int i = 0; i < children_list->count; i++)  {
            fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
        }

        fprintf(stderr, "\n=== done ===\n");
    }
    next_node_id = "";
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
    fflush(stdout);

    while(1){ 
        
        printf("À espera de conexão cliente\n");   

        // Bloqueia a espera de pedidos de conexão
        client_socket = accept(listening_socket,(struct sockaddr *) &client, &size_client);

        change_connected(1, stats);

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
            break;
        }

        if(invoke(msg, table, stats) < 0){
            close(client_socket);
            connected_clients(stats, -1);
            message_t__free_unpacked(msg, NULL);
            break;
        }
        if(msg->opcode == MESSAGE_T__OPCODE__OP_PUT+1){
        
            EntryT* e = msg->entry;

            char* dataValue = malloc(e->value.len + 1);
            if(dataValue == NULL){
                return NULL;
            }


            memcpy(dataValue, e->value.data, e->value.len);
            dataValue[e->value.len] = '\0';

            char* dataValueDup = strdup(dataValue);
            struct data_t* data = data_create(e->value.len, dataValueDup);

            free(dataValue);
        

            char* keyDup = strdup(e->key);
            struct entry_t* entry = entry_create(keyDup, data);

            rtable_put(next_server, entry);


        } else if(msg->opcode == MESSAGE_T__OPCODE__OP_DEL+1){
            rtable_del(next_server, msg->key);
        }
        if(network_send(client_socket, msg) == -1){
            close(client_socket);
            connected_clients(stats, -1);
            message_t__free_unpacked(msg, NULL);
            break;
        }
        message_t__free_unpacked(msg, NULL);
    }
    change_connected(-1, stats);
    
    printf("Conexão com o cliente encerrada. Socket: %d\n", client_socket);
    fflush(stdout);

    close(client_socket);
    free(args);

    pthread_exit(NULL);
    return NULL;
}

ssize_t receive_all(int socket, void *buffer, size_t length, int flags) {
    size_t total = 0;  // Total bytes received
    ssize_t n;

    while (total < length) {
        n = recv(socket, buffer + total, length - total, flags);
        if (n <= 0) {
            // Handle error or connection closed by peer
            printf("AAAAAAAAAAAAAAAAAAAAAAAa");
            return n;
        }
        total += n;
    }

    return total;
}

/* A função network_receive() deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * Retorna a mensagem com o pedido ou NULL em caso de erro.
 */
MessageT *network_receive(int client_socket) {
    MessageT *msg;

    // Receive short
    uint16_t msg_size;
    ssize_t nbytes;

    while (nbytes == 0){
        nbytes = recv(client_socket, &msg_size, sizeof(uint16_t), 0);
    }
    
    if (nbytes != sizeof(uint16_t)) {
        printf("Error receiving response size from the server Tamanho\n");
        return NULL;
    }
    int msg_size2 = ntohs(msg_size);

    // Receive message
    uint8_t *response_buffer = (uint8_t *)malloc(msg_size2);
    if (receive_all(client_socket, response_buffer, msg_size2, 0) < 0) {
        printf("Error receiving response from the server Mensagem\n");
        free(response_buffer);
        return NULL;
    } else { 
        // Unpack the received message
        msg = message_t__unpack(NULL, msg_size2, response_buffer);
        if (msg == NULL) {
            fprintf(stderr, "Error unpacking the request message.\n");
            message_t__free_unpacked(msg, NULL);
            free(response_buffer);
            return NULL;
        }
    }
    free(response_buffer);
    return msg;
}

ssize_t send_all(int socket, const void *buffer, size_t length, int flags) {
    size_t total = 0;  // Total bytes sent
    ssize_t n;

    while (total < length) {
        n = send(socket, (char *)buffer + total, length - total, flags);
        if (n == -1) {
            perror("send_all");
            return -1;
        }
        if (n == 0) {
            // Connection closed by peer
            return total;
        }
        total += n;
    }

    return total;
}

/* A função network_send() deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Enviar a mensagem serializada, através do client_socket.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int network_send(int client_socket, MessageT *msg) {
    if (client_socket == -1)
        return -1;
    if (msg == NULL)
        return -1;

    size_t msg_size = message_t__get_packed_size(msg);
    uint16_t msg_size_network = htons((uint16_t)msg_size);

    uint8_t *buffer = (uint8_t *)malloc(msg_size);
    if (buffer == NULL) {
        printf("Error allocating memory for the buffer\n");
        return -1;
    }

    message_t__pack(msg, buffer);

    if (send_all(client_socket, &msg_size_network, sizeof(uint16_t), 0) < 0) {
        printf("Error sending message size\n");
        free(buffer);
        return -1;
    }

    if (send_all(client_socket, buffer, msg_size, 0) < 0) {
        printf("Error sending message data\n");
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