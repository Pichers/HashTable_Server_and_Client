// Grupo 62
// António Almeida - nº 58235
// Carlos Van-Dùnem - nº 58202
// Pedro Cardoso - nº 58212

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "entry.h"

/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados de entrada.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_create(char *key, struct data_t *data){

    if(key == NULL || data == NULL){
        return NULL;
    }

    struct entry_t* entry;
    entry = malloc(sizeof(struct entry_t));

    entry->key = key;
    entry->value = data;

    return entry;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_destroy(struct entry_t *entry){

    if(entry == NULL)
        return -1;

    if(entry->key == NULL){
        return -1;
    }
    if(entry->value == NULL){
        return -1;
    }
    if(entry->key != NULL)
        free(entry->key);
    if(entry->value != NULL)
        data_destroy(entry->value);

    free(entry);

    return 0;
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_dup(struct entry_t *entry){
    if(entry == NULL)
        return NULL;

    struct entry_t* newEntry;
    newEntry = entry_create(entry->key, entry->value);
    if(newEntry == NULL)
        return NULL;

    newEntry->key = strdup(entry->key);
    newEntry->value = data_dup(entry->value);

    return newEntry;
}

/* Função que substitui o conteúdo de uma entry, usando a nova chave e
 * o novo valor passados como argumentos, e eliminando a memória ocupada
 * pelos conteúdos antigos da mesma.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value){
    if (entry == NULL || new_key == NULL || new_value == NULL)
        return -1;
    
    free(entry->key);

    data_destroy(entry->value);
    
    entry->key = new_key;
    entry->value = new_value;

    return 0;
}

/* Função que compara duas entries e retorna a ordem das mesmas, sendo esta
 * ordem definida pela ordem das suas chaves.
 * Retorna 0 se as chaves forem iguais, -1 se entry1 < entry2,
 * 1 se entry1 > entry2 ou -2 em caso de erro.
 */
int entry_compare(struct entry_t *entry1, struct entry_t *entry2){
    if(entry1 == NULL || entry2 == NULL)
        return -2;

    int r = strcmp(entry1->key, entry2->key);

    if(r == 0)
        return 0;
    else if(r < 0)
        return -1;
    else
        return 1;
}