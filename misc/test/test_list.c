#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../list.h"
#include "../base.h"

#define NB_ELEMENT_LIST 32
#define MODIFY_INDEX 8

int32_t main(void){
	struct list 		list;
	uint32_t 			i;
	struct listIterator it;

	list_init(list, sizeof(uint32_t));

	for (i = 0; i < NB_ELEMENT_LIST; i++){
		if (list_add_tail(&list, &i) == NULL){
			log_err("unable to add element to list");
			return EXIT_FAILURE;
		}
	}

	if (list_get_length(&list) != NB_ELEMENT_LIST){
		log_err_m("incorrect list size: %u", list_get_length(&list));
		return EXIT_FAILURE;
	}

	for (listIterator_init(&it, &list); listIterator_get_next(&it) != NULL; ){
		if (it.index != *(uint32_t*)listIterator_get_data(it)){
			log_err_m("iterator error @ %u", it.index);
			return EXIT_FAILURE;
		}
	}
	for (listIterator_init(&it, &list); listIterator_get_prev(&it) != NULL; ){
		if (it.index != *(uint32_t*)listIterator_get_data(it)){
			log_err_m("iterator error @ %u", it.index);
			return EXIT_FAILURE;
		}
	}

	if (listIterator_get_index(&it, &list, MODIFY_INDEX) != NULL){
		if (MODIFY_INDEX != *(uint32_t*)listIterator_get_data(it)){
			log_err_m("iterator error after get index: %u", *(uint32_t*)listIterator_get_data(it));
			return EXIT_FAILURE;
		}
		listIterator_pop_next(&it);
	}
	else{
		log_err_m("unable to get index %u", MODIFY_INDEX);
		return EXIT_FAILURE;
	}

	for (listIterator_init(&it, &list); listIterator_get_next(&it) != NULL; ){
		if (it.index == MODIFY_INDEX){
			listIterator_push_prev(&it, &(it.index));
			break;
		}
	}

	for (listIterator_init(&it, &list); listIterator_get_next(&it) != NULL; ){
		if (it.index != *(uint32_t*)listIterator_get_data(it)){
			log_err_m("iterator error @ %u", it.index);
			return EXIT_FAILURE;
		}
	}

	list_clean(&list);

	return EXIT_SUCCESS;
}