#include "data_structure/list.h"
#include "mem/mem.h"
#include <stdlib.h>


list_t *init_list() {
	list_t *list = NULL;
	
	list = kzalloc(sizeof(list_t));
	if (!list) return NULL;
	list->count = 0;
	list->head = NULL;
	return list;
}

list_node_t *new_node(void *data, list_node_t *next) {
	list_node_t *node = NULL;
	node = kzalloc(sizeof(list_node_t));
	if (!node)
		return NULL;
	node->data = data;
	node->next = next;
	return node;
}

void *list_get_at(list_t *list, unsigned long at) {
	list_node_t *node = list_get_node_at(list, at);
	return node != NULL ? node->data : NULL;
}

list_node_t *list_get_node_at(list_t *list, unsigned long at) {
	if (at >= list->count ) return NULL;
	list_node_t *current = list->head;
	for(unsigned long i = 1; i <= at && current != NULL; i++) {
		current = current->next;
	}
	return current;
}

//Add node at the end
void list_append(list_t *list, void *data) {
	list_node_t *node = NULL;
	if (!node)
		return;
	if (list->count) {
		list_node_t *last = list_get_at(list, list->count-1);
		last->next = new_node(data, NULL);
	} else {
		list->head = node;
	}
	list->count++;
}

void list_push(list_t *list, void *data) {
	list_node_t **head = &list->head;
	list_node_t *new = new_node(data, *head);
	*head = new;
	list->count++;
}

void list_insert_at(list_t *list, unsigned long at, void *data) {
	if (at == list->count)
		list_append(list, data);
	else if(at == 0)
		list_push(list, data);
	else {
		list_node_t *prev = list_get_node_at(list, at-1);
		prev->next = new_node(data, prev->next);
		list->count++;
	}
}

void list_remove_at(list_t *list, unsigned long at) {
	if(list->count == 0) return;
	if (!at){
		list_pop(list);
		return;
	}

	list_node_t *current = list->head;
	list_node_t *temp = NULL;
	for (unsigned int i = 1; i < at && current != NULL; i++) 
		current = current->next;

	temp = current->next;
	current->next = temp->next;
	if (temp->data != NULL)
		kfree(temp->data);
	kfree(temp->data);
	list->count--;
}

void list_pop(list_t *list) {
	if (!list->count) return;
	list_node_t *next = NULL;
	list_node_t **head = &list->head;
	
	next = (*head)->next;
	if ((*head)->data != NULL)
		kfree((*head)->data);
	kfree(*head);
	*head = next;
	list->count--;
}



//Create a new list with all node that return true to callback
//Doesn't copy the data, it only create another pointer to that data
list_t *list_filter(list_t *list, int (*callback)(void *)) {
	if (!list || list->count == 0 || callback == NULL)
		return NULL;

	list_t *res = init_list();
	if (res == NULL)
		return NULL;

	list_node_t *p = list->head;
	while (p != NULL) {
		if (callback(p->data)) {
			list_push(res, p->data);
		}
		p = p->next;
	}
	return res;
}

///Return the first node that match condition
void *get_first_match(list_t *list, int (*callback)(void *, void *), void *args) {
	if (!list || list->count == 0 || callback == NULL)
		return NULL;


	list_node_t *p = list->head;
	while(p != NULL) {
		if (callback(p->data, args)) {
			return p->data;
		}
		p = p->next;
	}
	return NULL;
}