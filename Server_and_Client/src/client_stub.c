#include "data.h"
#include "entry.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"
#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Function to establish an association between the client and the server,
 * where address_port is a string in the format <hostname>:<port>.
 * Returns the filled rtable structure or NULL in case of an error.
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
        free(hostname);
        printf("Error network connecting");
        return NULL;
    }
    
    free(hostname);
    return rtable;
}

/** 
 * Terminates the association between the client and the server, closing the
 * connection with the server and freeing all local memory.
 * Returns 0 if everything goes well, or -1 in case of an error.
 */
int rtable_disconnect(struct rtable_t *rtable){
    if(rtable == NULL){
        printf("why are we trying to disconnect a NULL rtable?\n");
        return -1;
    }
    
    int s = rtable->sockfd;
    if(s < 0){
        printf("disconnecting a negative socket? wth\n");
        return -1;
    }
    if(network_close(rtable) == -1){
        printf("Error closing socket in rtable\n");
        return -1;
    }

    free(rtable);
    return 0;
}

/** 
 * Function to add an element to the table.
 * If the key already exists, it will replace that entry with the new data.
 * Returns 0 (OK, for addition/replacement) or -1 (error).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry){

    if(rtable == NULL || entry == NULL)
        return -1;
    
    MessageT msg = MESSAGE_T__INIT;
    
    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    EntryT e = ENTRY_T__INIT;

    e.key = entry->key;

    //TODO nao precisa malloc nem packed data
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
        free(packedData);
        return -1;
    }  
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        message_t__free_unpacked(ret, NULL);
        free(packedData);
        return -1;
    } 
    message_t__free_unpacked(ret, NULL);
    free(packedData);

    return 0;
}

/**
 * Returns the element from the table with the key 'key', or NULL if it doesn't exist
 * or if an error occurs.
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
        free(msg.key);
        printf("Error sending message\n");
        return NULL;
    }  
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        free(msg.key);
        message_t__free_unpacked(ret, NULL);
        return NULL;
    } 

    free(msg.key);

    void* dataValue = malloc(ret->value.len);
    if(dataValue == NULL){
        message_t__free_unpacked(ret, NULL);
        return NULL;
    }
    memcpy(dataValue, ret->value.data, ret->value.len);

    struct data_t* data = data_create(ret->value.len,dataValue);
    if(data == NULL){
        message_t__free_unpacked(ret, NULL);
        return NULL;
    }
    message_t__free_unpacked(ret, NULL);
    return data;
}

/**
 * Function to remove an element from the table. It will free
 * all the memory allocated in the corresponding rtable_put() operation.
 * Returns 0 (OK) or -1 (key not found or error).
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
    if(ret->opcode == MESSAGE_T__OPCODE__OP_BAD){
        message_t__free_unpacked(ret, NULL);
        return -1;
    }
    if(ret->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        message_t__free_unpacked(ret, NULL);
        return -1;
    }
    message_t__free_unpacked(ret, NULL);
    return 0;
}

/**
 * Returns the number of elements in the table or -1 in case of error.
 */
int rtable_size(struct rtable_t *rtable){
    if(rtable == NULL)
        return -1;

    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* ret = network_send_receive(rtable, &msg);

    if(ret == NULL){
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

/**
 * Returns an array of char* with copies of all keys from the table,
 * placing a final NULL element in the array.
 * Returns NULL in case of an error.
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
        message_t__free_unpacked(ret, NULL);
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

/**
 * Frees the memory allocated by rtable_get_keys().
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

/** 
 * Returns an array of entry_t* with the entire contents of the table, placing
 * a final NULL element in the array.
 * Returns NULL in case of error.
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

        char* dataValue = malloc(e->value.len + 1);
        if(dataValue == NULL){
            message_t__free_unpacked(ret, NULL);
            return NULL;
        }

        memcpy(dataValue, e->value.data, e->value.len);
        dataValue[e->value.len] = '\0';

        char* dataValueDup = strdup(dataValue);
        struct data_t* data = data_create(e->value.len, dataValueDup);

        free(dataValue);

        char* keyDup = strdup(e->key);
        struct entry_t* entry = entry_create(keyDup, data);

        entries[i] = entry;
    }

    entries[ret->n_entries] = NULL;

    message_t__free_unpacked(ret, NULL);
    return entries;
}

/**
 * Frees the memory allocated by rtable_get_table().
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

/**
 * Returns the table stats, or NULL in case of error.
 */
struct stats_t *rtable_stats(struct rtable_t *rtable){
    if(rtable == NULL)
        return NULL;
    
    MessageT msg = MESSAGE_T__INIT;

    msg.opcode = MESSAGE_T__OPCODE__OP_STATS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_STATS;

    MessageT* ret = network_send_receive(rtable, &msg);
    if(ret == NULL){
        printf("Error sending message\n");
        return NULL;
    }


    struct stats_t* stats = malloc(sizeof(struct stats_t));
    if(stats == NULL){
        printf("Error allocating memory for stats\n");
        message_t__free_unpacked(ret, NULL);
        return NULL;
    }
    stats->total_operations = ret->stats->total_operations;
    stats->total_time = ret->stats->total_time;
    stats->connected_clients = ret->stats->connected_clients;

    message_t__free_unpacked(ret, NULL);
    return stats;
}