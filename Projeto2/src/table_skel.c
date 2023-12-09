#include <stddef.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "table_skel.h"
#include "entry.h"
#include "table.h"
#include "sdmessage.pb-c.h"
#include "stats.h"
#include "mutex-private.h"
#include <zookeeper/zookeeper.h>



struct table_t *table;
struct statistics *stats;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;



struct rtable_t *backup = NULL;
char *IPB;
char *IPP;
int server;


zhandle_t *zh;
int is_connected;
char *zoo_path = "/test";
typedef struct String_vector zoo_string;
static char *watcher_ctx = "ZooKeeper Data Watcher";

void my_watcher_func(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT){
        if (state == ZOO_CONNECTED_STATE){
            is_connected = 1;
        }
        else {
            is_connected = 0;
        }
    }
}


static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    if (type == ZOO_CHILD_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            // React to child node changes in the znode
            // For example:
            // - Fetch updated child nodes
            // - Update internal data structures or perform actions based on changes
            // - Handle these changes in the context of your table_skel

            // Example:
            // Fetch the updated list of child nodes
            zoo_string children_list;
            int ret = zoo_wget_children(wzh, zpath, child_watcher, watcher_ctx, &children_list);
            if (ret != ZOK) {
                fprintf(stderr, "Error setting watch at %s!\n", zpath);
                // Handle the error
            } else {
                // Process the updated children list
                // Example: Update internal data structures based on the changes
            }
        } else {
            // Handle other states if necessary
        }
    }
    // Add further handling based on the type and state if needed
}


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
//             // TODO
//             switch (children_list->count)
//             {
//             case 1:
//                 if (ZNONODE != zoo_exists(zh, "/kvstore/primary", 0, NULL))
//                 {
//                     if (backup != NULL){
//                     //     if ((rtable_disconnect(backup)) < 0)
//                     //     {
//                     //         exit(-1);
//                     //     }
//                         backup = NULL;
//                     }
//                     IPB = NULL;
//                     // meter a null caso antes houvesse um backup;
//                 }
//                 else
//                 {
//                     if (backup != NULL)
//                     {
//                         // if ((rtable_disconnect(backup)) < 0)
//                         // {
//                         //     exit(-1);
//                         // }
//                         backup = NULL;
//                     }
//                     IPB = NULL;
//                     int backupIPLen = 256;
//                     char backupIP[256] = "";
//                     if (ZOK != zoo_get(zh, "/kvstore/backup", 0, backupIP, &backupIPLen, NULL))
//                     {
//                         printf("ERRO A IR BUSCAR DATA BACKUP 1 FILHO\n");
//                         exit(-1);
//                     }
//                     IPP = NULL;
//                     if (ZOK != zoo_delete(zh, "/kvstore/backup", -1))
//                     {
//                         printf("DELETE DO BACKUP\n");
//                         exit(-1);
//                     }
//                     char *watch_prim_path = "/kvstore/primary";
//                     if (ZOK != zoo_create(zh, watch_prim_path, backupIP, strlen(backupIP), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
//                     {
//                         fprintf(stderr, "Error creating znode from path %s!\n", watch_prim_path);
//                         exit(-1);
//                     }
//                     server = 1;
//                 }
//                 break;
//             case 2:
//                 if (server == 1)
//                 {
//                     if (backup == NULL)
//                     {
//                         int backupIPLen = 256;
//                         char backupIP[256] = "";
//                         if (ZOK != zoo_get(zh, "/kvstore/backup", 0, backupIP, &backupIPLen, NULL))
//                         {
//                             printf("ERRO A IR BUSCAR DATA BACKUP 2 FILHOS\n");
//                             exit(-1);
//                         }
//                         // if ((backup = rtable_connect(backupIP)) == NULL)
//                         // {
//                         //     exit(-1);
//                         // }
//                         char **keys = table_get_keys(table);

//                         int i = 0;
//                         while (keys[i] != NULL)
//                         {
//                             struct data_t *data = table_get(table, keys[i]);
//                             // rtable_put(backup, entry_create(keys[i], data));
//                             i++;
//                         }
//                         IPB = backupIP;
//                     }
//                 }
//                 else
//                 {
//                     int primaryIPLen = 256;
//                     char primaryIP[256] = "";
//                     if (ZOK != zoo_get(zh, "/kvstore/primary", 0, primaryIP, &primaryIPLen, NULL))
//                     {
//                         printf("ERRO A IR BUSCAR DATA BACKUP 2 FILHOS\n");
//                         exit(-1);
//                     }
//                     IPP = primaryIP;
//                 }
//                 break;
//             default:
//                 // DO NOTHING
//                 break;
//                 free(children_list);
//             }
//         }
//     }
// }













/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct table_t *table_skel_init(int n_lists){
    struct table_t* table = table_create(n_lists);

    if (table == NULL)
        return NULL;

    return table;
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos 
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_skel_destroy(struct table_t *table){

    return table_destroy(table);
}

//sets the msg values to those of an error message
int handleError(MessageT* msg){
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;

    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    return -1;
}

//aquires the lock if needed, and waits for the counter to be greater than 0
void lock_sync(int table, int aquire){
    if(table){
        if(aquire)
            pthread_mutex_lock(&table_mux);
        while(table_counter <= 0)
            pthread_cond_wait(&table_cond, &table_mux);
        if(aquire)
            table_counter--;
    } else {
        if(aquire)
            pthread_mutex_lock(&stats_mux);
        while(stats_counter <= 0)
            pthread_cond_wait(&stats_cond, &stats_mux);
        if(aquire)
            stats_counter--;
    }
}

//releases the lock
void release(int table){
    if(table){
        table_counter++;
        pthread_cond_signal(&table_cond);
        pthread_mutex_unlock(&table_mux);
    } else {
        stats_counter++;
        pthread_cond_signal(&stats_cond);
        pthread_mutex_unlock(&stats_mux);
    }
}


/* Executa na tabela table a operação indicada pelo opcode contido em msg 
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
*/
int invoke(MessageT *msg, struct table_t *table, struct stats_t *stats){
    struct timeval start_time, end_time;

    if(table == NULL || msg == NULL)
        handleError(msg);

    MessageT__Opcode opCode = msg->opcode;

    char** keys;

    switch((int) opCode) {
        case (int) MESSAGE_T__OPCODE__OP_PUT:

            gettimeofday(&start_time, NULL);

            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            struct data_t* data = data_create(msg->entry->value.len, msg->entry->value.data);
            struct entry_t* entry = entry_create(strdup(msg->entry->key), data_dup(data));
            char* key = strdup(msg->key);

            //aquire table lock
            lock_sync(1, 1);
            //critical section
            int i = table_put(table, entry->key, entry->value);
            //release table lock
            release(1);

            gettimeofday(&end_time, NULL);
            //aquire stats lock
            lock_sync(0, 1);
            //critical section
            increment_operations(stats); 
            update_time(stats, start_time, end_time);
            //release stats lock
            release(0);

            if(i == -1){
                handleError(msg);
            }
            free(data);
            entry_destroy(entry); 
            free(key);
            break;
        case (int) MESSAGE_T__OPCODE__OP_GET:

            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            key = msg->key;

            //wait for table permission to read
            lock_sync(1, 0);
            //critical section
            struct data_t *dataValue = table_get(table, key);

            gettimeofday(&end_time, NULL);
            //aquire stats lock
            increment_operations(stats); 
            update_time(stats, start_time, end_time);
            //release stats lock
            release(0);

            if(dataValue == NULL){
                printf("Erro ao obter elemento da tabela\n");
                handleError(msg);

            } else {
                msg->value.data = (uint8_t *)dataValue->data;
                msg->value.len = dataValue->datasize;

                free(dataValue);
            }
            break;
        case (int) MESSAGE_T__OPCODE__OP_DEL:
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            key = msg->key;

            //aquire table lock
            lock_sync(1, 1);
            //critical section

            int tr = table_remove(table, key);
            if(tr == -1){
                handleError(msg);
            }
            if(tr == 1){
                msg->opcode = MESSAGE_T__OPCODE__OP_BAD;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
            //release table lock
            release(1);

            gettimeofday(&end_time, NULL);
            //aquire stats lock
            lock_sync(0, 1);
            //critical section
            increment_operations(stats);   
            update_time(stats, start_time, end_time);
            //release stats lock
            release(0);

            break;
        case (int) MESSAGE_T__OPCODE__OP_SIZE:
            
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;


            //wait for table permission to read
            lock_sync(1, 0);
            //critical section
            int size = table_size(table);

            gettimeofday(&end_time, NULL);
            //aquire stats lock
            lock_sync(0, 1);
            //critical section
            increment_operations(stats);   
            update_time(stats, start_time, end_time);
            //release stats lock
            release(0);

            if(size == -1){
                handleError(msg);
            }
            msg->result = size;
            
            break;
        case (int) MESSAGE_T__OPCODE__OP_GETKEYS:
            
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;


            //wait for table permission to read
            lock_sync(1, 0);
            //critical section
            keys = table_get_keys(table);

            gettimeofday(&end_time, NULL);
            //aquire stats lock
            lock_sync(0, 1);
            //critical section
            increment_operations(stats);   
            update_time(stats, start_time, end_time);
            //release stats lock
            release(0);

            if(keys == NULL){
                handleError(msg);
            } else {
                int j = 0;
                while(keys[j] != NULL){
                    j++;
                }                      

                msg->n_keys = j; 
                msg->keys = keys;
            }

            break;

        case (int) MESSAGE_T__OPCODE__OP_STATS:

            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
            if(stats == NULL){
                return handleError(msg);
            }

            StatsT* stats_msg;
            stats_msg = malloc(sizeof(StatsT));
            stats_t__init(stats_msg);
            
            //wait for stats permission to read
            lock_sync(0, 0);
            //critical section
            stats_msg->total_operations = stats->total_operations;
            stats_msg->total_time = stats->total_time;
            stats_msg->connected_clients = stats->connected_clients;

            msg->stats = stats_msg;

            break;

        case (int) MESSAGE_T__OPCODE__OP_GETTABLE:
            
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            //wait for table permission to read
            lock_sync(1, 0);
            //critical section
            keys = table_get_keys(table);

            gettimeofday(&end_time, NULL);
            //aquire stats lock
            lock_sync(0, 1);
            //critical section
            increment_operations(stats);   
            update_time(stats, start_time, end_time);
            //release stats lock
            release(0);
            
            if(keys == NULL){
                handleError(msg);
            }

            size_t nKeys = 0;

            while(keys[nKeys] != NULL){
                nKeys++;
            }
            msg->n_entries = nKeys;
            EntryT ** temp_Entries = malloc((nKeys+1) * sizeof(struct EntryT *));
            msg->entries = temp_Entries;

            EntryT** entries = malloc((nKeys) * sizeof(EntryT));
            
            if(msg->entries == NULL){
                handleError(msg);
            }

            for(int j = 0; j < nKeys; j++){

                EntryT* entry_temp = malloc(sizeof(EntryT));
                entry_t__init(entry_temp);

                char* key = keys[j];
                struct data_t* data = table_get(table, keys[j]);
                if(data == NULL){
                    handleError(msg);
                }

                char* dupKey = strdup(key);
                if(dupKey == NULL){
                    handleError(msg);
                }

                entry_temp->key = dupKey;
                entry_temp->value.len = data->datasize;
                entry_temp->value.data = data->data;

                entries[j] = entry_temp;

                free(data);
            }

            table_free_keys(keys);

            msg->n_entries = nKeys;
            EntryT** msg_entries = (EntryT**) entries;
            msg->entries = msg_entries;
            free(temp_Entries);
            break;
        default: // Case BAD || ERROR
            return handleError(msg);
    }
    return 0;
}
