#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "arrayMinCoverage.h"
#include "base.h"

static int32_t categoryDesc_compare_nb_element(const void* arg1, const void* arg2){
	const struct categoryDesc* desc1 = (const struct categoryDesc*)arg1;
	const struct categoryDesc* desc2 = (const struct categoryDesc*)arg2;

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

static struct tagMap* tagMap_create(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*)){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	struct array* 				element_array;
	uint32_t 					nb_tag 			= 0;
	void* 						tree_root 		= NULL;
	struct tagMapTreeToken* 	new_token 		= NULL;
	struct tagMapTreeToken** 	result;
	struct tagMap* 				tag_map 		= NULL;

	for (i = 0; i < nb_category; i++){
		if ((desc_buffer[i].tagMap_gateway = (void***)malloc(sizeof(void**) * desc_buffer[i].nb_element)) == NULL){
			log_err("unable to allocate memory");
			goto exit;
		}

		for (j = 0; j < desc_buffer[i].nb_element; j++){
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
			if (array_get_length(element_array) == 0){
				desc_buffer[i].tagMap_gateway[j] = NULL;
			}
			else{
				if ((desc_buffer[i].tagMap_gateway[j] = (void**)malloc(sizeof(void**) * array_get_length(element_array))) == NULL){
					log_err("unable to allocate memory");
					goto exit;
				}
				for (k = 0; k < array_get_length(element_array); k++){
					if (new_token == NULL){
						if ((new_token = (struct tagMapTreeToken*)malloc(sizeof(struct tagMapTreeToken))) == NULL){
							log_err("unable to allocate memory");
							goto exit;
						}
					}

					new_token->element = array_get(element_array, k);
					new_token->idx = nb_tag;

					result = tsearch(new_token, &tree_root, compare);
					if (result == NULL){
						log_err("tsearch failed to insert new item");
						goto exit;
					}
					else if (*result == new_token){
						new_token = NULL;
						nb_tag ++;
					}
					desc_buffer[i].tagMap_gateway[j][k] = *result;
				}
			}
		}
	}


	if (nb_tag){
		if ((tag_map = (struct tagMap*)malloc(sizeof(struct tagMap) + nb_tag * sizeof(uint32_t))) == NULL){
			log_err("unable to allocate memory");
			goto exit;
		}

		tag_map->nb_element = nb_tag;
		tag_map->map = (uint32_t*)(tag_map + 1);

		for (i = 0; i < nb_category; i++){
			for (j = 0; j < desc_buffer[i].nb_element; j++){
				element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
				for (k = 0; k < array_get_length(element_array); k++){
					desc_buffer[i].tagMap_gateway[j][k] = tag_map->map + ((struct tagMapTreeToken*)(desc_buffer[i].tagMap_gateway[j][k]))->idx;
				}
			}
		}
	}

	exit:
	if (new_token != NULL){
		free(new_token);
	}
	tdestroy(tree_root, free);

	return tag_map;
}

#define tagMap_deselect_all(tag_map) memset((tag_map)->map, 0, sizeof(uint32_t) * (tag_map)->nb_element)

static void tagMap_deselect(struct tagMap* tag_map, uint32_t selection_value){
	uint32_t i;

	for (i = 0; i < tag_map->nb_element; i++){
		if (tag_map->map[i] == selection_value){
			tag_map->map[i] = 0;
		}
	}
}

static void tagMap_delete(struct tagMap* tag_map, uint32_t nb_category, struct categoryDesc* desc_buffer){
	uint32_t i;
	uint32_t j;

	free(tag_map);

	for (i = 0; i < nb_category; i++){
		for (j = 0; j < desc_buffer[i].nb_element; j++){
			if (desc_buffer[i].tagMap_gateway[j] != NULL){
				free(desc_buffer[i].tagMap_gateway[j]);
			}
		}
		free(desc_buffer[i].tagMap_gateway);
	}
}

uint32_t arrayMinCoverage_rand(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value){
	uint32_t 		i;
	uint32_t 		j;
	struct array* 	element_array;
	uint32_t 		score;

	for (i = 0, score = 0; i < nb_category; i++){
		desc_buffer[i].choice = (uint32_t)rand() % desc_buffer[i].nb_element;

		element_array = *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice);
		for (j = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] == 0){
				*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] = selection_value;
				score ++;
			}
		}
	}

	return score;
}

uint32_t arrayMinCoverage_greedy(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		min_size;
	uint32_t 		size;
	struct array* 	element_array;
	uint32_t 		score;

	for (i = 0, score = 0; i < nb_category; i++){
		for (j = 0, min_size = 0xffffffff, desc_buffer[i].choice = 0; j < desc_buffer[i].nb_element; j++){
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
			if (array_get_length(element_array) == 0){
				continue;
			}

			for (k = 0, size = 0; k < array_get_length(element_array); k++){
				if (*(uint32_t*)desc_buffer[i].tagMap_gateway[j][k] == 0){
					size ++;
				}
			}
			if (size < min_size){
				min_size = size;
				desc_buffer[i].choice = j;
			}
		}

		element_array = *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice);
		for (j = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] == 0){
				*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] = selection_value;
				score ++;
			}
		}
	}

	return score;
}

uint32_t arrayMinCoverage_exact(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t best_score){
	uint32_t 		i;
	uint32_t 		j;
	struct array* 	element_array;
	uint32_t 		score;

	for (i = 0; i < desc_buffer[0].nb_element; i++){
		element_array = *(struct array**)array_get(array, desc_buffer[0].offset + i);
		for (j = 0, score = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] == 0){
				*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] = selection_value;
				score ++;
			}
		}
		if (score <= best_score){
			if (nb_category > 1){
				score += arrayMinCoverage_exact(array, nb_category - 1, desc_buffer + 1, selection_value + 1, best_score - score);
			}
			if (score < best_score){
				desc_buffer[0].choice = i;
				best_score = score;
			}
		}
		for (j = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] == selection_value){
				*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] = 0;
			}
		}
	}

	return best_score;
}

uint32_t arrayMinCoverage_reshape(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t* selection_value, struct tagMap* tag_map){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		l;
	uint32_t 		min_size;
	uint32_t 		size;
	struct array* 	element_array;
	uint32_t 		score;
	uint64_t 		complexity;
	uint32_t 		local_score;
	#if ARRAYMINCOVERAGE_DETERMINISTIC == 0
	uint32_t 		proba_offset;
	#endif

	for (i = 0, l = 0, score = 0, local_score = 0, complexity = 1; i < nb_category; i++){
		complexity = complexity * (uint64_t)desc_buffer[i].nb_element;
		if (complexity > ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD){
			if (i - l >= 2){
				tagMap_deselect(tag_map, *selection_value);
				local_score = arrayMinCoverage_exact(array, i - l, desc_buffer + l, *selection_value, local_score);
				for (j = l; j < i; j++){
					element_array = *(struct array**)array_get(array, desc_buffer[j].offset + desc_buffer[j].choice);
					for (k = 0; k < array_get_length(element_array); k++){
						if (*(uint32_t*)desc_buffer[j].tagMap_gateway[desc_buffer[j].choice][k] == 0){
							*(uint32_t*)desc_buffer[j].tagMap_gateway[desc_buffer[j].choice][k] = *selection_value;
						}
					}
				}
			}
			score += local_score;
			complexity = (uint64_t)desc_buffer[i].nb_element;
			*selection_value += 1;
			local_score = 0;
			l = i;
		}

		#if ARRAYMINCOVERAGE_DETERMINISTIC == 0
		proba_offset = 1;
		#endif

		for (j = 0, min_size = 0xffffffff, desc_buffer[i].choice = 0; j < desc_buffer[i].nb_element; j++){
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
			if (array_get_length(element_array) == 0){
				continue;
			}

			for (k = 0, size = 0; k < array_get_length(element_array); k++){
				if (*(uint32_t*)desc_buffer[i].tagMap_gateway[j][k] == 0){
					size ++;
				}
			}
			#if ARRAYMINCOVERAGE_DETERMINISTIC == 0
			if (size < min_size || (size == min_size && !(rand() % proba_offset))){
				if (size < min_size){
					proba_offset = 1;
				}
				else{
					proba_offset ++;
				}
				min_size = size;
				desc_buffer[i].choice = j;
			}
			#else
			if (size < min_size){
				min_size = size;
				desc_buffer[i].choice = j;
			}
			#endif
		}

		element_array = *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice);
		for (j = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] == 0){
				*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] = *selection_value;
				local_score ++;
			}
		}
	}

	if (nb_category - l >= 2){
		tagMap_deselect(tag_map, *selection_value);
		local_score = arrayMinCoverage_exact(array, nb_category - l, desc_buffer + l, *selection_value, local_score);
		for (j = l; j < nb_category; j++){
			element_array = *(struct array**)array_get(array, desc_buffer[j].offset + desc_buffer[j].choice);
			for (k = 0; k < array_get_length(element_array); k++){
				if (*(uint32_t*)desc_buffer[j].tagMap_gateway[desc_buffer[j].choice][k] == 0){
					*(uint32_t*)desc_buffer[j].tagMap_gateway[desc_buffer[j].choice][k] = *selection_value;
				}
			}
		}
	}
	*selection_value += 1;
	score += local_score;

	return score;
}

uint32_t arrayMinCoverage_auto(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t* selection_value, struct tagMap* tag_map){
	uint32_t 		i;
	uint32_t 		j;
	struct array* 	element_array;
	uint32_t 		score;
	uint64_t 		complexity;

	if ((complexity = categoryDesc_get_complexity_est(desc_buffer, nb_category)) <= ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD){
		#ifdef VERBOSE
		log_info_m("exact solution for %u categories", nb_category);
		#endif

		score = arrayMinCoverage_greedy(array, nb_category, desc_buffer, *selection_value);
		tagMap_deselect(tag_map, *selection_value);
		score = arrayMinCoverage_exact(array, nb_category, desc_buffer, *selection_value, score);
		for (i = 0; i < nb_category; i++){
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice);
			for (j = 0; j < array_get_length(element_array); j++){
				if (*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] == 0){
					*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] = *selection_value;
				}
			}
		}
		*selection_value += 1;
	}
	else{
		#ifdef VERBOSE
		log_info_m("reshape solution for %u categories (complexity is at least %llu)", nb_category, complexity);
		#endif

		score = arrayMinCoverage_reshape(array, nb_category, desc_buffer, selection_value, tag_map);
	}

	return score;
}

uint32_t arrayMinCoverage_split(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t* selection_value, struct tagMap* tag_map){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	uint32_t 				l;
	struct array* 			element_array;
	struct categoryDesc 	tmp;
	uint32_t 				score;

	for (i = 0, score = 0; i < nb_category; i++){
		if (desc_buffer[i].nb_element == 0){
			continue;
		}
		else if (desc_buffer[i].nb_element == 1){
			desc_buffer[i].choice = 0;
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset);
			for (j = 0; j < array_get_length(element_array); j++){
				if (*(uint32_t*)desc_buffer[i].tagMap_gateway[0][j] == 0){
					*(uint32_t*)desc_buffer[i].tagMap_gateway[0][j] = *selection_value;
					score ++;
				}
			}
		}
		else{
			break;
		}
	}

	nb_category -= i;
	desc_buffer += i;
	*selection_value += 1;

	while (nb_category){
		for (i = 0; i < desc_buffer[0].nb_element; i++){
			element_array = *(struct array**)array_get(array, desc_buffer[0].offset + i);
			for (j = 0; j < array_get_length(element_array); j++){
				if (*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] == 0){
					*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] = *selection_value;
				}
			}
		}

		for (i = 1, l = 0; i < nb_category && l < nb_category - i; ){
			for (j = 0; j < desc_buffer[i].nb_element; j++){
				element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
				for (k = 0; k < array_get_length(element_array); k++){
					if (*(uint32_t*)desc_buffer[i].tagMap_gateway[j][k] == *selection_value){
						goto add_category;
					}
				}
			}

			l ++;
			if (i < nb_category - l){
				memcpy(&tmp, desc_buffer + i, sizeof(struct categoryDesc));
				memcpy(desc_buffer + i, desc_buffer + nb_category - l, sizeof(struct categoryDesc));
				memcpy(desc_buffer + nb_category - l, &tmp, sizeof(struct categoryDesc));
			}
			continue;

			add_category:
			for (j = 0; j < desc_buffer[i].nb_element; j++){
				element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
				for (k = 0; k < array_get_length(element_array); k++){
					if (*(uint32_t*)desc_buffer[i].tagMap_gateway[j][k] == 0){
						*(uint32_t*)desc_buffer[i].tagMap_gateway[j][k] = *selection_value;
					}
				}
			}
			i ++;
			l = 0;
		}

		qsort(desc_buffer, i, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);
		tagMap_deselect(tag_map, *selection_value);
		score += arrayMinCoverage_auto(array, i, desc_buffer, selection_value, tag_map);

		nb_category -= i;
		desc_buffer += i;
	}

	return score;
}

static uint32_t* arrayMinCoverage_save_order(struct categoryDesc* desc_buffer, uint32_t nb_category);
static void arrayMinCoverage_restore_order(struct categoryDesc* desc_buffer, uint32_t nb_category, uint32_t* order);

int32_t arrayMinCoverage_rand_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 	tag_map 		= NULL;
	int32_t 		result 			= -1;
	uint32_t 		local_score 	= 0;

	if (nb_category == 0){
		result = 0;
	}
	else{
		if ((tag_map = tagMap_create(array, nb_category, desc_buffer, compare)) == NULL){
			log_err("unable to create tagMap");
			goto exit;
		}

		tagMap_deselect_all(tag_map);

		local_score = arrayMinCoverage_rand(array, nb_category, desc_buffer, 1);
		result = 0;
	}

	exit:
	if (score != NULL){
		*score = local_score;
	}
	if (tag_map != NULL){
		tagMap_delete(tag_map, nb_category, desc_buffer);
	}

	return result;
}

int32_t arrayMinCoverage_greedy_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 	tag_map 		= NULL;
	uint32_t* 		order 			= NULL;
	int32_t 		result 			= -1;
	uint32_t 		local_score 	= 0;

	if (nb_category == 0){
		result = 0;
	}
	else{
		if ((tag_map = tagMap_create(array, nb_category, desc_buffer, compare)) == NULL){
			log_err("unable to create tagMap");
			goto exit;
		}

		tagMap_deselect_all(tag_map);

		if ((order = arrayMinCoverage_save_order(desc_buffer, nb_category)) == NULL){
			log_err("unable to save order");
			goto exit;
		}

		qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);
		local_score = arrayMinCoverage_greedy(array, nb_category, desc_buffer, 1);
		result = 0;
	}

	exit:
	if (score != NULL){
		*score = local_score;
	}
	if (order != NULL){
		arrayMinCoverage_restore_order(desc_buffer, nb_category, order);
	}
	if (tag_map != NULL){
		tagMap_delete(tag_map, nb_category, desc_buffer);
	}

	return result;
}

int32_t arrayMinCoverage_exact_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 	tag_map 		= NULL;
	uint32_t* 		order 			= NULL;
	int32_t 		result 			= -1;
	uint32_t 		local_score 	= 0;

	if (nb_category == 0){
		result = 0;
	}
	else{
		if ((tag_map = tagMap_create(array, nb_category, desc_buffer, compare)) == NULL){
			log_err("unable to create tagMap");
			goto exit;
		}

		tagMap_deselect_all(tag_map);

		if ((order = arrayMinCoverage_save_order(desc_buffer, nb_category)) == NULL){
			log_err("unable to save order");
			goto exit;
		}

		qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);
		local_score = arrayMinCoverage_greedy(array, nb_category, desc_buffer, 1);

		tagMap_deselect_all(tag_map);

		local_score = arrayMinCoverage_exact(array, nb_category, desc_buffer, 1, local_score);
		result = 0;
	}

	exit:
	if (score != NULL){
		*score = local_score;
	}
	if (order != NULL){
		arrayMinCoverage_restore_order(desc_buffer, nb_category, order);
	}
	if (tag_map != NULL){
		tagMap_delete(tag_map, nb_category, desc_buffer);
	}

	return result;
}

int32_t arrayMinCoverage_reshape_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 	tag_map 		= NULL;
	uint32_t* 		order 			= NULL;
	int32_t 		result 			= -1;
	uint32_t 		local_score 	= 0;
	uint32_t 		selection_value = 1;

	if (nb_category == 0){
		result = 0;
	}
	else{
		if ((tag_map = tagMap_create(array, nb_category, desc_buffer, compare)) == NULL){
			log_err("unable to create tagMap");
			goto exit;
		}

		tagMap_deselect_all(tag_map);

		if ((order = arrayMinCoverage_save_order(desc_buffer, nb_category)) == NULL){
			log_err("unable to save order");
			goto exit;
		}

		qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);
		local_score = arrayMinCoverage_reshape(array, nb_category, desc_buffer, &selection_value, tag_map);
		result = 0;
	}

	exit:
	if (score != NULL){
		*score = local_score;
	}
	if (order != NULL){
		arrayMinCoverage_restore_order(desc_buffer, nb_category, order);
	}
	if (tag_map != NULL){
		tagMap_delete(tag_map, nb_category, desc_buffer);
	}

	return result;
}

int32_t arrayMinCoverage_split_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 	tag_map 		= NULL;
	uint32_t* 		order 			= NULL;
	int32_t 		result 			= -1;
	uint32_t 		local_score 	= 0;
	uint32_t 		selection_value = 1;

	if (nb_category == 0){
		result = 0;
	}
	else{
		if ((tag_map = tagMap_create(array, nb_category, desc_buffer, compare)) == NULL){
			log_err("unable to create tagMap");
			goto exit;
		}

		tagMap_deselect_all(tag_map);

		if ((order = arrayMinCoverage_save_order(desc_buffer, nb_category)) == NULL){
			log_err("unable to save order");
			goto exit;
		}

		qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);
		local_score = arrayMinCoverage_split(array, nb_category, desc_buffer, &selection_value, tag_map);
		result = 0;
	}

	exit:
	if (score != NULL){
		*score = local_score;
	}
	if (order != NULL){
		arrayMinCoverage_restore_order(desc_buffer, nb_category, order);
	}
	if (tag_map != NULL){
		tagMap_delete(tag_map, nb_category, desc_buffer);
	}

	return result;
}

uint32_t arrayMinCoverage_eval(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*)){
	uint32_t 					i;
	uint32_t 					j;
	struct array* 				element_array;
	uint32_t 					nb_tag 			= 0;
	void* 						tree_root 		= NULL;
	struct tagMapTreeToken* 	new_token 		= NULL;
	struct tagMapTreeToken** 	result;

	for (i = 0; i < nb_category; i++){
		element_array = *(struct array**)array_get(array, desc_buffer[i].offset + desc_buffer[i].choice);
		for (j = 0; j < array_get_length(element_array); j++){
			if (new_token == NULL){
				if ((new_token = (struct tagMapTreeToken*)malloc(sizeof(struct tagMapTreeToken))) == NULL){
					log_err("unable to allocate memory");
					goto exit;
				}
			}

			new_token->element = array_get(element_array, j);
			new_token->idx = nb_tag;

			result = tsearch(new_token, &tree_root, compare);
			if (result == NULL){
				log_err("tsearch failed to insert new item");
				goto exit;
			}
			else if (*result == new_token){
				new_token = NULL;
				nb_tag ++;
			}
		}
	}

	exit:
	if (new_token != NULL){
		free(new_token);
	}
	tdestroy(tree_root, free);

	return nb_tag;
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
