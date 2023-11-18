#include <stddef.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "table_skel.h"
#include "entry.h"
#include "table.h"
#include "sdmessage.pb-c.h"
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

int handleError(MessageT* msg){
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;

    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    return -1;
}


/* Executa na tabela table a operação indicada pelo opcode contido em msg 
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
*/
int invoke(MessageT *msg, struct table_t *table){

    if(table == NULL || msg == NULL)
        handleError(msg);

    MessageT__Opcode opCode = msg->opcode;

    switch((int) opCode) {
        case (int) MESSAGE_T__OPCODE__OP_PUT:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            struct data_t* data = data_create(msg->entry->value.len, msg->entry->value.data);
            struct entry_t* entry = entry_create(strdup(msg->entry->key), data_dup(data));
            char* key = strdup(msg->key);

            //aquire lock
            pthread_mutex_lock(&mux);

            while(counter <= 0){
                pthread_cond_wait(&cond, &mux);
            }
            counter--;

            //critical section
            int i = table_put(table, entry->key, entry->value);

            counter++;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mux);
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

            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            key = msg->key;

            while(counter <= 0){
                pthread_cond_wait(&cond, &mux);
            }
            struct data_t *dataValue = table_get(table, key);

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
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            key = msg->key;

            //aquire lock
            pthread_mutex_lock(&mux);

            while(counter <= 0){
                pthread_cond_wait(&cond, &mux);
            }
            counter--;
            //critical section
            if(table_remove(table, key) == -1){
                handleError(msg);
            }

            counter++;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mux);
            //lock released

            break;
        case (int) MESSAGE_T__OPCODE__OP_SIZE:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;


            while(counter <= 0){
                pthread_cond_wait(&cond, &mux);
            }
            int size = table_size(table);

            if(size == -1){
                handleError(msg);
            }
            msg->result = size;

            break;
        case (int) MESSAGE_T__OPCODE__OP_GETKEYS:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;


            while(counter <= 0){
                pthread_cond_wait(&cond, &mux);
            }
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

            break;
        case (int) MESSAGE_T__OPCODE__OP_GETTABLE:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            while(counter <= 0){
                pthread_cond_wait(&cond, &mux);
            }
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

                // struct entry_t* entry = malloc(sizeof(struct entry_t));
                // if(entry == NULL){
                //     return handleError(msg);
                // }

                entry_temp->key = dupKey;
                entry_temp->value.len = data->datasize;
                entry_temp->value.data = data->data;

                //entry = entry_create(dupKey, data);
                entries[j] = entry_temp;
            }

            msg->n_entries = nKeys;
            EntryT** msg_entries = (EntryT**) entries;
            msg->entries = msg_entries;

            break;
        default: // Case BAD || ERROR
            return handleError(msg);
    }
    return 0;
}
