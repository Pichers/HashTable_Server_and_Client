#include "data.h"
#include "entry.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Remote table, que deve conter as informações necessárias para comunicar
 * com o servidor. A definir pelo grupo em client_stub-private.h
 */
struct rtable_t;

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
 */
struct rtable_t *rtable_connect(char *address_port) {
    if (address_port == NULL)
        return NULL;

    char *hostname = NULL;
    char *port_str = NULL;

    // Use strtok to split the string at the ':'
    char *token = strtok(address_port, ":");
    
    if (token != NULL) {
        hostname = strdup(token); // Duplicate the hostname
        token = strtok(NULL, ":");
        if (token != NULL) {
            port_str = token;
        } else {
            free(hostname); // Free the hostname if port is missing
            return NULL;
        }
    } else {
        return NULL;
    }

    // Create and initialize the rtable structure
    struct rtable_t *rtable = (struct rtable_t *) malloc(sizeof(struct rtable_t));
    if (rtable == NULL) {
        free(hostname);
        return NULL;
    }
    
    rtable->server_address = hostname;
    rtable->server_port = atoi(port_str);

    int i = network_connect(rtable);
    if (i == -1){
        printf("Error network connecting");
        return NULL;
    }
    
    printf("Connected to server\n");
    return rtable;
}
/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem, ou -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *rtable){
    if(rtable == NULL){
        printf("why are we trying to disconnect a NULL rtable?\n");
        return -1;
    }
        
    if(rtable->sockfd < 0){
        printf("disconnecting a negative socket? wth\n");
        return -1;
    }
    if(network_close(rtable) == -1){
        printf("Error closing socket in rtable\n");
        return -1;
    }
    
    free(rtable->server_address);
    free(rtable);
    return 0;
}

/* Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Retorna 0 (OK, em adição/substituição), ou -1 (erro).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry){

    if(rtable == NULL || entry == NULL)
        return -1;
    
    MessageT msg = MESSAGE_T__INIT;
    
    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    EntryT e = ENTRY_T__INIT;

    e.key = entry->key;

    // size_t entryPackedLen = entry_t__get_packed_size(&e);
    uint8_t *packedData = malloc(entry->value->datasize);
    
    if(packedData == NULL){
        printf("Error allocating memory for entry\n");
        return -1;
    }

    memcpy(packedData, entry->value->data, entry->value->datasize);

    e.value.len = entry->value->datasize;
    e.value.data = packedData;

    msg.entry = &e;

    MessageT* ret = network_send_receive(rtable, &msg);

    if(ret  == NULL){
        printf("Error sending message\n");
        return -1;
    }  
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return -1;
    } 
    message_t__free_unpacked(ret, NULL);
    free(packedData);

    return 0;
}

/* Retorna o elemento da tabela com chave key, ou NULL caso não exista
 * ou se ocorrer algum erro.
 */
struct data_t *rtable_get(struct rtable_t *rtable, char *key){
    if(rtable == NULL || key == NULL)
        return NULL;
    
    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    msg.key = strdup(key);

    MessageT* ret = network_send_receive(rtable, &msg);
    if(ret  == NULL){
        printf("Error sending message\n");
        return NULL;
    }  
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return NULL;
    } 

    free(msg.key);

    void* dataValue = malloc(ret->value.len);
    if(dataValue == NULL){
        message_t__free_unpacked(ret, NULL);
        // printf("Error creating data value\n");
        return NULL;
    }
    memcpy(dataValue, ret->value.data, ret->value.len);

    struct data_t* data = data_create(ret->value.len,dataValue);
    if(data == NULL){
        // printf("Error creating data\n");
        message_t__free_unpacked(ret, NULL);
        return NULL;
    }
    message_t__free_unpacked(ret, NULL);
    return data;
}

/* Função para remover um elemento da tabela. Vai libertar 
 * toda a memoria alocada na respetiva operação rtable_put().
 * Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
 */
int rtable_del(struct rtable_t *rtable, char *key){
    if(rtable == NULL || key == NULL)
        return -1;

    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    msg.key = strdup(key);
    MessageT* ret = network_send_receive(rtable, &msg);
    free(msg.key);
    if(ret == NULL){
        printf("Error sending message\n");
        return -1;
    }
    // printf("opcode == error: %d\n", (ret->opcode == MESSAGE_T__OPCODE__OP_ERROR));
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return -1;
    }
    message_t__free_unpacked(ret, NULL);
    return 0;
}

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
 */
int rtable_size(struct rtable_t *rtable){
    if(rtable == NULL)
        return -1;

    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* ret = network_send_receive(rtable, &msg);

    if(ret  == NULL){
        printf("Error sending message\n");
        return -1;
    }  
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return -1;
    }  
    int size = ret->result;

    message_t__free_unpacked(ret, NULL);

    return size;
}

/* Retorna um array de char* com a cópia de todas as keys da tabela,
 * colocando um último elemento do array a NULL.
 * Retorna NULL em caso de erro.
 */
char **rtable_get_keys(struct rtable_t *rtable){
    if(rtable == NULL)
        return NULL;

    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* ret = network_send_receive(rtable, &msg);

    if(ret == NULL){
        printf("Error sending message\n");
        return NULL;
    }
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return NULL;
    }
    char** retKeys = ret->keys;
    char** keys = malloc(sizeof(char*) * (ret->n_keys + 1));
    
    if(keys == NULL){
        printf("Error allocating memory for keys\n");
        message_t__free_unpacked(ret, NULL);
        return NULL;
    }

    for (int i = 0; i < ret->n_keys; i++){
        keys[i] = strdup(retKeys[i]);
    }
    keys[ret->n_keys] = NULL;

    message_t__free_unpacked(ret, NULL);
    return keys;
}

/* Liberta a memória alocada por rtable_get_keys().
 */
void rtable_free_keys(char **keys){
    if(keys == NULL)
        return;
    for (int i = 0; keys[i] != NULL; i++){
        free(keys[i]);
    }
    free(keys);
    return;
}

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
 * um último elemento do array a NULL. Retorna NULL em caso de erro.
 */
struct entry_t **rtable_get_table(struct rtable_t *rtable){
    if(rtable == NULL)
        return NULL;
    
    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_GETTABLE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* ret = network_send_receive(rtable, &msg);
    if(ret == NULL){
        printf("Error sending message\n");
        return NULL;
    }
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return NULL;
    }

    struct entry_t** entries = malloc(sizeof(struct entry_t*) * (ret->n_entries + 1));
    if(entries == NULL){
        printf("Error allocating memory for entries\n");
        message_t__free_unpacked(ret, NULL);
        return NULL;
    }
    EntryT** entriesT = ret->entries;

    for(int i = 0; i < ret->n_entries; i++){
        EntryT* e = entriesT[i];

        char* dataValue = strdup((char*)(e->value.data));
        struct data_t* data = data_create(e->value.len, dataValue);

        char* keyDup = strdup(e->key);
        struct entry_t* entry = entry_create(keyDup, data);

        entries[i] = entry;
    }

    entries[ret->n_entries] = NULL;

    message_t__free_unpacked(ret, NULL);
    return entries;
}

/* Liberta a memória alocada por rtable_get_table().
 */
void rtable_free_entries(struct entry_t **entries){
    if(entries == NULL)
        return;
    for (int i = 0; entries[i] != NULL; i++){
        struct entry_t* e = entries[i];
        entry_destroy(e);
    }
    
    free(entries);
    return;
}
