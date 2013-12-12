#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "loop.h"
#include "workPercent.h"

int32_t loopEngine_search_previous_occurrence(struct instruction* ins1, struct instruction* ins2);
int32_t loopEngine_search_matching_loop(struct loopToken* loop, struct loopIteration* iteration);

static int32_t loopEngine_compare_instruction_sequence(struct loopEngine* engine, uint32_t offset1, uint32_t offset2, uint32_t length);
/*static int32_t loopEngine_remove_redundant_loop(struct loopEngine* engine);*/

struct loopEngine* loopEngine_create(){
	struct loopEngine* engine;

	engine = (struct loopEngine*)malloc(sizeof(struct loopEngine));
	if (engine == NULL){
		printf("ERROR: in %sunable to allocate memory\n", __func__);
	}
	else{
		if (loopEngine_init(engine)){
			printf("ERROR: in %s, unable to init loopEngine\n", __func__);
			free(engine);
			engine = NULL;
		}
	}

	return engine;
}

int32_t loopEngine_init(struct loopEngine* engine){
	int32_t result = -1;

	if (engine != NULL){
		result = array_init(&(engine->element_array), sizeof(struct instruction));
		if (result){
			printf("ERROR: in %s, unable to init element array\n", __func__);
		}
		else{
			engine->nb_loop = 0;
			engine->index = NULL;
			engine->iteration_pool = NULL;
		}
	}

	return result;
}

int32_t loopEngine_add(struct loopEngine* engine, struct instruction* instruction){
	int32_t result;

	result = array_add(&(engine->element_array), instruction);
	if (result < 0){
		printf("ERROR: in %s unable to add element to history array\n", __func__);
	}
	else{
		result = 0;
	}

	return result;
}

/* This routine is slow - to make it faster we can do some precomputation on the element array (sorting for example) */
int32_t loopEngine_process(struct loopEngine* engine){
	int32_t 				result 				= -1;
	uint32_t 				i;
	int32_t 				index_prev;
	void* 					element;
	uint32_t 				max_index;
	uint32_t 				min_index;
	struct loopToken 		new_iteration;
	struct loopToken 		new_instance;
	int32_t 				instance_index;
	int32_t 				loop_index;
	struct loopIteration 	iteration;
	struct loopToken*		matching_loop;
	uint32_t 				counter_instance 	= 0;
	uint32_t				counter_iteration 	= 0;
	struct array 			token_array;
	uint32_t* 				token_accelerator	= NULL;
	uint32_t				last_token;
	struct loopToken* 		current_token;
	uint32_t				pool_offset 		= 0;
	#ifdef VERBOSE
	struct workPercent 		work;
	#endif

	#ifdef VERBOSE
	printf("LoopEngine: %u element(s), loop core min length: %u\n", array_get_length(&(engine->element_array)), LOOP_MINIMAL_CORE_LENGTH);
	workPercent_init(&work, "LoopEngine initial traversal: ", WORKPERCENT_ACCURACY_1, array_get_length(&(engine->element_array)) - LOOP_MINIMAL_CORE_LENGTH);
	#endif
	
	if (array_init(&token_array, sizeof(struct loopToken))){
		printf("ERROR: in %s, unable to init token array\n", __func__);
		return result;
	}

	token_accelerator = (uint32_t*)calloc(array_get_length(&(engine->element_array)), sizeof(uint32_t));
	if (token_accelerator == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	for (i = LOOP_MINIMAL_CORE_LENGTH; i < array_get_length(&(engine->element_array)); i++){
		element = array_get(&(engine->element_array), i);
		index_prev = i + 1 - LOOP_MINIMAL_CORE_LENGTH;
		last_token = array_get_length(&token_array);

		do {
			max_index = index_prev - 1;
			min_index = (2*i > array_get_length(&(engine->element_array))) ? 2*i - array_get_length(&(engine->element_array)) : 0;

			index_prev = array_search_seq_down(&(engine->element_array), min_index, max_index, element, (int32_t(*)(void*,void*))loopEngine_search_previous_occurrence);

			if (index_prev >= 0){
				iteration.offset = index_prev;
				iteration.length = i - index_prev;

				if (loopEngine_compare_instruction_sequence(engine, iteration.offset, i, iteration.length) == 0){

					loop_index = array_search_seq_down(&token_array, token_accelerator[iteration.offset], array_get_length(&token_array) - 1, &iteration, (int32_t(*)(void*,void*))loopEngine_search_matching_loop);
					if (loop_index >= 0){
						matching_loop = (struct loopToken*)array_get(&token_array, loop_index);
					}
					else{
						new_instance.offset 		= iteration.offset;
						new_instance.length 		= iteration.length;
						new_instance.id 			= counter_instance;
						new_instance.iteration 		= 0;

						instance_index = array_add(&token_array, &new_instance);
						if (instance_index < 0){
							printf("ERROR: in %s, unable to add loop instance to loop array\n", __func__);
							continue;
						}

						counter_instance ++;
						counter_iteration ++;

						matching_loop = (struct loopToken*)array_get(&token_array, instance_index);
					}

					new_iteration.offset 		= i;
					new_iteration.length 		= iteration.length;
					new_iteration.id 			= matching_loop->id;					
					new_iteration.iteration 	= matching_loop->iteration + 1;

					if (array_add(&token_array, &new_iteration) < 0){
						printf("ERROR: in %s, unable to add loop iteration to loop array\n", __func__);
					}
					else{
						counter_iteration ++;
					}
				}
			}
		} while(index_prev > (int32_t)min_index);

		token_accelerator[i] = last_token;

		#ifdef VERBOSE
		workPercent_notify(&work, 1);
		#endif
	}

	#ifdef VERBOSE
	workPercent_conclude(&work);
	printf("LoopEngine: found %u iteration(s) distributed over %u raw loop(s) - formatting ...\n", counter_iteration, counter_instance);
	#endif

	if (engine->index != NULL){
		free(engine->index);
		engine->index = NULL;
	}
	if (engine->iteration_pool != NULL){
		free(engine->iteration_pool);
		engine->iteration_pool = NULL;
	}

	engine->index = (struct loopIndex*)malloc(counter_instance * sizeof(struct loopIndex));
	if (engine->index == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	engine->iteration_pool = (struct loopIteration*)malloc(counter_iteration * sizeof(struct loopIteration));
	if (engine->iteration_pool == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	memset(engine->index, 0, sizeof(struct loopIndex) * counter_instance);

	for (i = array_get_length(&token_array); i > 0; i--){
		current_token = (struct loopToken*)array_get(&token_array, i - 1);

		if (engine->index[current_token->id].nb_iteration == 0){
			engine->index[current_token->id].first_iteration = engine->iteration_pool + pool_offset;
			engine->index[current_token->id].nb_iteration = current_token->iteration + 1;

			pool_offset += current_token->iteration + 1;
		}
		
		engine->index[current_token->id].first_iteration[current_token->iteration].offset = current_token->offset;
		engine->index[current_token->id].first_iteration[current_token->iteration].length = current_token->length;
	}

	engine->nb_loop = counter_instance;


	result = 0;

	exit:

	array_clean(&token_array);
	if (token_accelerator != NULL){
		free(token_accelerator);
	}

	return result;
}

void loopEngine_print_loop(struct loopEngine* engine){
	uint32_t i;
	uint32_t j;

	if (engine->index != NULL && engine->iteration_pool != NULL){
		for (i = 0; i < engine->nb_loop; i++){
			printf("%u - it = %u {", i, engine->index[i].nb_iteration);
			for (j = 0; j < engine->index[i].nb_iteration; j++){
				if (j != engine->index[i].nb_iteration - 1){
					printf("(off: %u, lgth: %u), ", engine->index[i].first_iteration[j].offset, engine->index[i].first_iteration[j].length);
				}
				else{
					printf("(off: %u, lgth: %u)}\n", engine->index[i].first_iteration[j].offset, engine->index[i].first_iteration[j].length);
				}
			}
		}
	}
}

void loopEngine_clean(struct loopEngine* engine){
	if (engine != NULL){
		array_clean(&(engine->element_array));
		
		if (engine->index != NULL){
			free(engine->index);
			engine->index = NULL;
		}
		if (engine->iteration_pool != NULL){
			free(engine->iteration_pool);
			engine->iteration_pool = NULL;
		}
	}
}

void loopEngine_delete(struct loopEngine* engine){
	if (engine != NULL){
		loopEngine_clean(engine);
		free(engine);
	}
}

static int32_t loopEngine_compare_instruction_sequence(struct loopEngine* engine, uint32_t offset1, uint32_t offset2, uint32_t length){
	struct instruction* ins1;
	struct instruction* ins2;
	uint32_t 			i;
	int32_t	 			result = 0;

	for (i = 0; i < length && result == 0; i++){
		ins1 = (struct instruction*)array_get(&(engine->element_array), offset1 + i);
		ins2 = (struct instruction*)array_get(&(engine->element_array), offset2 + i);

		result = (ins1->opcode != ins2->opcode);
	}

	return result;
}

#if 0
static int32_t loopEngine_remove_redundant_loop(struct loopEngine* engine){
	/*we can sort loop byt start address and */
}
#endif

/* ===================================================================== */
/* Sorting routine(s)	    				                             */
/* ===================================================================== */

int32_t loopEngine_search_previous_occurrence(struct instruction* ins1, struct instruction* ins2){
	return (int32_t)(ins1->opcode - ins2->opcode);
}

int32_t loopEngine_search_matching_loop(struct loopToken* loop, struct loopIteration* iteration){
	return !(loop->offset == iteration->offset && loop->length == iteration->length);
}