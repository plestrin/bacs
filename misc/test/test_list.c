#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../list.h"
#include "../base.h"

#define NB_ELEMENT_LIST 32
#define MODIFY_INDEX 	8

int32_t main(void){
	struct list 		list;
	uint32_t 			i;
	struct listIterator it;

	list_init(list, sizeof(uint32_t));

	for (i = 0; i < NB_ELEMENT_LIST; i++){
		if (list_add_tail(&list, &i) < 0){
			log_err("unable to add element to list");
			return EXIT_FAILURE;
		}
	}

	for (listIterator_init(&it, &list); listIterator_get_next(&it) != NULL; ){
		if (it.index != *(uint32_t*)listIterator_get_data(it)){
			log_err_m("iterator error @ %u", it.index);
			return EXIT_FAILURE;
		}
	}

	for (listIterator_init(&it, &list); listIterator_get_next(&it) != NULL; ){
		if (it.index == MODIFY_INDEX){
			listIterator_pop_next(&it);
			break;
		}
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