#define _TABLE_H /* Módulo table */

#include "../include/table.h"
#include "../include/data.h"
#include "../include/list.h"
#include "../include/entry.h"
#include "../include/table-private.h"
#include <stddef.h> 
#include <stdlib.h>
#include <string.h>

struct table_t {
    int size;
    struct list_t **lists;
};

// Function to create and initialize a new hash table
struct table_t *table_create(int n) {
    if (n <= 0) {
        return NULL;
    }

    struct table_t *table = (struct table_t *)malloc(sizeof(struct table_t));
    if (table == NULL) {
        return NULL;
    }

    table->size = n;
    table->lists = (struct list_t **)malloc(sizeof(struct list_t *) * n);
    if (table->lists == NULL) {
        free(table);  // Free table if table->lists allocation fails
        return NULL;
    }

    // Initialize table->lists elements to NULL
    for (int i = 0; i < n; i++) {
        table->lists[i] = NULL;
    }

    return table;
}


// Function to destroy a table
int table_destroy(struct table_t *table) {
    if (table == NULL) {
        return -1;
    }

    for (int i = 0; i < table->size; i++) {
        if (table->lists[i] != NULL) {
            list_destroy(table->lists[i]);
        }
    }

    free(table->lists);
    free(table);
    return 0;
}

// Function to add a key-value pair to the table
int table_put(struct table_t *table, char *key, struct data_t *value) {
    if (table == NULL || key == NULL || value == NULL) {
        return -1;
    }

    int hash = hash_code(key, table->size);
    if (table->lists[hash] == NULL) {
        table->lists[hash] = list_create();
        if (table->lists[hash] == NULL) {
            return -1;
        }
    }

    struct entry_t *entry = entry_create(strdup(key), data_dup(value));
    if (entry == NULL) {
        return -1;
    }

    if (list_add(table->lists[hash], entry) != 0) {
        return -1;
    }

    return 0;
}

// Function to search the table for an entry with the given key
struct data_t *table_get(struct table_t *table, char *key) {
    if (table == NULL || key == NULL) {
        return NULL;
    }
    int hash = hash_code(key, table->size);
    if (table->lists[hash] == NULL) {
        return NULL;
    }
    struct entry_t *entry = list_get(table->lists[hash], key);
    if (entry == NULL) {
        return NULL;
    }
    return data_dup(entry->value);
}

// Function to remove an entry with the specified key from the table
int table_remove(struct table_t *table, char *key) {
    if (table == NULL || key == NULL) {
        return -1; 
    }

    int hash = hash_code(key, table->size);
    if (hash < 0 || hash >= table->size || table->lists[hash] == NULL) {
        return 1; 
    }

    int result = list_remove(table->lists[hash], key);
    return result;
}

// Function to count the number of entries in the table
int table_size(struct table_t *table) {
    if (table == NULL) {
        return -1;
    }

    int size = 0;
    for (int i = 0; i < table->size; i++) {
        if (table->lists[i] != NULL) {
            size += list_size(table->lists[i]);
        }
    }

    return size;
}

// Function to build an array of char* with copies of all the keys in the table
char **table_get_keys(struct table_t *table) {
    if (table == NULL) {
        return NULL;
    }

    char **keys = (char **)malloc(sizeof(char *) * (table_size(table) + 1));
    if (keys == NULL) {
        return NULL;
    }

    int j = 0;
    for (int i = 0; i < table->size; i++) {
        if (table->lists[i] != NULL) {
            char **list_keys = list_get_keys(table->lists[i]);
            if (list_keys == NULL) {
                return NULL;
            }

            int k = 0;
            while (list_keys[k] != NULL) {
                keys[j] = strdup(list_keys[k]);
                if (keys[j] == NULL) {
                    return NULL;
                }

                k++;
                j++;
            }
        }
    }

    keys[j] = NULL;
    return keys;
}

// Function to free the memory occupied by the array of keys obtained
int table_free_keys(char **keys) {
    if (keys == NULL) {
        return -1;
    }

    for (int i = 0; keys[i] != NULL; i++) {
        free(keys[i]);
    }

    free(keys);
    return 0;
}

// Function to calculate the hash code for a given key
int hash_code(char *key, int n) {
    int sum = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        sum += key[i];
    }

    return sum % n;
}