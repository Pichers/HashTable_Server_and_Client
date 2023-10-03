#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H

#include "entry.h"

struct node_t {
	struct entry_t *entry;
	struct node_t  *next;
	struct node_t *previous;
};

struct list_t {
	int size;
	struct node_t *head;
};

/**
 * Creates a new empty list
*/
struct list_t* makeList();

/**
 * Returns the head entry of the list
*/
struct entry_t* head(struct list_t* l);

/**
 * Returns the tail of the list
*/
struct list_t* tail(struct list_t* l);

/**
 * Returns the element of the list l with index i
*/
struct entry_t* get(struct list_t* l, int i);

/**
 * Returns 1 if the list l is empty, 0 otherwise
*/
int isEmpty(struct list_t* l);

/**
 * Adds the given entry e to the end of the list l
*/
void add(struct list_t* l, struct entry_t* e);

/**
 * returns the node with the given key
*/
struct node_t* getNode(struct list_t* l, char* k);

#endif