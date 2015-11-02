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

static uint64_t categoryDesc_get_complexity_est(struct categoryDesc* desc_buffer, uint32_t nb_category){
	uint64_t result = 1;
	uint32_t i;

	for (i = 0; i < nb_category; i++){
		result = result * (uint64_t)desc_buffer[i].nb_element;
		if (result & 0xffffffff00000000ULL){
			return result;
		}
	}

	return result;

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

static int32_t selection_nb_new_element(const struct selection* selection, struct array* array, int32_t(*compare)(void*,void*)){
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

static int32_t selection_intersect(const struct selection* selection1, struct selection* selection2, int32_t(*compare)(void*,void*)){
	uint32_t 	ptr1;
	uint32_t 	ptr2;
	int32_t 	diff;

	for (ptr1 = 0, ptr2 = 0; ptr1 < selection1->nb_element && ptr2 < selection2->nb_element; ){
		diff = compare(selection1->buffer[ptr1], selection2->buffer[ptr2]);
		if (diff < 0){
			ptr1 ++;
		}
		else if (diff > 0){
			ptr2 ++;
		}
		else{
			return 1;
		}
	}

	return 0;
}

static void selection_remove(struct selection* selection_dst, const struct selection* selection_src, int32_t(*compare)(void*,void*)){
	uint32_t 	ptr_dst;
	uint32_t 	ptr_src;
	int32_t 	diff;
	uint32_t 	rewrite_ptr_dst;

	for (ptr_dst = 0, ptr_src = 0, rewrite_ptr_dst = 0; ptr_dst < selection_dst->nb_element && ptr_src < selection_src->nb_element; ){
		diff = compare(selection_dst->buffer[ptr_dst], selection_src->buffer[ptr_src]);
		if (diff < 0){
			selection_dst->buffer[rewrite_ptr_dst ++] = selection_dst->buffer[ptr_dst ++];
		}
		else if (diff > 0){
			ptr_src ++;
		}
		else{
			ptr_src ++;
			ptr_dst ++;
		}
	}

	memmove(selection_dst->buffer + rewrite_ptr_dst, selection_dst->buffer + ptr_dst, sizeof(void*) * (selection_dst->nb_element - ptr_dst));

	selection_dst->nb_element = rewrite_ptr_dst + (selection_dst->nb_element - ptr_dst);
}

#define selection_clean(selection) 									\
	if ((selection).buffer != NULL){ 								\
		free((selection).buffer); 									\
	}

static uint32_t arrayMinCoverage_get_array_nb_element(struct array* array, int32_t(*compare)(void*,void*)){
	uint32_t* 	mapping;
	uint32_t 	i;
	uint32_t 	result;

	mapping = array_create_mapping(array, compare);
	if (mapping == NULL){
		log_err("unable to create mapping");
		return array_get_length(array);
	}

	for (i = 1, result = min(1, array_get_length(array)); i < array_get_length(array); i++){
		if (compare(array_get(array, mapping[i]), array_get(array, mapping[i - 1])) != 0)
		{
			result ++;
		}
	}

	free(mapping);

	return result;
}

static uint32_t* arrayMinCoverage_save_order(struct categoryDesc* desc_buffer, uint32_t nb_category){
	uint32_t* 	order;
	uint32_t 	i;

	order = (uint32_t*)malloc(sizeof(uint32_t) * nb_category);
	if (order == NULL){
		log_err("unable to allocate memory");
	}
	else{
		for (i = 0; i < nb_category; i++){
			order[i] = desc_buffer[i].offset;
		}
	}

	return order;
}

static void arrayMinCoverage_restore_order(struct categoryDesc* desc_buffer, uint32_t nb_category, uint32_t* order){
	uint32_t 				i;
	uint32_t 				j;
	struct categoryDesc 	tmp_desc;

	if (order == NULL){
		log_err("empty order record");
		return;
	}

	for (i = 0; i < nb_category; i++){
		for (j = i; j < nb_category; j++){
			if (desc_buffer[j].offset == order[i]){
				break;
			}
		}
		if (j == nb_category){
			log_err("unable to restore order");
			break;
		}

		memcpy(&tmp_desc, desc_buffer + i, sizeof(struct categoryDesc));
		memmove(desc_buffer + i, desc_buffer + j, sizeof(struct categoryDesc));
		memcpy(desc_buffer + j, &tmp_desc, sizeof(struct categoryDesc));
	}

	free(order);
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
	uint32_t* 			order;

	selection_init(selection)

	order = arrayMinCoverage_save_order(desc_buffer, nb_category);

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

	arrayMinCoverage_restore_order(desc_buffer, nb_category, order);

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
		if (score != NULL){
			*score = 0;
		}

		return 0;
	}
	else if (nb_category == 1){
		for (i = 0; i < desc_buffer[0].nb_element; i++){
			size = arrayMinCoverage_get_array_nb_element(*(struct array**)array_get(array, desc_buffer[0].offset + i), compare);
			if ((uint32_t)size < best_score){
				desc_buffer[0].choice = i;
				best_score = size;
			}
		}

		if (score != NULL){
			*score = best_score;
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

int32_t arrayMinCoverage_auto(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*)){
	if (categoryDesc_get_complexity_est(desc_buffer, nb_category) <= ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD){
		#ifdef VERBOSE
		log_info_m("exact solution for %u categories", nb_category);
		#endif
		return arrayMinCoverage_exact(array, nb_category, desc_buffer, compare, NULL);
	}
	else{
		#ifdef VERBOSE
		log_info_m("greedy solution for %u categories", nb_category);
		#endif
		return arrayMinCoverage_greedy(array, nb_category, desc_buffer, compare, NULL);
	}
}

struct categoryClusteringData{
	struct selection 	selection;
	uint32_t 			state;
};

#define CATEGORY_CLUSTER_STATE_IDLE 0
#define CATEGORY_CLUSTER_STATE_CURR 1
#define CATEGORY_CLUSTER_STATE_DONE 2

static uint32_t arrayMinCoverage_find_intersect_recursive(struct categoryClusteringData* ccd_buffer, uint32_t nb_category, uint32_t element, int32_t(*compare)(void*,void*)){
	uint32_t i;
	uint32_t result;

	ccd_buffer[element].state = CATEGORY_CLUSTER_STATE_CURR;

	for (i = 0, result = 1; i < nb_category; i++){
		if (ccd_buffer[i].state == CATEGORY_CLUSTER_STATE_IDLE && selection_intersect(&(ccd_buffer[element].selection), &(ccd_buffer[i].selection), compare)){
			result += arrayMinCoverage_find_intersect_recursive(ccd_buffer, nb_category, i, compare);
		}
	}

	return result;
}

int32_t arrayMinCoverage_split(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*)){
	struct categoryClusteringData* 	ccd_buffer;
	uint32_t 						i;
	uint32_t 						j;
	uint32_t 						k;
	uint32_t 						cluster_nb_category;
	struct categoryDesc* 			cluster_desc_buffer;
	uint32_t 						nb_remaining_category;
	struct selection 				one_choice_selection;
	struct categoryDesc* 			remaining_desc_buffer;
	uint32_t* 						order;

	selection_init(one_choice_selection)

	order = arrayMinCoverage_save_order(desc_buffer, nb_category);

	qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);

	for (i = 0; i < nb_category; i++){
		if (desc_buffer[i].nb_element == 1){
			desc_buffer[i].choice = 0;
			if (selection_add(&one_choice_selection, *(struct array**)array_get(array, desc_buffer[i].offset), compare)){
				log_err("unable to add array to selection");
			}
		}
		else if (desc_buffer[i].nb_element > 1){
			break;
		}
	}

	nb_remaining_category = nb_category - i;
	remaining_desc_buffer = desc_buffer + i;

	if (nb_remaining_category){
		ccd_buffer = calloc(nb_remaining_category, sizeof(struct categoryClusteringData));
		if (ccd_buffer == NULL){
			log_err("unable to allocate memory");
			return -1;
		}

		for (i = 0; i < nb_remaining_category; i++){
			for (j = 0; j < remaining_desc_buffer[i].nb_element; j++){
				if (selection_add(&(ccd_buffer[i].selection), *(struct array**)array_get(array, remaining_desc_buffer[i].offset + j), compare)){
					log_err("unable to add array to selection");
				}
			}
			selection_remove(&(ccd_buffer[i].selection), &one_choice_selection, compare);
		}

		selection_clean(one_choice_selection)

		for (i = 0; i < nb_remaining_category; i++){
			if (ccd_buffer[i].state == CATEGORY_CLUSTER_STATE_IDLE){
				cluster_nb_category = arrayMinCoverage_find_intersect_recursive(ccd_buffer + i, nb_remaining_category - i, 0, compare) + (nb_category - nb_remaining_category);

				cluster_desc_buffer = (struct categoryDesc*)malloc(sizeof(struct categoryDesc) * cluster_nb_category);
				if (cluster_desc_buffer == NULL){
					log_err("unable to allocate memory");
				}
				else{
					memcpy(cluster_desc_buffer, desc_buffer, sizeof(struct categoryDesc) * (nb_category - nb_remaining_category));
					k = nb_category - nb_remaining_category;

					for (j = i; j < nb_remaining_category; j++){
						if (ccd_buffer[j].state == CATEGORY_CLUSTER_STATE_CURR){
							memcpy(cluster_desc_buffer + k, remaining_desc_buffer + j, sizeof(struct categoryDesc));
							k ++;
						}
					}

					if (arrayMinCoverage_auto(array, cluster_nb_category, cluster_desc_buffer, compare)){
						log_err("arrayMinCoverage_auto returned an error code");
					}

					for (j = i, k = nb_category - nb_remaining_category; j < nb_remaining_category; j++){
						if (ccd_buffer[j].state == CATEGORY_CLUSTER_STATE_CURR){
							remaining_desc_buffer[j].choice = cluster_desc_buffer[k ++].choice;
							ccd_buffer[j].state = CATEGORY_CLUSTER_STATE_DONE;
						}
					}

					free(cluster_desc_buffer);
				}
			}

			selection_clean(ccd_buffer[i].selection);
		}

		free(ccd_buffer);
	}

	arrayMinCoverage_restore_order(desc_buffer, nb_category, order);

	return 0;
}

uint32_t arrayMinCoverage_eval(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*)){
	struct selection 	selection;
	uint32_t 			result;
	uint32_t 			i;

	selection_init(selection)

	for (i = 0; i < nb_category; i++){
		if (selection_add(&selection, *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice), compare)){
			log_err("unable to add array to selection");
		}
	}

	result = selection.nb_element;

	selection_clean(selection);

	return result;
}
