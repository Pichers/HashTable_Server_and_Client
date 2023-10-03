#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/list-private.h"

// struct node_t {
// 	struct entry_t *entry;
// 	struct node_t  *next; 
// };

// struct list_t {
// 	int size;
// 	struct node_t *head;
// };


/**
 * Creates a new empty list
*/
struct list_t* makeList(){
    // struct node_t* head;

    // head->next = NULL;
    // head->entry = NULL;

    struct list_t* list = malloc(sizeof(struct list_t*));
    
    list->size = 0;
    list->head = NULL;

    return list;
}

/**
 * Returns the head entry of the list
*/
struct entry_t* head(struct list_t* l){
    if(l == NULL || isEmpty(l))
        return NULL;
    return l->head->entry;
}

/**
 * Returns the tail of the list
*/
struct list_t* tail(struct list_t* l){
    if(l == NULL || isEmpty(l)){
        return NULL;
    }
    struct list_t* nl = makeList();
    nl->head = l->head->next;
    nl->size = l->size - 1;

    return nl;
}

/**
 * Returns the element of the list l with index i, NULL if error
*/
struct entry_t* get(struct list_t* l, int i){

    if(l == NULL || isEmpty(l)){
        return NULL;
    }
    if(i>= l->size){
        return NULL;
    }

    struct node_t* currentNode = l->head;

    for (int j = 0; j < i; j++)
    {
        if (currentNode == NULL){
            return NULL;
        }
        currentNode = currentNode->next;
    }
    return currentNode;
}

/**
 * Returns 1 if the list l is empty, 0 otherwise
*/
int isEmpty(struct list_t* l){

}

/**
 * Adds the given entry e to the end of the list l
*/
void add(struct list_t* l, struct entry_t* e){

}