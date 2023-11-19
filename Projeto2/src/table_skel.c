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
void sync(aquire){
    if(aquire)
        pthread_mutex_lock(&mux);

    while(counter <= 0){
        pthread_cond_wait(&cond, &mux);
    }
    if(aquire)
        counter--;
}

//releases the lock
void release(){
    counter++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mux);
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

    switch((int) opCode) {
        case (int) MESSAGE_T__OPCODE__OP_PUT:

            gettimeofday(&start_time, NULL);

            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            struct data_t* data = data_create(msg->entry->value.len, msg->entry->value.data);
            struct entry_t* entry = entry_create(strdup(msg->entry->key), data_dup(data));
            char* key = strdup(msg->key);

            //aquire lock
            sync(1);

            //critical section
            int i = table_put(table, entry->key, entry->value);

            increment_operations(stats); 
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);

            release();
            //lock released

            if(i == -1){
                free(data);
                entry_destroy(entry);
                handleError(msg);
            }
            free(data);
            entry_destroy(entry); 
            
            break;
        case (int) MESSAGE_T__OPCODE__OP_GET:

            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            key = msg->key;

            //wait for permission to read
            sync(0);
            //critical section
            struct data_t *dataValue = table_get(table, key);

            if(dataValue == NULL){
                printf("Erro ao obter elemento da tabela\n");
                handleError(msg);

            } else {
                msg->value.data = (uint8_t *)dataValue->data;
                msg->value.len = dataValue->datasize;

                free(dataValue);
            }
            //nao tem syncronização
            increment_operations(stats); 
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);
            ////////////////////////////////
            break;
        case (int) MESSAGE_T__OPCODE__OP_DEL:
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            key = msg->key;

            //aquire lock
            sync(1);
            //critical section
            if(table_remove(table, key) == -1){
                handleError(msg);
            }
            increment_operations(stats);   
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);

            release();
            //lock released

            break;
        case (int) MESSAGE_T__OPCODE__OP_SIZE:
            
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;


            //wait for permission to read
            sync(0);
            //critical section
            int size = table_size(table);

            if(size == -1){
                handleError(msg);
            }
            msg->result = size;
            
            //nao tem sincronização
            increment_operations(stats); 
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);
            ////////////////////////////////
            break;
        case (int) MESSAGE_T__OPCODE__OP_GETKEYS:
            
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;


            //wait for permission to read
            sync(0);
            //critical section
            char** keys = table_get_keys(table);

            if(keys == NULL){
                handleError(msg);
            } else {
                int j = 0;
                while(keys[j] != NULL){
                    j++;
                }                      

                msg->n_keys = j; 
                msg->keys = malloc(j * sizeof(char*));
                msg->keys = keys;
            }
            //add lock
            increment_operations(stats); 
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);
            //

            break;

        case (int) MESSAGE_T__OPCODE__OP_STATS:
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
            if(stats == NULL){
                return handleError(msg);
            }


            StatsT* stats_msg;
            stats_msg = malloc(sizeof(StatsT));
            stats_t__init(stats_msg);

            //nao trata de concorrencia
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);

            
            stats_msg->total_operations = stats->total_operations;
            stats_msg->total_time = stats->total_time;
            stats_msg->connected_clients = stats->connected_clients;

            msg->stats = stats_msg;
            
            // increment_operations(stats); STATS NAO AUMENTA

            break;

        case (int) MESSAGE_T__OPCODE__OP_GETTABLE:
            
            gettimeofday(&start_time, NULL);
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            //wait for permission to read
            sync(0);
            //critical section
            keys = table_get_keys(table);
            
            if(keys == NULL){
                handleError(msg);
            }

            size_t nKeys = 0;

            while(keys[nKeys] != NULL){
                nKeys++;
            }
            msg->n_entries = nKeys;
            msg->entries = (EntryT **) malloc((nKeys+1) * sizeof(struct EntryT *));

            EntryT** entries = malloc((nKeys) * sizeof(EntryT));
            
            if(msg->entries == NULL){
                handleError(msg);
            }

            for(int j = 0; j < nKeys; j++){

                EntryT* entry_temp = malloc(sizeof(EntryT));
                entry_t__init(entry_temp);

                char* key = keys[j];
                struct data_t* data = data_dup(table_get(table, keys[j]));
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
            }

            msg->n_entries = nKeys;
            EntryT** msg_entries = (EntryT**) entries;
            msg->entries = msg_entries;

            //falta locks também
            increment_operations(stats); 
            gettimeofday(&end_time, NULL);
            update_time(stats, start_time, end_time);
            
            break;
        default: // Case BAD || ERROR
            return handleError(msg);
    }
    return 0;
}
