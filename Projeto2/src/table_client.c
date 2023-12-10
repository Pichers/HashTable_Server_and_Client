#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "client_stub.h"
#include "stats.h"
#include <zookeeper/zookeeper.h>

struct rtable_t* read_rtable;
struct rtable_t* write_rtable;


// #define ZDATALEN 1024 * 1024

// struct rtable_t *rtable = NULL;


// char *nextCommand;
static zhandle_t *zh;
void client_quit();
// static int is_connected;
static char *zoo_path = "/chain";
// static char *watcher_ctx = "ZooKeeper Data Watcher";

typedef struct String_vector zoo_string;

// void my_watcher_func(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx)
// {
//     if (type == ZOO_SESSION_EVENT){
//         if (state == ZOO_CONNECTED_STATE){
//             is_connected = 1;
//         }
//         else {
//             is_connected = 0;
//         }
//     }
// }

// static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
// {
//     zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
//     if (state == ZOO_CONNECTED_STATE)
//     {
//         if (type == ZOO_CHILD_EVENT)
//         {
//             /* Get the updated children and reset the watch */
//             if (ZOK != zoo_wget_children(zh, zoo_path, child_watcher, watcher_ctx, children_list))
//             {
//                 fprintf(stderr, "Error setting watch at %s!\n", zoo_path);
//             }

//             switch (children_list->count)
//             {
//             case 0:
//                 if (rtable != NULL)
//                 {
//                     if (rtable_disconnect(rtable) < 0)
//                     {
//                         exit(-1);
//                     }
//                     rtable = NULL;
//                 }
//                 break;
//             case 1:
//                 if (ZNONODE == zoo_exists(zh, "/kvstore/primary", 0, NULL))
//                 {
//                     int primaryIPLen = 256;
//                     char primaryIP[256] = "";
//                     sleep(1);
//                     if (ZOK != zoo_get(zh, "/kvstore/primary", 0, primaryIP, &primaryIPLen, NULL))
//                     {
//                         printf("ERRO A IR BUSCAR DATA SWITCH 1 FILHO PRIMARY\n");
//                         client_quit();
//                         exit(-1);
//                     }
//                     if ((rtable = rtable_connect(primaryIP)) == NULL)
//                     {
//                         exit(-1);
//                     } // Ligação ao servidor!
//                 }
//                 break;
//             default:
//                 //DO NOTHING
//                 break;
//             }
//             free(children_list);
//         }
//     }
// }

void help() {
        printf("Comandos disponíveis:\n");
        printf("put <key> <data>\n");
        printf("get <key>\n");
        printf("del <key>\n");
        printf("size\n");
        printf("getkeys\n");
        printf("gettable\n");
        printf("stats\n");
        printf("quit\n");
}

void client_quit(){
    if(rtable_disconnect(read_rtable) == -1){
        printf("Erro ao desconectar do servidor\n\n");
    }

    if(rtable_disconnect(write_rtable) == -1){
        printf("Erro ao desconectar do servidor\n\n");
    }
    printf("Bye bye client\n");
    exit(0);
}

void get_read_write_servers(){

    // if (zookeeper_info == NULL)
    //     return;

    // char *zoo_ip = NULL;
    // char *zoo_port = NULL;

    // // Use strtok to split the string at the ':'
    // char *token = strtok(zookeeper_info, ":");
    
    // if (token != NULL) {
    //     zoo_ip = strdup(token); // Duplicate the zoo_ip
    //     token = strtok(NULL, ":");
    //     if (token != NULL) {
    //         zoo_port = token;
    //     } else {
    //         free(zoo_ip); // Free the zoo_ip if port is missing
    //         return;
    //     }
    // } else {
    //     return;
    // }

    //Get the zoo nodes with server info
    //int zoo_get_children(zhandle_t *zh, const char *path, int watch, struct String_vector *children);
    struct String_vector children;

    int ret = zoo_get_children(zh, zoo_path, 0, &children);

    if(ret == ZOK && children.count > 0){
        
        char headChildPath[256];
        snprintf(headChildPath, sizeof(headChildPath), "%s/%s", zoo_path, children.data[0]);

        char tailChildPath[256];
        snprintf(tailChildPath, sizeof(tailChildPath), "%s/%s", zoo_path, children.data[children.count - 1]);

        char tailBuffer[128];
        char headBuffer[128];
        int bufferLen = sizeof(tailBuffer);
        
        zoo_get(zh, tailChildPath, 0, tailBuffer, &bufferLen, NULL);
        zoo_get(zh, headChildPath, 0, headBuffer, &bufferLen, NULL);

        // Conecta ao servidor de escrita
        write_rtable = rtable_connect(headBuffer);
        if (write_rtable == NULL) {
            fprintf(stderr, "Falha ao conectar ao servidor\n");
            exit(1);
        }

        // Conecta ao servidor de leitura
        read_rtable = rtable_connect(tailBuffer);
        if (read_rtable == NULL) {
            fprintf(stderr, "Falha ao conectar ao servidor\n");
            exit(1);
        }
        
        // Free the memory allocated by zoo_get_children
        deallocate_String_vector(&children);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <server_address:port>\n", argv[0]);
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, client_quit);

    zh = zookeeper_init(argv[1], NULL, 10000, 0, 0, 0);

    //função para ir buscar head e tail, e registar as rtables
    void get_read_write_servers();

    char input[256];
    char *token;
    help();
    while (1) {
        printf("Digite um comando: ");

        fgets(input, sizeof(input), stdin);
        token = strtok(input, " \n");

        if (token == NULL) {
            printf("Comando inválido. Tente novamente ou digite help para obter a lista de comandos\n");
            continue;
        }

        if (strcmp(token, "put") == 0) {

            char *key = strtok(NULL, " \n");
            char *data = strtok(NULL, "\n");

            if (key == NULL || data == NULL) {
                printf("Comando put requer <key> e <data>\n\n");
            } else {
                char* tempData = strdup(data);
                struct data_t* datat = data_create(strlen(data), tempData);
                if (datat == NULL) {
                    printf("Erro ao criar data\n\n");
                }
                char* tempKey = strdup(key);

                struct entry_t* entry = entry_create(tempKey, datat);
                if (entry == NULL) {
                    printf("Erro ao criar entry\n\n");
                }

                int a = rtable_put(write_rtable, entry);
                if(a == -1)
                    printf("Erro ao inserir elemento\n\n");
                
                printf("Inserindo elemento: %s\n\n", entry->key);

                entry_destroy(entry);
            }
        } else if (strcmp(token, "get") == 0) {

            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando get requer <key>\n\n");
            } else {
                struct data_t* data = rtable_get(read_rtable, key);
                if (data == NULL) {
                    printf("Elemento nao encontrado, ou erro ao obte-lo\n\n");
                }
                else{
                    char* str = malloc(data->datasize + 1);
                    memcpy(str, data->data, data->datasize);
                    str[data->datasize] = '\0';

                    printf("Elemento encontrado: %s\n\n", str);
                    free(str);
                }
                data_destroy(data);
            }
        } else if (strcmp(token, "del") == 0) {

            char *key = strtok(NULL, " \n");

            if (key == NULL) {
                printf("Comando del requer <key>\n\n");
            } else {
                
                int a = rtable_del(write_rtable, key);
                if(a == -1){
                    printf("Elemento nao encontrado, ou erro ao apaga-lo\n\n");
                }
                else{
                    printf("Elemento com a chave %s apagado\n\n", key);
                }
            }
        } else if (strcmp(token, "size") == 0) {

            int a = rtable_size(read_rtable);
            if(a == -1){
                printf("Erro ao obter tamanho da tabela\n\n");
            }
            else{
                printf("Tamanho da tabela: %d\n\n", a);
            }

        } else if (strcmp(token, "getkeys") == 0) {
            char** keys = rtable_get_keys(read_rtable);

            if(keys == NULL){
                printf("Erro ao obter chaves da tabela\n\n");
            }else if(keys[0] == NULL){
                printf("Tabela vazia\n\n");
                rtable_free_keys(keys);
            }else{
                printf("Chaves da tabela: \n");
                for (int i = 0; keys[i] != NULL; i++){
                    printf("%s \n", keys[i]);
                }
                printf("\n");
                rtable_free_keys(keys);
            }
        } else if (strcmp(token, "gettable") == 0) {
            struct entry_t** entries = rtable_get_table(read_rtable);

            if(entries == NULL){
                printf("Erro ao obter tabela\n\n");
            }else if(entries[0] == NULL){
                printf("Tabela vazia\n\n");
                rtable_free_entries(entries);
            }else{
                printf("Tabela: \n");
                for (int i = 0; entries[i] != NULL; i++){
                    struct entry_t* e = entries[i];

                    char* str = malloc(e->value->datasize + 1);
                    if(str == NULL){
                        printf("Erro ao alocar memoria para entrada\n\n");
                    }
                    memcpy(str, e->value->data, e->value->datasize);
                    str[e->value->datasize] = '\0';


                    printf("%s :: %s \n", entries[i]->key,str);
                    free(str);
                }
                printf("\n");
                rtable_free_entries(entries);
            }
            
        } else if (strcmp(token, "stats") == 0) {
            struct stats_t* stats = rtable_stats(read_rtable);
            if(stats == NULL){
                printf("Erro ao obter estatisticas do servidor\n");
            }else{
                printf("Estatisticas do servidor: \n");
                printf("Numero de operacoes: %d\n", stats->total_operations);
                printf("Clientes atuais: %d\n", stats->connected_clients);
                printf("Tempo total: %d\n\n", stats->total_time);
                
                free(stats);
            }

        } else if (strcmp(token, "quit") == 0) {
            client_quit();
            break;
        } else {
            help();
        }
    }
    exit(0);
    return 0;
}
