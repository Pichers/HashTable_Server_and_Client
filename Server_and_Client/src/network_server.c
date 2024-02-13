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

//Structure to pass every needed argument to the thread as a single arg
struct thread_args{
    int client_socket;
    struct table_t* table;
    struct stats_t* stats;
};

//List of zookeeper children nodes
struct String_vector* children_list;
//Zookeeper handle
zhandle_t *zh;

int is_connected;
static char *watcher_ctx = "ZooKeeper Data Watcher";
char *zoo_path = "/chain";

struct rtable_t* next_server;
char* node_id;
char serverADDR[120] = "";

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);
void* client_handler(void* arg);


int compare_func(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * Creates the current Zookeeper node and adds it to the chain
*/
void create_current_znode(zhandle_t *zh) {
    
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
        fprintf(stderr, "Error creating znode in ZooKeeper: %s\n", path_buffer);
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

/**
 * Verifies the zookeeper connection state.
 * Is called everytime there is a Zookeeper event
*/
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

/**
 * Gets this server external IP address
*/
char* get_ip_address() {
    char* ip_address = NULL;
    char buffer[1024];
    FILE* fp = popen("hostname -I", "r");
    if (fp == NULL) {
        return NULL;
    }
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char *token = strtok(buffer, " \t\n");
        if (token != NULL) {
            ip_address = strdup(token);
        }
    }
    pclose(fp);
    return ip_address;
}

/**
 * Sets this server's table to the same as the previous server's in the chain
*/
int setTable(struct table_t* table){
    if(children_list == NULL){
        printf("Null children?\n");
        return -1;
    }

    if(children_list->count <= 1 ){
        return 0;
    }

    if (is_connected){

        char prevIP[INET_ADDRSTRLEN + 5];
        int prevIP_len = sizeof(prevIP);

        char prev_server_path[120] = "";
        strcat(prev_server_path, zoo_path);
        strcat(prev_server_path, "/");
        strcat(prev_server_path, children_list->data[children_list->count-2]);

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
        if(entries_size < 0){
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
        if(rtable_disconnect(prevServer) == -1){
            printf("Error disconnecting previous server\n");
        }
    }
    return 0;
}

/**
 * Called whenever a Zookeeper child event is triggered.
 * Updates the list of children, and verifies and updates the current server's connections
*/
void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx){
    if(state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
        if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list)) {
            printf("Error listing children of node 1 %s!\n", zoo_path);
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
        
                    if(zoo_get(zh, nextPath, 0, buf, &bufLen, NULL) == -1){
                        printf("Error getting next server's metadata");
                        return;
                    }


                    if(next_server){
                        if(rtable_disconnect(next_server) == -1){
                            printf("error disconnecting old server");
                            return;
                        }
                        next_server = NULL;
                    }

                    //maybe works
                    next_server = rtable_connect(buf);
                    if(next_server == NULL){
                        printf("error connecting to new server");
                        return;
                    }
                }else if(next_server){
                    if(rtable_disconnect(next_server) == -1){
                        printf("Error disconnecting from next server");
                        return;
                    }
                    next_server = NULL;
                }
            }

        }

        for (int i = 0; i < children_list->count; i++)
            fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);

        fprintf(stderr, "\n=== done ===\n");
    }

}

//initialize mutex-private.h variables
pthread_mutex_t table_mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t table_cond = PTHREAD_COND_INITIALIZER;
int table_counter = 1;

pthread_mutex_t stats_mux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t stats_cond = PTHREAD_COND_INITIALIZER;
int stats_counter = 1;

/**
 * Aquires the stats lock, and waits for cond
*/
void aquire_stats_lock(){
    pthread_mutex_lock(&stats_mux);
    while(stats_counter <= 0)
        pthread_cond_wait(&stats_cond, &stats_mux);
    stats_counter--;
}


/**
 * Releases the stats lock, and signals cond
*/
void release_stats_lock(){
    stats_counter++;
    pthread_cond_signal(&stats_cond);
    pthread_mutex_unlock(&stats_mux);
}

/**
 * Handles synchronization and changes the number of connected clients according to change
*/
void change_connected(int change, struct stats_t* stats){
    //aquire stats lock
    aquire_stats_lock();
    //critical section
    connected_clients(stats, change);
    //release stats lock
    release_stats_lock();
}

/**
 * Function to prepare a socket for receiving connection requests
 * on the given port specified.
 * Returns the socket descriptor or -1 in case of an error.
 */
int network_server_init(short port, char* ZKADDR){
    int skt;

    fflush(stdout);
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

    //Fills the server structure to bind
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port); /*port is the tcp port*/
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(skt, (struct sockaddr *) &server, sizeof(server)) < 0){
        printf("Erro ao fazer bind");
        close(skt);
        return -1;
    };

    if (listen(skt, 0) < 0){
        printf("Erro ao executar listen");
        close(skt);
        return -1;
    };


    //ZOOKEEPER//
    /////////////

    char portaSTR[20];
    sprintf(portaSTR, "%d", port);

    //get this server's external ip address
    char* ip = get_ip_address();

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
        printf("Error listing children of node 2 %s!\n", zoo_path);
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
            printf("Error listing children of node 3 %s!\n", zoo_path);
            return -1;
        }
        qsort(children_list->data, children_list->count, sizeof(char *), compare_func);


        for (int i = 0; i < children_list->count; i++)  {
            fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
        }

        fprintf(stderr, "\n=== done ===\n");
    }

    free(ip);
    return skt;
}

/**
 * The network_main_loop() function should:
 * - Accept a connection from a client;
 * - Receive a message using the network_receive function;
 * - Deliver the deserialized message to the skeleton to be processed in the table 'table';
 * - Wait for the skeleton's response;
 * - Send the response to the client using the network_send function.
 * The function should not return unless an error occurs. In that case, it returns -1.
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
        fflush(stdout);
        
        //Blocks waiting for client connections
        client_socket = accept(listening_socket,(struct sockaddr *) &client, &size_client);

        change_connected(1, stats);

        printf("Conexão de cliente estabelecida\n");
        fflush(stdout);

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

/**
 * Function that handles client called operations, used by the corresponding client's thread
*/
void* client_handler(void* arg){
    struct thread_args *args = (struct thread_args *) arg;

    int client_socket;
    struct table_t* table;
    struct stats_t* stats;

    client_socket = args->client_socket;
    table = args->table;
    stats = args->stats;

    free(arg);

    while (1) {
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

            struct data_t* data = data_create(e->value.len, dataValue);

            char* keyDup = strdup(e->key);
            struct entry_t* entry = entry_create(keyDup, data);

            rtable_put(next_server, entry);

            entry_destroy(entry);

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

    if(table_destroy(table) == -1){
        printf("Error destroying thread table");
    }

    free(arg);

    pthread_exit(NULL);
    return NULL;
}

/**
 * Given a socket and a buffer, receives everything in the socket to the buffer
 */
ssize_t receive_all(int socket, void *buffer, size_t length, int flags) {
    size_t total = 0;  // Total bytes received
    ssize_t n;

    while (total < length) {
        n = recv(socket, (char *)buffer + total, length - total, flags);
        if (n <= 0) {
            // Handle error or connection closed by peer
            return n;
        }
        total += n;
    }

    return total;
}

/**
 * Given a socket and a buffer, sends everything in the buffer to the socket
*/
ssize_t send_all(int socket, const void *buffer, size_t length, int flags) {
    size_t total = 0;  // Total bytes sent
    ssize_t n;

    while (total < length) {
        n = send(socket, (char *)buffer + total, length - total, flags);
        if (n == -1) {
            // Handle error, e.g., print an error message
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

/** 
 * The network_receive() function should:
 * - Read bytes from the network, starting from the indicated 'client_socket';
 * - Deserialize these bytes and construct the message with the request,
 *   allocating the necessary memory for the MessageT structure.
 * Returns the message with the request or NULL in case of an error.
 */
MessageT *network_receive(int client_socket) {
    MessageT *msg;

    // Receive short
    uint16_t msg_size;
    ssize_t nbytes = recv(client_socket, &msg_size, sizeof(uint16_t), 0);

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
    }

    // Unpack the received message
    msg = message_t__unpack(NULL, msg_size2, response_buffer);
    if (msg == NULL) {
        fprintf(stderr, "Error unpacking the request message.\n");
        message_t__free_unpacked(msg, NULL);
        free(response_buffer);
        return NULL;
    }

    free(response_buffer);
    return msg;
}

/**
 * The network_send() function should:
 * - Serialize the response message contained in 'msg';
 * - Send the serialized message through the 'client_socket'.
 * Returns 0 (OK) or -1 in case of an error.
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


/**
 * Frees the resources allocated by network_server_init(), namely
 * by closing the socket passed as an argument.
 * Returns 0 (OK) or -1 in case of an error.
 */
int network_server_close(int socket){
    int ret = 0;
    free(children_list);
    free(node_id);

    if(next_server != NULL){
        if(rtable_disconnect(next_server) == -1){
            printf("Error disconnecting next server");
            ret = -1;
        }
        next_server = NULL;
    }
    if(socket < 0)
        ret = -1;

    if(close(socket) == -1){
        printf("Erro ao fechar socket");
        ret = -1;
    }

    if(zookeeper_close(zh) != ZOK){
        printf("Error closing ZooKeeper");
        ret = -1;
    }

    return ret;
}