#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "base.h"

#define list_get_element_size(list) (sizeof(struct listElement) + (list)->element_size)

int32_t list_add_head(struct list* list, void* element){
	struct listElement* list_el;

	if ((list_el = (struct listElement*)malloc(list_get_element_size(list))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	memcpy(list_el + 1, element, list->element_size);

	list_el->prev = NULL;
	list_el->next = list->head;

	list->nb_element ++;
	if (list_el->next == NULL){
		list->tail = list_el;
	}
	else{
		list_el->next->prev = list_el;
	}
	list->head = list_el;

	return 0;
}

int32_t list_add_tail(struct list* list, void* element){
	struct listElement* list_el;

	if ((list_el = (struct listElement*)malloc(list_get_element_size(list))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	memcpy(list_el + 1, element, list->element_size);

	list_el->prev = list->tail;
	list_el->next = NULL;

	list->nb_element ++;
	list->tail = list_el;
	if (list_el->prev == NULL){
		list->head = list_el;
	}
	else{
		list_el->prev->next = list_el;
	}

	return list->nb_element - 1;
}

void list_clean(struct list* list){
	struct listElement* cursor;
	void* 				tmp;

	for (cursor = list->head; cursor != NULL; ){
		tmp = cursor;
		cursor = cursor->next;
		free(tmp);
	}
}

int32_t listIterator_push_next(struct listIterator* it, void* element){
	struct listElement* list_el;

	if (it->cursor == NULL){
		log_err("unable to push element after a NULL cursor");
		return -1;
	}

	if ((list_el = (struct listElement*)malloc(list_get_element_size(it->list))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	memcpy(list_el + 1, element, it->list->element_size);

	list_el->prev = it->cursor;
	list_el->next = it->cursor->next;
	
	it->cursor->next = list_el;
	if (list_el->next == NULL){
		it->list->tail = list_el;
	}
	else{
		list_el->next->prev = list_el;
	}

	it->list->nb_element ++;

	return it->index + 1;
}

int32_t listIterator_push_prev(struct listIterator* it, void* element){
	struct listElement* list_el;

	if (it->cursor == NULL){
		log_err("unable to push element before a NULL cursor");
		return -1;
	}

	if ((list_el = (struct listElement*)malloc(list_get_element_size(it->list))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	memcpy(list_el + 1, element, it->list->element_size);

	list_el->prev = it->cursor->prev;
	list_el->next = it->cursor;
	
	it->cursor->prev = list_el;
	if (list_el->prev == NULL){
		it->list->head = list_el;
	}
	else{
		list_el->prev->next = list_el;
	}

	it->list->nb_element ++;
	it->index ++;

	return it->index - 1;
}

void* listIterator_pop_next(struct listIterator* it){
	void* tmp;

	if (it->cursor == NULL){
		log_err("unable to pop a NULL cursor");
		return NULL;
	}

	tmp = it->cursor;

	if (it->cursor->prev == NULL){
		it->list->head = it->cursor->next;
	}
	else{
		it->cursor->prev->next = it->cursor->next;
	}
	if (it->cursor->next == NULL){
		it->list->tail = it->cursor->prev;
		
	}
	else{
		it->cursor->next->prev = it->cursor->prev;
	}

	it->cursor = it->cursor->next;
	it->list->nb_element --;

	free(tmp);

	return it->cursor;
}

void* listIterator_pop_prev(struct listIterator* it){
	void* tmp;

	if (it->cursor == NULL){
		log_err("unable to pop a NULL cursor");
		return NULL;
	}

	tmp = it->cursor;

	if (it->cursor->prev == NULL){
		it->list->head = it->cursor->next;
	}
	else{
		it->cursor->prev->next = it->cursor->next;
	}
	if (it->cursor->next == NULL){
		it->list->tail = it->cursor->prev;
		
	}
	else{
		it->cursor->next->prev = it->cursor->prev;
	}

	it->cursor = it->cursor->prev;
	it->index --;
	it->list->nb_element --;

	free(tmp);

	return it->cursor;
}
