#include <stddef.h> 
#include <stdlib.h>
#include <string.h>

#include "../include/table.h"
#include "../sdmessage.pb-c.h"

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct table_t *table_skel_init(int n_lists){
    struct table_t* table = table_create(n_lists);

    if (table == NULL)
        return NULL

    return table;
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos 
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_skel_destroy(struct table_t *table){

    int i = table_destroy(table);

    return i;
}

void handleError(struct MessageT* msg){
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

    struct MessageT__Opcode* opCode = msg->opcode;

    switch(opCode) {
        case MESSAGE_T__OPCODE__OP_PUT:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            char* key = msg->key;

            struct data_t* data = data_create(msg->value->len, msg->value->data);

            int i = table_put(table, key, data);
            msg->key = NULL;

            if(i == -1){
                return handleError(msg);
            }
            
            break;
        case MESSAGE_T__OPCODE__OP_GET:

            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            char* key = msg->key;

            struct data_t* dataValue = table_get(table, key);

            if(dataValue == NULL){
                return handleError(msg);
            }
            else{
                msg->value = dataValue;
                free(msg->entry)
                msg->key = NULL;
            }

            break;
        case MESSAGE_T__OPCODE__OP_DEL:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            char* key = msg->key;

            int i = table_remove(table, key);

            free(msg->key);

            if(i = -1){
                return handleError(msg);
            }

            break;
        case MESSAGE_T__OPCODE__OP_SIZE:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            int size = table_size(table);

            if(size == -1){
                return handleError(msg);
            }
            msg->result = size;

            break;
        case MESSAGE_T__OPCODE__OP_GETKEYS:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            char** keys = table_get_keys(table);

            if(keys == NULL){
                return handleError(msg);
            }
            else{
                int j = 0;
                while(keys[j] != NULL){ //
                    j++;                //
                }                       //
                                        //
                msg->n_keys = j;        //não sei se é suposto por isto, mas parece acertado
                msg->keys = keys;
            }

            break;
        case MESSAGE_T__OPCODE__OP_GETTABLE:
            
            msg->opcode++;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            struct char** keys = table_get_keys(table);
            
            if(keys == NULL){
                return handleError(table, i);
            }

            int nKeys = 0;

            while(keys[nKeys] != NULL){
                nKeys++;
            }

            struct entry_t** entries = malloc(nKeys * sizeof(struct entry_t));
            if(entries == NULL){
                return handleError(msg);
            }

            for(int j = 0; j < nKeys; j++){
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
            msg->entries = entries;

            break;
        default: // Case BAD || ERROR
            return handleError(msg);
    }
    return 0;
}