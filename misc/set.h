#ifndef SET_H
#define SET_H

#include <stdint.h>

#define SET_DATA_PADDING_ALIGNEMENT 4

struct setBlock{
	uint32_t 			nb_element;
	struct setBlock* 	next;
	struct setBlock* 	prev;
	char 				data[SET_DATA_PADDING_ALIGNEMENT];
};

#define setBlock_get_size(element_size, nb_element_block) (sizeof(struct setBlock) + (element_size) * (nb_element_block) - SET_DATA_PADDING_ALIGNEMENT)

struct set{
	uint32_t 			nb_element_tot;
	uint32_t 			element_size;
	uint32_t 			nb_element_block;
	struct setBlock		block;
};

#define set_get_size(element_size, nb_element_block) (sizeof(struct set) + (element_size) * (nb_element_block) - SET_DATA_PADDING_ALIGNEMENT)

struct set* set_create(uint32_t element_size, uint32_t nb_element_block);

#define set_init(set, element_size_, nb_element_block_) 							\
	(set)->nb_element_tot 	= 0; 													\
	(set)->element_size 	= element_size_; 										\
	(set)->nb_element_block = ((nb_element_block_) < 1) ? 1 : (nb_element_block_); 	\
	(set)->block.nb_element = 0; 													\
	(set)->block.next 		= NULL; 												\
	(set)->block.prev 		= NULL;

int32_t set_add(struct set* set, void* element);
void* set_get(struct set* set, uint32_t index);
int32_t set_search(struct set* set, void* element);
void set_remove(struct set* set, void* element);

#define set_get_length(set) ((set)->nb_element_tot)

int32_t set_are_disjoint(struct set* set1, struct set* set2);

void* set_export_buffer(struct set* set);
void* set_export_buffer_unique(struct set* set, uint32_t* nb_element);

#define set_empty(set) 																\
	set_clean(set); 																\
	(set)->nb_element_tot 	= 0; 													\
	(set)->block.nb_element = 0; 													\
	(set)->block.next 		= NULL;

void set_clean(struct set* set);

#define set_delete(set) 															\
	set_clean(set); 																\
	free(set);

struct setIterator{
	struct set* 	 	set;
	struct setBlock* 	block;
	uint32_t 			element;
};

void* setIterator_get_first(struct set* set, struct setIterator* iterator);
void* setIterator_get_next(struct setIterator* iterator);

#define setIterator_get_current(iterator) ((iterator)->block->data + ((iterator)->element * (iterator)->set->element_size))

void setIterator_pop(struct setIterator* iterator); 


#endif