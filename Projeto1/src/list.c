#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/list-private.h"
#include "../include/list.h"

/* Função que cria e inicializa uma nova lista (estrutura list_t a
 * ser definida pelo grupo no ficheiro list-private.h).
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create(){
    struct list_t* list = malloc(sizeof(struct list_t*));
    if(list == NULL){
        //in case of memory allocation fail
        return NULL;
    }

    if(list == NULL){
        return NULL;
    }
    
    list->size = 0;
    list->head = NULL;

    return list;
}

/* Função que elimina uma lista, libertando *toda* a memória utilizada
 * pela lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *list){
    if(list == NULL)
        return -1;
    if(isEmpty(list))
        free(list);
    else {
        struct node_t* cNode = list->head;

        for (int i = 0; i < list->size; i++){
            entry_destroy(cNode->entry);

            if(i == list->size - 1)
                free(cNode); //free the last node
            else{
                cNode = cNode->next;
            
                free(cNode->previous);
            }
        }

        free(list);
    }
}

/* Função que adiciona à lista a entry passada como argumento.
 * A entry é inserida de forma ordenada, tendo por base a comparação
 * de entries feita pela função entry_compare do módulo entry e
 * considerando que a entry menor deve ficar na cabeça da lista.
 * Se já existir uma entry igual (com a mesma chave), a entry
 * já existente na lista será substituída pela nova entry,
 * sendo libertada a memória ocupada pela entry antiga.
 * Retorna 0 se a entry ainda não existia, 1 se já existia e foi
 * substituída, ou -1 em caso de erro.
 */
int list_add(struct list_t *list, struct entry_t *entry){

    struct node_t* newNode = malloc(sizeof(struct node_t*));
    if(newNode == NULL){
        //in case of memory allocation fail
        return -1;
    }

    newNode->entry = entry;

    if(list == NULL || entry == NULL){
        return -1;
    }
    if(isEmpty(list)){
        
        newNode->next = NULL;
        newNode->previous = NULL;

        list->head = newNode;
        list->size = 1;
        return 0;
    }

    struct node_t* node1 = list->head;

    for (int i = 0; i < list->size; i++){

        if(entry_compare(entry, node1->entry) == -1){
            
            newNode->next = node1;
            newNode->previous = node1->previous;
            node1->previous = newNode;

            if(i == 0){
                list->head = newNode;

                list->size++;

                return 0;
            }

            newNode->previous->next = newNode;
            list->size++;

            return 0;
        }
        if(entry_compare(entry, node1->entry) == 0){

            newNode->next = node1->next;
            newNode->previous = node1->previous;

            if(i == 0){
                list->head = newNode;
                return 1;
            }
            newNode->previous->next = newNode;

            entry_destroy(node1->entry);
            free(node1);

            return 1;
        }
        if(node1->next == NULL)
            break;
        node1 = node1->next;
    }
    
    newNode->previous = node1;
    node1->next = newNode;

    list->size++;

    return 0;
}

/* Função que elimina da lista a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int list_remove(struct list_t *list, char *key){

    if(list == NULL)
        return -1;
    
    struct node_t* n = getNode(list, key);

    if(isEmpty(list) == 1 || n == NULL)
        return 1;

    if(n == list->head)
        list->head = n->next;
    else
        n->previous->next = n->next;

    if(n->next != NULL)
        n->next->previous = n->previous;

    entry_destroy(n->entry);
    free(n);

    list->size--;

    return 0;
}

/* Função que obtém da lista a entry com a chave key.
 * Retorna a referência da entry na lista ou NULL se não encontrar a
 * entry ou em caso de erro.
*/
struct entry_t* list_get(struct list_t* list, char* key){
    struct node_t* n = getNode(list, key);

    return n->entry;
}

/* Função que conta o número de entries na lista passada como argumento.
 * Retorna o tamanho da lista ou -1 em caso de erro.
 */
int list_size(struct list_t *list){
    return list->size;
}

/* Função que constrói um array de char* com a cópia de todas as keys na 
 * lista, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **list_get_keys(struct list_t *list){
    if(list == NULL || isEmpty(list))
        return NULL;
    
    char** list_keys = malloc((list->size + 1) * sizeof(char*));
    if (list_keys == NULL) {
        //in case of memory allocation fail
        return NULL;
    }

    struct node_t* cNode = list->head;
    
    for (int i = 0; i < list->size; i++){

        list_keys[i] = strdup(cNode->entry->key);
        if (list_keys[i] == NULL) {
            //in case of memory allocation fail
            for (int j = 0; j < i; j++) {
                free(list_keys[j]);
            }
            free(list_keys);
            return NULL;
        }
        
        cNode = cNode->next;
    }
    list_keys[list->size] = NULL;

    return list_keys;
    
}

/* Função que liberta a memória ocupada pelo array de keys obtido pela 
 * função list_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_keys(char **keys){
    if(keys == NULL)
        return -1;
    
    int i = 0;
    while (keys[i] != NULL){
        free(keys[i]);
        i++;
    }
    return 0;
}

/**
 * Returns 1 if the list l is empty, 0 if it's not, -1 in case of error
*/
int isEmpty(struct list_t* l){
    if(l == NULL)
        return -1;
    
    if(l->head == NULL && l->size == 0)
        return 1;
    if(l->head != NULL && l->size != 0)
        return 0;
    return -1;
}

struct node_t* getNode(struct list_t* l, char* k){

    if(l == NULL || k == NULL)
        return NULL;

    struct node_t* cNode = l->head;

    for (int i = 0; i < l->size; i++){
        if(strcmp(cNode->entry->key, k) == 0){
            return cNode;
        }
        if(cNode->next == NULL)
            break;
        cNode = cNode->next;
    }
    return NULL;
}
