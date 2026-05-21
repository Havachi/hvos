#ifndef HVOS_DEFS_LIST_H
#define HVOS_DEFS_LIST_H
#include <stdint.h>
typedef struct listnode_s {
	struct listnode_s *prev;
	struct listnode_s *next;
	void *val;
} listnode_t;

typedef struct list_s {
	listnode_t *head;
	listnode_t *tail;
	uint32_t size;
} list_t ;

#define foreach(t, list) for(listnode_t *t = list->head; t != NULL; t = t->next)

#endif