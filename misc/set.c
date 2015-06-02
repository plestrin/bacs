#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "set.h"

struct set* set_create(uint32_t element_size, uint32_t nb_element_block){
	struct set* set;

	set = (struct set*)malloc(set_get_size(element_size, nb_element_block));
	if (set != NULL){
		set_init(set, element_size, nb_element_block)
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return set;
}

int32_t set_add(struct set* set, void* element){
	struct setBlock* block_cursor;

	for (block_cursor = &(set->block); block_cursor != NULL; block_cursor = block_cursor->next){
		if (block_cursor->nb_element < set->nb_element_block){
			memcpy(block_cursor->data + (block_cursor->nb_element * set->element_size), element, set->element_size);
			block_cursor->nb_element ++;
			set->nb_element_tot ++;
			return 0;
		}

		if (block_cursor->next == NULL){
			block_cursor->next = (struct setBlock*)malloc(setBlock_get_size(set->element_size, set->nb_element_block));
			if (block_cursor->next == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
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

	printf("ERROR: in %s, unable to find element %p in set\n", __func__, element);
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