#ifndef _HVOS_DATA_LIST_H
#define _HVOS_DATA_LIST_H 1

#include <stdlib.h>

typedef struct list_node_s {
	void *data;
	struct list_node_s *next;
} list_node_t;

typedef struct list_s {
	unsigned long count;
	list_node_t *head;
} list_t;


list_t *init_list();
list_node_t *new_node(void *data, list_node_t *next);

///Get Operation
void *list_get_at(list_t *list, unsigned long at);
list_node_t *list_get_node_at(list_t *list, unsigned long at);

///Add operation

//Add node at the end of the list
void list_append(list_t *list, void *data);
//Add node at the start (at head location)
void list_push(list_t *list, void *data);
//Add node at designed index
void list_insert_at(list_t *list, unsigned long at, void *data);

///Delete operation

void list_pop(list_t *list);
void list_remove_at(list_t *list, unsigned long at);

///Compare




///Other
list_t *list_filter(list_t *list, int (*callback)(void *));
void *get_first_match(list_t *list, int (*callback)(void *, void *), void *args);



#endif