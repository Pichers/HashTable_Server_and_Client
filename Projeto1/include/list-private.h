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
 * Returns 1 if the list l is empty, 0 otherwise
*/
int isEmpty(struct list_t* l);

/**
 * returns the node with the given key
*/
struct node_t* getNode(struct list_t* l, char* k);

#endif