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

/* Executa na tabela table a operação indicada pelo opcode contido em msg 
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
*/
int invoke(MessageT *msg, struct table_t *table){
    struct MessageT__Opcode* opCode = msg->opcode;

    int i;

    switch(opCode) {
        case MESSAGE_T__OPCODE__OP_PUT:
            
            msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            char* key = msg->key;

            //???
            ProtobufCBinaryData msgValue = msg->value;
            struct data_t* data = data_create(msgValue->len, msgValue->data);

            i = table_put(table, key, data);
            msg->key = NULL;

            if(i == -1){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }

            break;
        case MESSAGE_T__OPCODE__OP_GET:

            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            char* key = msg->key;

            struct data_t* dataValue = table_get(table, key);

            if(dataValue == NULL){
                i = -1;
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
            else{
                msg->value = dataValue;
                msg->key = NULL;
                i = 0
            }

            break;
        case MESSAGE_T__OPCODE__OP_DEL:
            
            msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            char* key = msg->key;

            i = table_remove(table, key);

            msg->key = NULL

            if(i = -1){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }

            break;
        case MESSAGE_T__OPCODE__OP_SIZE:
            
            msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            int size = table_size(table);

            if(size == -1){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                i = -1;
            }
            else{
                msg->result = size;
                i = 0;
            }


            break;
        case MESSAGE_T__OPCODE__OP_GETKEYS:
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            char** keys = table_get_keys(table);

            if(keys == NULL){
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                i = -1;
            }
            else{
                int j = 0;
                while(keys[j] != NULL){ //
                    j++;                //
                }                       //
                                        //
                msg->n_keys = j-1;      //não sei se é suposto por isto, mas parece acertado
                msg->keys = keys;
                i = 0;
            }

            break;
        case MESSAGE_T__OPCODE__OP_GETTABLE:
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            //coisinhas (não percebi "EntryT")

            break;
        default: // Case BAD || ERROR
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            i = -1;
    }
    return i;
}
