/*
Grupo 02
Simão Quintas 58190
Manuel Lourenço 58215
Renato Ferreira 58238
*/

#include <stdint.h>
#include <string.h>
#include <sys/time.h>	
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "stats.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include "table_skel.h"
#include "network_server.h"
#include "mensage-private.h"
#include "zookeeper/zookeeper.h"

typedef struct String_vector zoo_string; 
zoo_string* children_list; 
static zhandle_t *zh;
int is_connected;
static char *watcher_ctx = "ZooKeeper Data Watcher";
static char *root_path = "/chain";
struct rtable_t* next_server;
char* node_id;
char* next_node_id;
int server_pos = -1;

struct statistics_t* stats;

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}


// Comparison function for qsort
int compareStrings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}


/**
* Data Watcher function for /MyData node
TODO, adaptar função para o servidor
*/
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {
            /* Get the updated children and reset the watch */ 
            if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list)) {//zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list)
                fprintf(stderr, "Error setting watch at %s!\n", root_path);
            }

            if(children_list->count == 0){
                printf("Nenhum servidor disponivel, a tentar novamente em 5 segundos...\n");
                sleep(5);
            } else {
                //verificar se o next_server é o mesmo
                //ver em que servidor estamos
                qsort(children_list->data, children_list->count, sizeof(char*), compareStrings);
                for(int i = 0; i < children_list->count; i++){
                    char full_path_child[24] = "";
                    strcat(full_path_child,root_path); 
                    strcat(full_path_child,"/"); 
                    strcat(full_path_child,children_list->data[i]); 
                    if(strcmp(node_id, full_path_child) == 0){
                        server_pos = i;
                    }
                    printf("server_pos: %s\n", node_id);
                    printf("server_pos: %s\n", children_list->data[i]);
                }

                printf("\n\nPASSA POR AQUI ANTES DE EXPLUDIR\n\n");
                printf("children_list->count: %d\n", children_list->count);
                printf("server_pos: %d\n", server_pos);
                printf("server_pos + 1 < children_list->count: %d\n", server_pos + 1 < children_list->count);

                //verificar se estamos no mesmo server
                if(server_pos + 1 < children_list->count){

                    //se não estivermos, ligar ao next_server
                    
                    char full_path_next_serv[24] = "";
                    strcat(full_path_next_serv,root_path); 
                    strcat(full_path_next_serv,"/"); 
                    strcat(full_path_next_serv,children_list->data[server_pos + 1]); 
                    printf("ERRO DEVE SER AQUI!\n");
                    printf("next_node_id: %s\n", next_node_id);
                    printf("children_list->data[server_pos+1]: %s\n", children_list->data[server_pos+1]);
                    printf("strcmp(next_node_id, children_list->data[server_pos+1]): %d\n", strcmp(next_node_id, children_list->data[server_pos+1]));

                    if(next_server == NULL || strcmp(next_node_id, children_list->data[server_pos+1]) != 0){   //TODO isto ta mak, é preciso o caminho completo
                        /* Em principio isto não é preciso só acontece se o prox server já não existir
                        if(next_node_id != NULL)
                            rtable_disconnect(next_server);
                        */
                        next_node_id = children_list->data[server_pos+1];
                        char next_server_ip[1024];
                        int next_server_ip_len = sizeof(next_server_ip);
                        if(ZOK != zoo_get(zh,full_path_next_serv, 0, next_server_ip, &next_server_ip_len, NULL)){
                            printf("Erro ao obter data do next_rtable 3\n");
                            return;
                        }   
                        printf("3 LIGAÇÂO AO NEXT SERVER\n");
                        next_server = rtable_connect(next_server_ip);
                        if (next_server == NULL){
                            printf("Erro de ligação à tabela remota next na main");
                            return;
                        }
                    }
                } else {
                    next_server = NULL;
                    next_node_id = "";
                }

                //TODO apagar, print para testes
                fprintf(stderr, "\n=== znode listing === [ %s ]", root_path); 
                for (int i = 0; i < children_list->count; i++)  {
                    fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
                }
                fprintf(stderr, "\n=== done ===\n");
            }
        } 
    }
    
}

void getSocketInfo(int socket_fd, char *ip_address) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getsockname(socket_fd, (struct sockaddr *)&addr, &addr_len) == 0) {
        // Convert the IP address from binary to string form
        inet_ntop(AF_INET, &(addr.sin_addr), ip_address, INET_ADDRSTRLEN);


    } else {
        perror("Error getting socket information");
        exit(EXIT_FAILURE);
    }
}


char* ZK_node_init(char* server_ip_port){
    static char *root_path = "/chain";
    char* new_path;

    // Check if the node "/chain" exists
    if (ZNONODE == zoo_exists(zh, root_path, 0, NULL)) {
        if (ZOK == zoo_create( zh, root_path, NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
            fprintf(stderr, "%s created!\n", root_path);
        } else {
            fprintf(stderr,"Error Creating %s!\n", root_path);
            exit(EXIT_FAILURE);
        } 
    }


    sleep(2); /* Sleep a little for connection to complete */
    if (is_connected) {
        /*
		if (ZNONODE == zoo_exists(zh, root_path, 0, NULL)) {
			fprintf(stderr, "%s doesn't exist! \
				Please start ZChildMaker.\n", root_path);
			exit(EXIT_FAILURE);
		}
        */
		char node_path[120] = "";
		strcat(node_path,root_path); 
		strcat(node_path,"/node"); 
		int new_path_len = 1024;
		new_path = malloc(new_path_len);
		int data_size = strlen(server_ip_port) + 1;
        if(ZOK != zoo_create(zh, node_path, server_ip_port, data_size, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, new_path, new_path_len)) {
            printf("Error creating znode from path %s!\n", node_path);
            exit(EXIT_FAILURE);
        }
        sleep(1);
		
	}

    return new_path;
}

int network_server_init(char* ZKport, short port) {
    
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

    char str_port[20];  // Adjust the array size based on your needs

    // Use sprintf to format the integer as a string
    sprintf(str_port, "%d", port);
    
    char peer_ip[INET_ADDRSTRLEN];
    getSocketInfo(server_socket, peer_ip);

    char server_ip_port[120] = "";
    strcat(server_ip_port,peer_ip); 
    strcat(server_ip_port,":");
    strcat(server_ip_port,str_port);

    /* Connect to ZooKeeper server */
	zh = zookeeper_init(ZKport, connection_watcher, 2000, 0, 0, 0); 
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}
    sleep(3);
    node_id = ZK_node_init(server_ip_port);

    //obter as crianças e fazer watch
    if (is_connected) {
        children_list =	(zoo_string *) malloc(sizeof(zoo_string));
        if (ZNONODE == zoo_exists(zh, root_path, 0, NULL)) {
            printf("Nenhum servidor ligado, após ligar o mesmo\n");
            return -1;
        } 

        if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list)) {
            printf("Error setting watch at %s!\n", root_path);
            return -1;
        }
        for (int i = 0; i < children_list->count; i++)  {
            fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
        }
        fprintf(stderr, "\n=== done ===\n");

        if(children_list == NULL || children_list->count == 0){
            printf("Nenhum servidor disponivel, após ligar o mesmo\n");
            return -1;
        }

        //TODO depois apagar, prints para teste
        fprintf(stderr, "\n=== znode listing === [ %s ]", root_path); 
        for (int i = 0; i < children_list->count; i++)  {
            fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
        }
        fprintf(stderr, "\n=== done ===\n");
        printf("children_list->count: %d\n", children_list->count);
    }

    //ver em que servidor estamos
    // Sort the array using qsort
    qsort(children_list->data, children_list->count, sizeof(char*), compareStrings);
    for(int i = 0; i < children_list->count; i++){
        char full_path_children[24] = "";
        strcat(full_path_children,root_path); 
        strcat(full_path_children,"/"); 
        strcat(full_path_children,children_list->data[i]); 
        printf("node_id %s\n children_list->data[i] %s\n", node_id, full_path_children);
        if(strcmp(node_id, full_path_children) == 0){
            server_pos = i;
        }
    }
    printf("server_pos: %d\n", server_pos);

    //prep ligar ao next_server 
    next_node_id = "";
    next_server = NULL;

    return server_socket;
}

int set_table(struct table_t* table) {

    //copiar a tabela do servidor anterior
    if(server_pos > 0){
        char full_path_prev_serv[24] = "";
        strcat(full_path_prev_serv,root_path); 
        strcat(full_path_prev_serv,"/"); 
        strcat(full_path_prev_serv,children_list->data[server_pos-1]); 
        char prev_server_ip[1024];
        int prev_server_ip_len = sizeof(prev_server_ip);
        if(ZOK != zoo_get(zh,full_path_prev_serv, 0, prev_server_ip, &prev_server_ip_len, NULL)){
            printf("Erro ao obter data do next_rtable 2\n");
            return -1;
        }    //NOTA estou a assumir que a ordem mantem-se

        signal(SIGPIPE, SIG_IGN);

        struct rtable_t* prev_server = rtable_connect(prev_server_ip);
        if (prev_server == NULL){
            printf("Erro de ligação à tabela remota next na main");
            return -1;
        }
        
        struct entry_t** entries = rtable_get_table(prev_server);
        int num_entries = rtable_size(prev_server);
        printf("num_entries: %d\n", num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            if(table_put(table, entries[i]->key, entries[i]->value) == -1){
                printf("Erro na operação put.\n");
                return -1;
            }
        }
        rtable_free_entries(entries);
        rtable_disconnect(prev_server);
    }


    return 0;

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
        } else if (request->c_type != MESSAGE_T__C_TYPE__CT_STATS){
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

                printf("request->opcode: %d\n", request->opcode);
                printf("next_server == NULL: %d \n",next_server == NULL);
                //passar a mensagem para o proximo servidor
                //TODO é isto que está mal
                if(next_server != NULL){
                    if(request->opcode == MESSAGE_T__OPCODE__OP_PUT+1){
                        printf("request->entry->value.data: %s\n", request->entry->value.data);

                        struct entry_t* entry = malloc(sizeof(struct entry_t));

                        // Allocate memory for the deserialized data
                        void* tempData = malloc(request->entry->value.len);
                        if (tempData == NULL) {
                            // Handle memory allocation error
                            free(tempData);
                            message_t__free_unpacked(request, NULL);
                            return NULL;
                        }

                        // Copy the deserialized data from the message
                        memcpy(tempData, request->entry->value.data, request->entry->value.len);

                        // Deserialize the value from the message using Protocol Buffers
                        struct data_t *value = data_create(request->entry->value.len, tempData);//malloc(sizeof(struct data_t));
                        if (value == NULL) {
                            free(tempData);
                            // Handle memory allocation error
                            return NULL;
                        }


                        //Tranformar isto numa entry_t, mas bem
                        entry->key = strdup(request->entry->key);
                        entry->value = value;
                        printf("isto chega aqui?AAAAAAAAAAaa\n");
                        rtable_put(next_server, entry);
                        printf("isto chega aqui?\n");
                    } else if(request->opcode == MESSAGE_T__OPCODE__OP_DEL+1) {
                        rtable_del(next_server, request->key);
                        printf("isto chega aqui? DELETE\n");
                    }
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