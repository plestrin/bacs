#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "arrayMinCoverage.h"
#include "base.h"

typedef uint32_t(*arrayMinCoverage_search)(struct array*,uint32_t,struct categoryDesc*,uint32_t,uint32_t,struct tagMap*,void(*)(void));

static uint32_t    arrayMinCoverage_rand(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value);
static uint32_t  arrayMinCoverage_greedy(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value);
static uint32_t   arrayMinCoverage_start(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map, arrayMinCoverage_search arrayMinCoverage_next[2]);
static uint32_t   arrayMinCoverage_exact(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score);
static uint32_t   arrayMinCoverage_split(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map, arrayMinCoverage_search arrayMinCoverage_next);
static uint32_t   arrayMinCoverage_super(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map);
static uint32_t arrayMinCoverage_reshape(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map);
static uint32_t    arrayMinCoverage_auto(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map);

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
		desc_buffer[i].tagMap_gateway = NULL;
	}

	for (i = 0; i < nb_category; i++){
		if ((desc_buffer[i].tagMap_gateway = (void***)calloc(desc_buffer[i].nb_element, sizeof(void**))) == NULL){
			log_err("unable to allocate memory");
			goto exit;
		}

		for (j = 0; j < desc_buffer[i].nb_element; j++){
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
			if (!array_get_length(element_array)){
				desc_buffer[i].tagMap_gateway[j] = NULL;
			}
			else{
				if ((desc_buffer[i].tagMap_gateway[j] = (void**)malloc(array_get_length(element_array) * sizeof(void*))) == NULL){
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

#define tagMap_delete(tag_map) free(tag_map)

#define categoryDesc_swap(cat_desc1, cat_desc2) 								\
	{ 																			\
		struct categoryDesc __tmp__; 											\
																				\
		memcpy(&__tmp__, cat_desc1, sizeof(struct categoryDesc)); 				\
		memcpy(cat_desc1, cat_desc2, sizeof(struct categoryDesc)); 				\
		memcpy(cat_desc2, &__tmp__, sizeof(struct categoryDesc)); 				\
	}

static uint64_t categoryDesc_get_complexity_est(uint32_t nb_category, const struct categoryDesc* desc_buffer){
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

static void categoryDesc_clean(uint32_t nb_category, struct categoryDesc* desc_buffer){
	uint32_t i;
	uint32_t j;

	for (i = 0; i < nb_category; i++){
		if (desc_buffer[i].tagMap_gateway == NULL){
			continue;
		}

		for (j = 0; j < desc_buffer[i].nb_element; j++){
			if (desc_buffer[i].tagMap_gateway[j] != NULL){
				free(desc_buffer[i].tagMap_gateway[j]);
			}
		}
		free(desc_buffer[i].tagMap_gateway);
	}
}

static uint32_t arrayMinCoverage_rand(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value){
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

static uint32_t arrayMinCoverage_greedy(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		min_size;
	uint32_t 		size;
	struct array* 	element_array;
	uint32_t 		score;

	qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);

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

static uint32_t arrayMinCoverage_start(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map, arrayMinCoverage_search arrayMinCoverage_next[2]){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	struct array* 	element_array;
	uint32_t 		score;

	for (i = 0, k = 0, score = 0; i < nb_category; i++){
		if (desc_buffer[i].nb_element > 1){
			continue;
		}

		if (desc_buffer[i].nb_element == 1){
			desc_buffer[i].choice = 0;
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset);
			for (j = 0; j < array_get_length(element_array); j++){
				if (!*(uint32_t*)desc_buffer[i].tagMap_gateway[0][j]){
					*(uint32_t*)desc_buffer[i].tagMap_gateway[0][j] = selection_value;
					score ++;
				}
			}
		}

		if (i != k){
			categoryDesc_swap(desc_buffer + i, desc_buffer + k)
		}
		k ++;
	}

	if (k == nb_category){
		return score;
	}
	else if (k + 1== nb_category){
		return score + arrayMinCoverage_exact(array, 1, desc_buffer + k, selection_value + 1, min_score - score);
	}
	else{
		return score + arrayMinCoverage_next[0](array, nb_category - k, desc_buffer + k, selection_value + 1, min_score - score, tag_map, (void(*)(void))arrayMinCoverage_next[1]);
	}
}

static uint32_t arrayMinCoverage_super(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map){
	uint32_t 		i;
	uint32_t 		j;
	struct array* 	element_array;
	uint32_t 		score;

	for (i = 0; i < desc_buffer[0].nb_element; i++){
		element_array = *(struct array**)array_get(array, desc_buffer[0].offset + i);
		for (j = 0, score = 0; j < array_get_length(element_array); j++){
			if (!*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j]){
				*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] = selection_value;
				score ++;
			}
		}
		if (score < min_score){
			if (nb_category == 2){
				score += arrayMinCoverage_exact(array, nb_category - 1, desc_buffer + 1, selection_value + 1, min_score - score);
			}
			else if (nb_category > 2){
				score += arrayMinCoverage_split(array, nb_category - 1, desc_buffer + 1, selection_value + 1, min_score - score, tag_map, (arrayMinCoverage_search)arrayMinCoverage_super);
			}

			if (score < min_score){
				desc_buffer[0].choice = i;
				min_score = score;
			}
		}
		for (j = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] == selection_value){
				*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] = 0;
			}
		}
	}

	return min_score;
}

static uint32_t arrayMinCoverage_exact(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score){
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
		if (score < min_score){
			if (nb_category > 1){
				score += arrayMinCoverage_exact(array, nb_category - 1, desc_buffer + 1, selection_value + 1, min_score - score);
			}
			if (score < min_score){
				desc_buffer[0].choice = i;
				min_score = score;
			}
		}
		for (j = 0; j < array_get_length(element_array); j++){
			if (*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] == selection_value){
				*(uint32_t*)desc_buffer[0].tagMap_gateway[i][j] = 0;
			}
		}
	}

	return min_score;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static uint32_t arrayMinCoverage_reshape(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map){
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

	qsort(desc_buffer, nb_category, sizeof(struct categoryDesc), categoryDesc_compare_nb_element);

	for (i = 0, l = 0, score = 0, local_score = 0, complexity = 1; i < nb_category; i++){
		complexity = complexity * (uint64_t)desc_buffer[i].nb_element;
		if (complexity > ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD){
			if (i - l >= 2){
				tagMap_deselect(tag_map, selection_value);
				local_score = arrayMinCoverage_exact(array, i - l, desc_buffer + l, selection_value, local_score);
				for (j = l; j < i; j++){
					element_array = *(struct array**)array_get(array, desc_buffer[j].offset + desc_buffer[j].choice);
					for (k = 0; k < array_get_length(element_array); k++){
						if (!*(uint32_t*)desc_buffer[j].tagMap_gateway[desc_buffer[j].choice][k]){
							*(uint32_t*)desc_buffer[j].tagMap_gateway[desc_buffer[j].choice][k] = selection_value;
						}
					}
				}
			}
			score += local_score;
			complexity = (uint64_t)desc_buffer[i].nb_element;
			selection_value ++;
			local_score = 0;
			l = i;
		}

		#if ARRAYMINCOVERAGE_DETERMINISTIC == 0
		proba_offset = 2;
		#endif

		for (j = 0, min_size = 0xffffffff, desc_buffer[i].choice = 0; j < desc_buffer[i].nb_element; j++){
			element_array = *(struct array**)array_get(array, desc_buffer[i].offset + j);
			for (k = 0, size = 0; k < array_get_length(element_array); k++){
				if (!*(uint32_t*)desc_buffer[i].tagMap_gateway[j][k]){
					size ++;
				}
			}
			#if ARRAYMINCOVERAGE_DETERMINISTIC == 0
			if (size < min_size || (size == min_size && !(rand() % proba_offset))){
				if (size < min_size){
					proba_offset = 2;
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
			if (!*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j]){
				*(uint32_t*)desc_buffer[i].tagMap_gateway[desc_buffer[i].choice][j] = selection_value;
				local_score ++;
			}
		}
	}

	if (nb_category - l >= 2){
		tagMap_deselect(tag_map, selection_value);
		local_score = arrayMinCoverage_exact(array, nb_category - l, desc_buffer + l, selection_value, local_score);
	}

	score += local_score;

	return score;
}

static uint32_t arrayMinCoverage_auto(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map){
	uint64_t complexity;

	if ((complexity = categoryDesc_get_complexity_est(nb_category, desc_buffer)) <= ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD){
		#ifdef VERBOSE
		log_info_m("exact solution for %u categories", nb_category);
		#endif

		return arrayMinCoverage_exact(array, nb_category, desc_buffer, selection_value, min_score);
	}

	#ifdef VERBOSE
	log_info_m("reshape solution for %u categories (complexity is at least %llu)", nb_category, complexity);
	#endif

	return arrayMinCoverage_reshape(array, nb_category, desc_buffer, selection_value, min_score, tag_map);
}

static uint32_t arrayMinCoverage_split(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, uint32_t selection_value, uint32_t min_score, struct tagMap* tag_map, arrayMinCoverage_search arrayMinCoverage_next){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	uint32_t 				l;
	uint32_t 				m;
	uint32_t 				score;
	struct categoryDesc* 	copy_desc_buffer;
	struct array* 			element_array;

	if ((copy_desc_buffer = (struct categoryDesc*)malloc(nb_category * sizeof(struct categoryDesc))) == NULL){
		log_err("unable to allocate memory");
		return arrayMinCoverage_exact(array, nb_category, desc_buffer, selection_value, min_score);
	}

	memcpy(copy_desc_buffer, desc_buffer, sizeof(struct categoryDesc) * nb_category);

	for (m = 0, score = 0; m < nb_category && score < min_score; m = i){
		for (j = 0; j < copy_desc_buffer[m].nb_element; j++){
			element_array = *(struct array**)array_get(array, copy_desc_buffer[m].offset + j);
			for (k = 0; k < array_get_length(element_array); k++){
				if (!*(uint32_t*)copy_desc_buffer[m].tagMap_gateway[j][k]){
					*(uint32_t*)copy_desc_buffer[m].tagMap_gateway[j][k] = selection_value;
				}
			}
		}

		for (i = m + 1, l = 0; l < nb_category - i; ){
			for (j = 0; j < copy_desc_buffer[i].nb_element; j++){
				element_array = *(struct array**)array_get(array, copy_desc_buffer[i].offset + j);
				for (k = 0; k < array_get_length(element_array); k++){
					if (*(uint32_t*)copy_desc_buffer[i].tagMap_gateway[j][k] == selection_value){
						goto add_selection;
					}
				}
			}

			l ++;
			if (i < nb_category - l){
				categoryDesc_swap(copy_desc_buffer + i, copy_desc_buffer + nb_category - l)
			}
			continue;

			add_selection:
			for (j = 0; j < copy_desc_buffer[i].nb_element; j++){
				element_array = *(struct array**)array_get(array, copy_desc_buffer[i].offset + j);
				for (k = 0; k < array_get_length(element_array); k++){
					if (!*(uint32_t*)copy_desc_buffer[i].tagMap_gateway[j][k]){
						*(uint32_t*)copy_desc_buffer[i].tagMap_gateway[j][k] = selection_value;
					}
				}
			}
			i ++;
			l = 0;
		}

		tagMap_deselect(tag_map, selection_value);

		score += arrayMinCoverage_next(array, i - m, copy_desc_buffer + m, selection_value, min_score - score, tag_map, NULL);
	}

	if (score < min_score){
		memcpy(desc_buffer, copy_desc_buffer, sizeof(struct categoryDesc) * nb_category);
		min_score = score;
	}

	free(copy_desc_buffer);

	return min_score;
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
		tagMap_delete(tag_map);
	}
	categoryDesc_clean(nb_category, desc_buffer);

	return result;
}

int32_t arrayMinCoverage_greedy_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 			tag_map 		= NULL;
	uint32_t* 				order 			= NULL;
	int32_t 				result 			= -1;
	uint32_t 				local_score 	= 0;
	arrayMinCoverage_search arg[] 			= {(arrayMinCoverage_search)arrayMinCoverage_greedy, NULL};

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

		local_score = arrayMinCoverage_start(array, nb_category, desc_buffer, 1, tag_map->nb_element + 1, tag_map, arg);
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
		tagMap_delete(tag_map);
	}
	categoryDesc_clean(nb_category, desc_buffer);

	return result;
}

int32_t arrayMinCoverage_exact_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 			tag_map 		= NULL;
	uint32_t* 				order 			= NULL;
	int32_t 				result 			= -1;
	uint32_t 				local_score 	= 0xffffffff;
	arrayMinCoverage_search arg[] 			= {(arrayMinCoverage_search)arrayMinCoverage_exact, NULL};


	if (!nb_category){
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

		local_score = arrayMinCoverage_start(array, nb_category, desc_buffer, 1, tag_map->nb_element + 1, tag_map, arg);
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
		tagMap_delete(tag_map);
	}
	categoryDesc_clean(nb_category, desc_buffer);

	return result;
}

int32_t arrayMinCoverage_reshape_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 			tag_map 		= NULL;
	uint32_t* 				order 			= NULL;
	int32_t 				result 			= -1;
	uint32_t 				local_score 	= 0;
	arrayMinCoverage_search arg[] 			= {(arrayMinCoverage_search)arrayMinCoverage_reshape, NULL};

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

		local_score = arrayMinCoverage_start(array, nb_category, desc_buffer, 1, tag_map->nb_element + 1, tag_map, arg);
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
		tagMap_delete(tag_map);
	}
	categoryDesc_clean(nb_category, desc_buffer);

	return result;
}

int32_t arrayMinCoverage_split_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 			tag_map 		= NULL;
	uint32_t* 				order 			= NULL;
	int32_t 				result 			= -1;
	uint32_t 				local_score 	= 0;
	arrayMinCoverage_search arg[] 			= {(arrayMinCoverage_search)arrayMinCoverage_split, (arrayMinCoverage_search)arrayMinCoverage_auto};

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

		local_score = arrayMinCoverage_start(array, nb_category, desc_buffer, 1, tag_map->nb_element + 1, tag_map, arg);
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
		tagMap_delete(tag_map);
	}
	categoryDesc_clean(nb_category, desc_buffer);

	return result;
}

int32_t arrayMinCoverage_super_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score){
	struct tagMap* 			tag_map 		= NULL;
	uint32_t* 				order 			= NULL;
	int32_t 				result 			= -1;
	uint32_t 				local_score 	= 0;
	arrayMinCoverage_search arg[] 			= {(arrayMinCoverage_search)arrayMinCoverage_split, (arrayMinCoverage_search)arrayMinCoverage_super};


	if (!nb_category){
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

		local_score = arrayMinCoverage_start(array, nb_category, desc_buffer, 1, tag_map->nb_element + 1, tag_map, arg);
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
		tagMap_delete(tag_map);
	}
	categoryDesc_clean(nb_category, desc_buffer);

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
