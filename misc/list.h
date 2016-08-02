#ifndef LIST_H
#define LIST_H

#include <stdint.h>

struct listElement{
	struct listElement* prev;
	struct listElement* next;
};

#define listElement_get_data(el) ((void*)((struct listElement*)(el) + 1))

#define LIST_NULL_DATA ((void*)((char*)NULL + sizeof(struct listElement)))

struct list{
	uint32_t 				nb_element;
	size_t 					element_size;
	struct listElement* 	tail;
	struct listElement* 	head;	
};

#define list_init(list, element_size_) 			\
	(list).nb_element 	= 0; 					\
	(list).element_size = (element_size_); 		\
	(list).tail 		= NULL; 				\
	(list).head 		= NULL;

#define list_get_length(list) ((list)->nb_element)

int32_t list_add_head(struct list* list, const void* element);
int32_t list_add_tail(struct list* list, const void* element);

#define list_empty(list) 						\
	list_clean(list); 							\
	(list)->nb_element 		= 0; 				\
	(list)->tail 			= 0; 				\
	(list)->head 			= 0;

void list_clean(struct list* list);

struct listIterator{
	struct list* 			list;
	uint32_t 				index;
	struct listElement* 	cursor;
};

static inline void listIterator_init(struct listIterator* it, struct list* list){
	it->list 	= list;
	it->index 	= 0;
	it->cursor 	= NULL;
}

static inline void* listIterator_get_next(struct listIterator* it){
	if (it->cursor == NULL){
		it->index 	= 0;
		it->cursor 	= it->list->head;
	}
	else{
		it->index ++;
		it->cursor = it->cursor->next;
	}

	return it->cursor;
}

static inline void* listIterator_get_prev(struct listIterator* it){
	if (it->cursor == NULL){
		it->index 	= it->list->nb_element - 1;
		it->cursor 	= it->list->tail;
	}
	else{
		it->index --;
		it->cursor = it->cursor->prev;
	}

	return it->cursor;
}

void* listIterator_get_index(struct listIterator* it, struct list* list, uint32_t index);

#define listIterator_get_data(it) ((void*)((it).cursor + 1))

int32_t listIterator_push_next(struct listIterator* it, const void* element);
int32_t listIterator_push_prev(struct listIterator* it, const void* element);
void* listIterator_pop_next(struct listIterator* it);
void* listIterator_pop_prev(struct listIterator* it);

#endif
