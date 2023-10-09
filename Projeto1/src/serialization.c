#include "../include/serialization.h"
#include "../include/data.h"
#include "../include/entry.h"
#include "../include/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>



/* Serializa todas as chaves presentes no array de strings keys para o
 * buffer keys_buf, que sera alocado dentro da funcao. A serializa��o
 * deve ser feita de acordo com o seguinte formato:
 *    | int   | string | string | string |
 *    | nkeys | key1   | key2   | key3   |
 * Retorna o tamanho do buffer alocado ou -1 em caso de erro.
 */
int keyArray_to_buffer(char **keys, char **keys_buf) {
    if (keys == NULL || keys_buf == NULL)
        return -1;

    int nkeys = 0;
    int total_len = 0;

    // Calculate the total length of all keys and count the number of keys
    for (int i = 0; keys[i] != NULL; i++) {
        nkeys++;
        total_len += strlen(keys[i]) + 1; // Include the NULL terminator
    }

    // Allocate memory for the serialized data
    *keys_buf = malloc(sizeof(int) + total_len);
    if (*keys_buf == NULL)
        return -1;

    // Serialize the number of keys as an int
    int nkeys_network_order = htonl(nkeys);
    memcpy(*keys_buf, &nkeys_network_order, sizeof(int));

    // Serialize each key
    int offset = sizeof(int);
    for (int i = 0; i < nkeys; i++) {
        strcpy(*keys_buf + offset, keys[i]);
        offset += strlen(keys[i]) + 1;
    }

    return sizeof(int) + total_len;
}


/* De-serializa a mensagem contida em keys_buf, colocando-a num array de
 * strings cujo espaco em memoria deve ser reservado. A mensagem contida
 * em keys_buf devera ter o seguinte formato:
 *    | int   | string | string | string |
 *    | nkeys | key1   | key2   | key3   |
 * Retorna o array de strings ou NULL em caso de erro.
 */
char** buffer_to_keyArray(char *keys_buf) {
    if (keys_buf == NULL)
        return NULL;

    int nkeys;
    memcpy(&nkeys, keys_buf, sizeof(int));
    nkeys = ntohl(nkeys);

    char **keys = malloc((nkeys + 1) * sizeof(char*));
    if (keys == NULL)
        return NULL;

    int offset = sizeof(int);
    for (int i = 0; i < nkeys; i++) {
        keys[i] = strdup(keys_buf + offset);
        if (keys[i] == NULL) {
            // Clean up allocated memory
            for (int j = 0; j < i; j++) {
                free(keys[j]);
            }
            free(keys);
            return NULL;
        }
        offset += strlen(keys_buf + offset) + 1;
    }
    keys[nkeys] = NULL;

    return keys;
}


