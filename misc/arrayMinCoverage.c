#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arrayMinCoverage.h"
#include "base.h"

static int32_t categoryDesc_compare_nb_element(const void* arg1, const void* arg2){
	struct categoryDesc* desc1 = (struct categoryDesc*)arg1;
	struct categoryDesc* desc2 = (struct categoryDesc*)arg2;

	if (desc1->nb_element < desc2->nb_element){
		return -1;
	}
	else if (desc1->nb_element > desc2->nb_element){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t categoryDesc_compare_offset(const void* arg1, const void* arg2){
	struct categoryDesc* desc1 = (struct categoryDesc*)arg1;
	struct categoryDesc* desc2 = (struct categoryDesc*)arg2;

	if (desc1->offset < desc2->offset){
		return -1;
	}
	else if (desc1->offset > desc2->offset){
		return 1;
	}
	else{
		return 0;
	}
}

struct selection{
	uint32_t 	nb_element;
	void** 		buffer;
};

#define selection_init(selection) 									\
	(selection).nb_element = 0; 									\
	(selection).buffer = NULL;

static int32_t selection_clone(struct selection* selection_dst, struct selection* selection_src){
	selection_dst->nb_element = selection_src->nb_element;
	selection_dst->buffer = (void**)malloc(sizeof(void*) * selection_dst->nb_element);
	if (selection_dst->buffer == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	memcpy(selection_dst->buffer, selection_src->buffer, sizeof(void*) * selection_dst->nb_element);

	return 0;
}

static int32_t selection_add(struct selection* selection, struct array* array, int32_t(*compare)(void*,void*)){
	uint32_t* 	mapping;
	uint32_t 	i;
	void** 		new_buffer;
	uint32_t 	ptr1;
	uint32_t 	ptr2;
	int32_t 	diff;

	new_buffer = (void**)malloc(sizeof(void*) * (array_get_length(array) + selection->nb_element));
	if (new_buffer == NULL){
		log_err("unable to allocate memory");
		return -1;
	}
		
	mapping = array_create_mapping(array, compare);
	if (mapping == NULL){
		log_err("unable to create mapping");
		free(new_buffer);
		return -1;
	}

	for (i = 0, ptr1 = 0, ptr2 = 0; ptr1 < selection->nb_element || ptr2 < array_get_length(array); ){
		if (ptr1 < selection->nb_element){
			if (ptr2 < array_get_length(array)){
				diff = compare(selection->buffer[ptr1], array_get(array, mapping[ptr2]));
				if (diff < 0){
					new_buffer[i ++] = selection->buffer[ptr1 ++];
				}
				else if (diff > 0){
					if (i == 0 || compare(new_buffer[i - 1], array_get(array, mapping[ptr2])) != 0){
						new_buffer[i ++] = array_get(array, mapping[ptr2]);
					}
					ptr2 ++;
				}
				else{
					new_buffer[i ++] = selection->buffer[ptr1];

					ptr1 ++;
					ptr2 ++;
				}
			}
			else{
				new_buffer[i ++] = selection->buffer[ptr1 ++];
			}
		}
		else{
			if (i == 0 || compare(new_buffer[i - 1], array_get(array, mapping[ptr2])) != 0){
				new_buffer[i ++] = array_get(array, mapping[ptr2]);
			}
			ptr2 ++;
		}
	}
	
	if (selection->buffer != NULL){
		free(selection->buffer);
	}

	free(mapping);

	selection->nb_element = i;
	selection->buffer = new_buffer;

	return 0;
}

static int32_t selection_nb_new_element(struct selection* selection, struct array* array, int32_t(*compare)(void*,void*)){
	uint32_t* 	mapping;
	uint32_t 	i;
	uint32_t 	ptr1;
	uint32_t 	ptr2;
	int32_t 	diff;
		
	mapping = array_create_mapping(array, compare);
	if (mapping == NULL){
		log_err("unable to create mapping");
		return -1;
	}

	for (i = 0, ptr1 = 0, ptr2 = 0; ptr2 < array_get_length(array); ){
		if (ptr1 < selection->nb_element){
			diff = compare(selection->buffer[ptr1], array_get(array, mapping[ptr2]));
			if (diff < 0){
				ptr1 ++;
			}
			else if (diff > 0){
				if (ptr2 == 0 || compare(array_get(array, mapping[ptr2 - 1]), array_get(array, mapping[ptr2])) != 0){
					i++;
				}
				ptr2 ++;
			}
			else{
				ptr1 ++;
				ptr2 ++;
			}
		}
		else{
			if (ptr2 == 0 || compare(array_get(array, mapping[ptr2 - 1]), array_get(array, mapping[ptr2])) != 0){
				i++;
			}
			ptr2 ++;
		}
	}
	
	free(mapping);

	return i;
}

#define selection_clean(selection) 									\
	if ((selection).buffer != NULL){ 								\
		free((selection).buffer); 									\
	}

int32_t arrayMinCoverage_rand(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*), uint32_t* score){
	uint32_t 			i;
	struct selection 	selection;

	selection_init(selection)

	for (i = 0; i < nb_category; i++){
		desc_buffer[i].choice = rand() % desc_buffer[i].nb_element;
		if (selection_add(&selection, *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice), compare)){
			log_err("unable to add array to selection");
		}
	}

	if (score != NULL){
		*score = selection.nb_element;
	}

	selection_clean(selection)

	return 0;
}

int32_t arrayMinCoverage_greedy(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*), uint32_t* score){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			min_size;
	int32_t 			size;
	struct selection 	selection;

	selection_init(selection)

	qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);

	for (i = 0; i < nb_category; i++){
		for (j = 0, min_size = 0xffffffff, desc_buffer[i].choice = 0; j < desc_buffer[i].nb_element; j++){
			size = selection_nb_new_element(&selection, *(struct array**)array_get(array, desc_buffer[i].offset + j), compare);
			if (size < 0){
				log_err("unable to compare array to selection");
				continue;
			}

			if ((uint32_t)size < min_size){
				min_size = (uint32_t)size;
				desc_buffer[i].choice = j;
			}
		}
		
		if (selection_add(&selection, *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice), compare)){
			log_err("unable to add array to selection");
		}
	}

	qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_offset);

	if (score != NULL){
		*score = selection.nb_element;
	}

	selection_clean(selection)

	return 0;
}

int32_t arrayMinCoverage_exact(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*), uint32_t* score){
	struct selectionStack{
		uint32_t 			choice;
		struct selection 	selection;
	};

	uint32_t 				i;
	uint32_t 				j;
	struct selectionStack* 	selection_stack;
	uint32_t 				best_score = 0xffffffff;
	int32_t 				size;

	if (nb_category == 0){
		return 0;
	}
	else if (nb_category == 1){
		for (i = 0; i < desc_buffer[0].nb_element; i++){
			size = array_get_length(*(struct array**)array_get(array, desc_buffer[0].offset + i));
			if ((uint32_t)size < best_score){
				desc_buffer[0].choice = i;
			}
		}

		return 0;
	}
	else{
		selection_stack = (struct selectionStack*)malloc(sizeof(struct selectionStack) * nb_category);
		if (selection_stack == NULL){
			log_err("unable to allocate memory");
			return -1;
		}

		for (i = 0, selection_stack[0].choice = 0; ; ){
			if (selection_stack[i].choice < desc_buffer[i].nb_element){
				if (i + 1 >= nb_category){
					size = selection_nb_new_element(&(selection_stack[i - 1].selection), *(struct array**)array_get(array, desc_buffer[i].offset + selection_stack[i].choice), compare);
					if (size < 0){
						log_err_m("unable to process choice %u for category %u", selection_stack[i].choice, i);
					}
					else if (selection_stack[i - 1].selection.nb_element + (uint32_t)size < best_score){
						best_score = selection_stack[i - 1].selection.nb_element + (uint32_t)size;
						for (j = 0; j < nb_category; j++){
							desc_buffer[j].choice = selection_stack[j].choice;
						}
					}
					selection_stack[i].choice ++;
				}
				else if (i > 0){
					if (selection_clone(&(selection_stack[i].selection), &(selection_stack[i - 1].selection)) || selection_add(&(selection_stack[i].selection), *(struct array**)array_get(array, desc_buffer[i].offset + selection_stack[i].choice), compare)){
						log_err_m("unable to process choice %u for category %u", selection_stack[i].choice, i);
						selection_clean(selection_stack[i].selection)
						selection_stack[i].choice ++;
					}
					else if (selection_stack[i].selection.nb_element > best_score){
						selection_clean(selection_stack[i].selection)
						selection_stack[i].choice ++;
					}
					else{
						selection_stack[++ i].choice = 0;
					}
				}
				else{
					selection_init(selection_stack[i].selection)
					if (selection_add(&(selection_stack[i].selection), *(struct array**)array_get(array, desc_buffer[i].offset + selection_stack[i].choice), compare)){
						log_err_m("unable to process choice %u for category %u", selection_stack[i].choice, i);
						selection_clean(selection_stack[i].selection)
						selection_stack[i].choice ++;
					}
					else if (selection_stack[i].selection.nb_element > best_score){
						selection_clean(selection_stack[i].selection)
						selection_stack[i].choice ++;
					}
					else{
						selection_stack[++ i].choice = 0;
						continue;
					}
				}
			}
			else{
				if (i + 1 >= nb_category){
					i --;
					selection_clean(selection_stack[i].selection)
					selection_stack[i].choice ++;
				}
				else if (i > 0){
					i --;
					selection_clean(selection_stack[i].selection)
					selection_stack[i].choice ++;
				}
				else{
					break;
				}
			}
		}

		if (score != NULL){
			*score = best_score;
		}

		free(selection_stack);

		return 0;
	}
}
