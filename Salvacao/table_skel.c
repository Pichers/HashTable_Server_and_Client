/*
Grupo 02
Simão Quintas 58190
Manuel Lourenço 58215
Renato Ferreira 58238
*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "entry.h"
#include "table-private.h"
#include "table_skel.h"
#include "stats.h"
#include "sdmessage.pb-c.h"




/****************/
int n_write = 0, n_write_w8 = 0, n_read = 0; 

pthread_mutex_t dados = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dados_disponiveis = PTHREAD_COND_INITIALIZER;
/***************/


void startRead(){
  pthread_mutex_lock(&dados);
  while(n_write > 0 || n_write_w8 > 0)
    pthread_cond_wait(&dados_disponiveis, &dados);
  n_read++;
  pthread_mutex_unlock(&dados);
}

void stopRead(){
  pthread_mutex_lock(&dados);
  n_read--;
  if(n_read==0)
    pthread_cond_signal(&dados_disponiveis);
  pthread_mutex_unlock(&dados);
}

void startWrite(){
  pthread_mutex_lock(&dados);
  n_write_w8++;
  while(n_read>0 || n_write > 0)
    pthread_cond_wait(&dados_disponiveis, &dados);
  n_write_w8--;
  n_write++;
  pthread_mutex_unlock(&dados);
}

void stopWrite(){
  pthread_mutex_lock(&dados);
  n_write--;
  pthread_cond_signal(&dados_disponiveis);
  pthread_mutex_unlock(&dados);
}



/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct table_t *table_skel_init(int n_lists){
    struct table_t* table = table_create(n_lists);
    if (table == NULL) {
        printf("Erro ao inicializar a tabela.\n");
    }
    return table;
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos 
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_skel_destroy(struct table_t *table){
    if (table_destroy(table) == -1) {
        printf("Erro ao destruir a tabela.\n");
        return -1;
    }
    return 0;
}


// Function to handle the "put" operation
int handle_put(struct table_t *table, EntryT *entryT, MessageT *msg) {
    // Implement the logic to add the key and data to the table
    struct entry_t* entry;
    struct data_t* data = data_create(msg->entry->value.len, msg->entry->value.data);
    entry = entry_create(strdup(msg->entry->key), data_dup(data));   

    startWrite();
    int result = table_put(table, entry->key, entry->value);
    stopWrite();
    if(result == -1){
        printf("Erro na operação put.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        free(data);
        entry_destroy(entry);
        return -1;
    }

    free(data);
    entry_destroy(entry);
    msg->opcode++; 
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    return 0;
}

// Function to handle the "get" operation
int handle_get(struct table_t *table, char *key, MessageT *msg) {
    // Implement the logic to retrieve data for the given key from the table
    
    startRead();
    struct data_t *data_ptr = table_get(table, key);
    stopRead();
    if(data_ptr == NULL){
        printf("Erro na operação get.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    // Assign binary_data to msg->value
    msg->value.len = data_ptr->datasize;
    msg->value.data = (uint8_t *)data_ptr->data; 

    // Update the message structure
    free(data_ptr);
    msg->opcode++; // You can use msg->opcode += 1 or msg->opcode = msg->opcode + 1
    msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
    return 0;
}


// Function to handle the "del" operation
int handle_del(struct table_t *table, char *key, MessageT *msg) {
    // Implement the logic to delete the entry for the given key from the table
    startWrite();
    int result = table_remove(table, key);
    stopWrite();
    if(result == -1){
        printf("Erro na operação del.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    // Atualizar a estrutura da mensagem
    msg->opcode++; //talvez seja preciso msg->Opcode += 1 ou msg->Opcode = msg->Opcode+1
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    return 0;
}

// Function to handle the "size" operation
int handle_size(struct table_t *table, MessageT *msg) {
    // Implement the logic to get the size of the table
    startRead();
    msg->result = table_size(table);
    stopRead();
    if(msg->result == -1){
        printf("Erro na operação size.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }


    // Atualizar a estrutura da mensagem
    msg->opcode++; //talvez seja preciso msg->Opcode += 1 ou msg->Opcode = msg->Opcode+1
    msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    return 0;
}

// Function to handle the "getkeys" operation
int handle_getkeys(struct table_t *table, MessageT *msg) {

    // Implement the logic to get the keys in the table
    startRead();
    msg->keys = table_get_keys(table);
    stopRead();
    size_t aux = 0;
    for (int i = 0; msg->keys[i] != NULL; i++){
        aux++;
    }
    msg->n_keys = aux;
    if(msg->keys == NULL){
        printf("Erro na operação getkeys.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    // Atualizar a estrutura da mensagem
    msg->opcode++; //talvez seja preciso msg->Opcode += 1 ou msg->Opcode = msg->Opcode+1
    msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
    return 0;
}

// Function to handle the "gettable" operation
int handle_gettable(struct table_t *table, MessageT *msg) {
    // Get the keys from the table

    startRead();
    char **keys = table_get_keys(table);
    stopRead();
    if (keys == NULL) {
        fprintf(stderr, "Error in operation gettable: Failed to retrieve keys.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    // Calculate the number of keys
    size_t num_keys = 0;
    while (keys[num_keys] != NULL) {
        num_keys++;
    }
    msg->n_entries = num_keys; 

    // Allocate memory for the entries array
    msg->entries = (EntryT **)malloc((num_keys) * sizeof(EntryT *));
    //EntryT* entry_temp = malloc((num_keys) * sizeof(EntryT));


    if (msg->entries == NULL) {
        fprintf(stderr, "Error in operation gettable: Memory allocation failed.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        table_free_keys(keys);
        return -1;
    }

    // Populate the entries array
    for (size_t i = 0; i < num_keys; i++) {
        msg->entries[i] = malloc(sizeof(EntryT));

        // Initialize the EntryT structure
        entry_t__init(msg->entries[i]);

        // Copy the key from the keys array
        char *auxKey = strdup(keys[i]);
        if (auxKey == NULL) {
            fprintf(stderr, "Error in operation gettable: Failed to get the key value.\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            table_free_keys(keys);
            return -1;
        }

        // Retrieve the data for the key
        struct data_t *data = table_get(table, auxKey);
        if (data == NULL) {
            fprintf(stderr, "Error in operation gettable: Failed to retrieve data for key.\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            table_free_keys(keys);
            return -1;
        }

        // Populate the EntryT structure
        msg->entries[i]->key = auxKey;
        msg->entries[i]->value.len = data->datasize;
        msg->entries[i]->value.data = malloc(data->datasize);
        memcpy(msg->entries[i]->value.data, data->data, data->datasize);

        data_destroy(data);
    }

    // Update the message structure
    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;

    // Free the keys array
    table_free_keys(keys);

    return 0;
}

// Function to handle the "stats" operation
int handle_stats(struct table_t *table, MessageT *msg, struct statistics_t *stats) {
    StatisticsT *statsAux = malloc(sizeof(StatisticsT));
    statistics_t__init(statsAux);
    
    statsAux->numop = stats->numOp;
    statsAux->wastedtime = stats->wastedTime;
    statsAux->connected = stats->connected;
    startRead();
    msg->stats = statsAux;
    stopRead();
    if(msg->stats == NULL){
        printf("Erro na operação stats.\n");
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    // Atualizar a estrutura da mensagem
    msg->opcode++;
    msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;
    return 0;
}

/* Executa na tabela table a operação indicada pelo opcode contido em msg 
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
*/
int invoke(MessageT *msg, struct table_t *table, struct statistics_t *stats) {
    if (msg == NULL || table == NULL) {
        return -1; // Error: Invalid input
    }

    // Determine the operation based on the opcode
    switch (msg->opcode) {
        case MESSAGE_T__OPCODE__OP_PUT: 
            return handle_put(table, msg->entry, msg);
        case MESSAGE_T__OPCODE__OP_GET: 
            return handle_get(table, msg->key, msg);
        case MESSAGE_T__OPCODE__OP_DEL: 
            return handle_del(table, msg->key, msg);
        case MESSAGE_T__OPCODE__OP_SIZE: 
            return handle_size(table, msg);
        case MESSAGE_T__OPCODE__OP_GETKEYS: 
            return handle_getkeys(table, msg);
        case MESSAGE_T__OPCODE__OP_GETTABLE: 
            return handle_gettable(table, msg);
        case MESSAGE_T__OPCODE__OP_STATS: 
            return handle_stats(table, msg, stats);
        default:
            printf("Codigo não suportado\n");
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1; // Error: Unsupported opcode
    }
}
