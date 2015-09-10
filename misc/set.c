#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "set.h"
#include "base.h"

struct set* set_create(uint32_t element_size, uint32_t nb_element_block){
	struct set* set;

	set = (struct set*)malloc(set_get_size(element_size, nb_element_block));
	if (set != NULL){
		set_init(set, element_size, nb_element_block)
	}
	else{
		log_err("unable to allocate memory");
	}

	return set;
}

int32_t set_add(struct set* set, void* element){
	struct setBlock* block_cursor;

	for (block_cursor = &(set->block); block_cursor != NULL; block_cursor = block_cursor->next){
		if (block_cursor->nb_element < set->nb_element_block){
			memcpy(block_cursor->data + (block_cursor->nb_element * set->element_size), element, set->element_size);
			block_cursor->nb_element ++;
			return (int32_t)(set->nb_element_tot ++);
		}

		if (block_cursor->next == NULL){
			block_cursor->next = (struct setBlock*)malloc(setBlock_get_size(set->element_size, set->nb_element_block));
			if (block_cursor->next == NULL){
				log_err("unable to allocate memory");
			}
			else{
				block_cursor->next->nb_element 	= 0;
				block_cursor->next->next 		= NULL;
				block_cursor->next->prev 		= block_cursor;
			}
		}
	}

	return -1;
}

int32_t set_add_unique(struct set* set, void* element){
	struct setBlock* 	block_cursor;
	uint32_t 			i;
	uint32_t 			element_count;

	for (block_cursor = &(set->block), element_count = 0; block_cursor != NULL; element_count += block_cursor->nb_element, block_cursor = block_cursor->next){
		for (i = 0; i < block_cursor->nb_element; i++){
			if (!memcmp(block_cursor->data + (i * set->element_size), element, set->element_size)){
				return (int32_t)(element_count + i);
			}
		}

		if (block_cursor->nb_element < set->nb_element_block){
			memcpy(block_cursor->data + (block_cursor->nb_element * set->element_size), element, set->element_size);
			block_cursor->nb_element ++;
			return (int32_t)(set->nb_element_tot ++);
		}

		if (block_cursor->next == NULL){
			block_cursor->next = (struct setBlock*)malloc(setBlock_get_size(set->element_size, set->nb_element_block));
			if (block_cursor->next == NULL){
				log_err("unable to allocate memory");
			}
			else{
				block_cursor->next->nb_element 	= 0;
				block_cursor->next->next 		= NULL;
				block_cursor->next->prev 		= block_cursor;
			}
		}
	}

	return -1;
}

void* set_get(struct set* set, uint32_t index){
	struct setBlock* 	block_cursor;
	uint32_t 			nb;

	for (block_cursor = &(set->block), nb = 0; block_cursor != NULL; block_cursor = block_cursor->next){
		if (nb + set->nb_element_block > index){
			return block_cursor->data + (index - nb) * set->element_size;
		}
		else{
			nb += set->nb_element_block;
		}
	}

	return NULL;
}

int32_t set_search(struct set* set, void* element){
	struct setBlock* 	block_cursor;
	uint32_t 			index;
	uint32_t 			i;

	for (block_cursor = &(set->block), index = 0; block_cursor != NULL; block_cursor = block_cursor->next){
		for (i = 0; i < block_cursor->nb_element; i++){
			if (!memcmp(block_cursor->data + i * set->element_size, element, set->element_size)){
				return (int32_t)(index + i);
			}
		}
		index += set->nb_element_block;
	}

	return -1;
}

void set_remove(struct set* set, void* element){
	struct setBlock* 	block_cursor;
	uint32_t 			i;

	for (block_cursor = &(set->block); block_cursor != NULL; block_cursor = block_cursor->next){
		for (i = 0; i < block_cursor->nb_element; i++){
			if (!memcmp(block_cursor->data + (i * set->element_size), element, set->element_size)){
				if (block_cursor->nb_element == 1 && block_cursor->prev != NULL){
					if (block_cursor->next != NULL){
						block_cursor->next->prev = block_cursor->prev;
					}
					block_cursor->prev->next = block_cursor->next;
					free(block_cursor);
				}
				else{
					memmove(block_cursor->data + (i * set->element_size), block_cursor->data + ((i + 1) * set->element_size), set->element_size * (block_cursor->nb_element - (i + 1)));
					block_cursor->nb_element --;
				}

				set->nb_element_tot --;

				return;
			}
		}
	}

	log_err_m("unable to find element %p in set", element);
}

int32_t set_are_disjoint(struct set* set1, struct set* set2){
	struct setIterator 	iterator1;
	struct setIterator 	iterator2;
	void* 				data1;
	void* 				data2;

	if (set1->element_size != set2->element_size){
		log_err_m("unable to compare sets since their element differ (%u vs %u)", set1->element_size, set2->element_size);
		return 0;
	}

	for (data1 = setIterator_get_first(set1, &iterator1); data1 != NULL; data1 = setIterator_get_next(&iterator1)){
		for (data2 = setIterator_get_first(set2, &iterator2); data2 != NULL; data2 = setIterator_get_next(&iterator2)){
			if (!memcmp(data1, data2, set1->element_size)){
				return 1;
			}
		}
	}

	return 0;
}

void* set_export_buffer(struct set* set){
	uint8_t* 			buffer;
	struct setBlock* 	block_cursor;
	uint32_t 			offset;

	buffer = (uint8_t*)malloc(set->nb_element_tot * set->element_size);
	if (buffer != NULL){
		for (block_cursor = &(set->block), offset = 0; block_cursor != NULL; block_cursor = block_cursor->next){
			memcpy(buffer + offset, block_cursor->data, set->element_size * block_cursor->nb_element);
			offset += set->element_size * block_cursor->nb_element;
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return buffer;
}

void* set_export_buffer_unique(struct set* set, uint32_t* nb_element){
	void* 		buffer;
	uint32_t 	i;
	uint32_t 	offset;

	buffer = set_export_buffer(set);
	if (buffer != NULL){
		qsort_r(buffer, set->nb_element_tot, set->element_size, (__compar_d_fn_t)memcmp, (void*)(set->element_size));
		for (i = 1, offset = 1; i < set->nb_element_tot; i++){
			if (memcmp((uint8_t*)buffer + i * set->element_size, (uint8_t*)buffer + (i - 1) * set->element_size, set->element_size)){
				if (i != offset){
					memcpy((uint8_t*)buffer + offset * set->element_size, (uint8_t*)buffer + i * set->element_size, set->element_size);
				}
				offset ++;
			}
		}
		buffer = realloc(buffer, offset * set->element_size);
		if (buffer == NULL){
			log_err("unable to realloc");
		}
		*nb_element = offset;
	}
	else{
		log_err("unable to export set");
	}

	return buffer;
}

void set_clean(struct set* set){
	struct setBlock* block_cursor;
	struct setBlock* block_delete;

	for (block_cursor = set->block.next; block_cursor != NULL; ){
		block_delete = block_cursor;
		block_cursor = block_cursor->next;
		free(block_delete);
	}
}

void* setIterator_get_first(struct set* set, struct setIterator* iterator){
	iterator->set 		= set;
	iterator->block 	= &(set->block);
	iterator->element 	= 0;

	for ( ; iterator->block != NULL; iterator->block = iterator->block->next){
		if (iterator->block->nb_element > 0){
			return iterator->block->data;
		}
	}

	return NULL;
}

void* setIterator_get_next(struct setIterator* iterator){
	for (iterator->element ++; iterator->block != NULL; iterator->block = iterator->block->next, iterator->element = 0){
		if (iterator->element < iterator->block->nb_element){
			return setIterator_get_current(iterator);
		}
	}

	return NULL;
}

void setIterator_pop(struct setIterator* iterator){
	struct setBlock* block_delete;

	if (iterator->block != NULL && iterator->element < iterator->block->nb_element){
		if (iterator->block->nb_element == 1 && iterator->block->prev != NULL){
			if (iterator->block->next != NULL){
				iterator->block->next->prev = iterator->block->prev;
			}
			iterator->block->prev->next = iterator->block->next;

			block_delete = iterator->block;

			iterator->block = iterator->block->prev;
			iterator->element = iterator->block->nb_element - 1;

			free(block_delete);
		}
		else{
			memmove(iterator->block->data + (iterator->element * iterator->set->element_size), iterator->block->data + ((iterator->element + 1) * iterator->set->element_size), iterator->set->element_size * (iterator->block->nb_element - (iterator->element + 1)));
			iterator->block->nb_element --;

			iterator->element --;
		}

		iterator->set->nb_element_tot --;
	}
}