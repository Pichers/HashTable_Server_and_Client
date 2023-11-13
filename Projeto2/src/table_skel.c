#include <stddef.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "table_skel.h"
#include "entry.h"
#include "table.h"
#include "sdmessage.pb-c.h"

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

    if(table == NULL){
        return handleError(msg);
    }
    if(msg == NULL){
        return handleError(msg);
    }

    MessageT__Opcode opCode = msg->opcode;

    switch((int) opCode) {
        case (int) MESSAGE_T__OPCODE__OP_PUT:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            struct data_t* data = data_create(msg->entry->value.len, msg->entry->value.data);
            struct entry_t* entry = entry_create(strdup(msg->entry->key), data_dup(data));
            char* key = strdup(msg->key);

            int i = table_put(table, entry->key, entry->value);
            // free(msg->key);

            if(i == -1){
                free(data);
                entry_destroy(entry);
                return handleError(msg);
            }
            free(data);
            entry_destroy(entry);  
            break;
        case (int) MESSAGE_T__OPCODE__OP_GET:

            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            key = msg->key;

            struct data_t *dataValue = table_get(table, key);

            if(dataValue == NULL){
                handleError(msg);
                perror("Erro ao encontrar valor");

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
            if(table_remove(table, key) == -1){
                return handleError(msg);
            }

            break;
        case (int) MESSAGE_T__OPCODE__OP_SIZE:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            int size = table_size(table);

            if(size == -1){
                return handleError(msg);
            }
            msg->result = size;

            break;
        case (int) MESSAGE_T__OPCODE__OP_GETKEYS:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            char** keys = table_get_keys(table);

            if(keys == NULL){
                return handleError(msg);
            } else {
                int j = 0;
                while(keys[j] != NULL){
                    j++;
                }                      

                msg->n_keys = j; 
                msg->keys = keys;
            }

            break;
        case (int) MESSAGE_T__OPCODE__OP_GETTABLE:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            keys = table_get_keys(table);
            
            if(keys == NULL){
                return handleError(msg);
                
            }

            size_t nKeys = 0;

            while(keys[nKeys] != NULL){
                nKeys++;
            }
            msg->n_entries = nKeys;
            msg->entries = (EntryT **)malloc((nKeys+1) * sizeof(struct EntryT *));
            EntryT* entry_temp = malloc((num_keys) * sizeof(EntryT));
            if(msg->entries == NULL){
                return handleError(msg);
            }

            for(int j = 0; j < nKeys; j++){
                entry_t__init(&entry_temp);

                char* key = keys[j];
                struct data_t* data = data_dup(table_get(table, keys[j]));
                if(data == NULL){
                    return handleError(msg);
                }

                char* dupKey = strdup(key);
                if(dupKey == NULL){
                    return handleError(msg);
                }

                struct entry_t* entry = malloc(sizeof(struct entry_t));
                if(entry == NULL){
                    return handleError(msg);
                }

                entry = entry_create(dupKey, data);
                entries[j] = entry;
            }

            msg->n_entries = nKeys;
            EntryT** msg_entries = (EntryT**)entries;
            msg->entries = msg_entries;

            break;
        default: // Case BAD || ERROR
            return handleError(msg);
    }
    return 0;
}
