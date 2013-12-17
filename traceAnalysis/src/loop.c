#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "loop.h"
#include "workPercent.h"
#include "multiColumn.h"
#include "traceFragment.h"

struct loopIteration{
	uint32_t offset;
	uint32_t length;
};

int32_t loopEngine_search_previous_occurrence(struct instruction* ins1, struct instruction* ins2);
int32_t loopEngine_search_matching_loop(struct loopToken* loop, struct loopIteration* iteration);
int32_t loopEngine_sort_redundant_loop(const void* arg1, const void* arg2);

static int32_t loopEngine_compare_instruction_sequence(struct loopEngine* engine, uint32_t offset1, uint32_t offset2, uint32_t length);

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
			engine->loops = NULL;
			engine->nb_loop = 0;	
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
	uint32_t* 				token_search_bound = NULL;
	struct loopToken* 		current_token;
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

	token_search_bound = (uint32_t*)calloc(2 * array_get_length(&(engine->element_array)), sizeof(uint32_t));
	if (token_search_bound == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	for (i = LOOP_MINIMAL_CORE_LENGTH; i < array_get_length(&(engine->element_array)); i++){
		element = array_get(&(engine->element_array), i);
		index_prev = i + 1 - LOOP_MINIMAL_CORE_LENGTH;
		token_search_bound[2*i] = array_get_length(&token_array);

		do {
			max_index = index_prev - 1;
			min_index = (2*i > array_get_length(&(engine->element_array))) ? 2*i - array_get_length(&(engine->element_array)) : 0;

			index_prev = array_search_seq_down(&(engine->element_array), min_index, max_index, element, (int32_t(*)(void*,void*))loopEngine_search_previous_occurrence);

			if (index_prev >= 0){
				iteration.offset = index_prev;
				iteration.length = i - index_prev;

				if (loopEngine_compare_instruction_sequence(engine, iteration.offset, i, iteration.length) == 0){

					loop_index = array_search_seq_down(&token_array, token_search_bound[2*iteration.offset], token_search_bound[2*iteration.offset + 1], &iteration, (int32_t(*)(void*,void*))loopEngine_search_matching_loop);
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

		token_search_bound[2*i + 1] = array_get_length(&token_array);

		#ifdef VERBOSE
		workPercent_notify(&work, 1);
		#endif
	}

	#ifdef VERBOSE
	workPercent_conclude(&work);
	printf("LoopEngine: found %u iteration(s) distributed over %u raw loop(s) - formatting\n", counter_iteration, counter_instance);
	#endif

	free(token_search_bound);
	token_search_bound = NULL;

	if (engine->loops != NULL){
		free(engine->loops);
		engine->loops = NULL;
	}

	engine->loops = (struct loop*)calloc(counter_instance, sizeof(struct loop));
	if (engine->loops == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	for (i = array_get_length(&token_array); i > 0; i--){
		current_token = (struct loopToken*)array_get(&token_array, i - 1);

		if (engine->loops[current_token->id].nb_iteration == 0){
			engine->loops[current_token->id].nb_iteration = current_token->iteration + 1;
		}
		if (current_token->iteration == 0){
			engine->loops[current_token->id].offset = current_token->offset;
			engine->loops[current_token->id].length = current_token->length;
			engine->loops[current_token->id].epilogue = 0;
		}
	}

	engine->nb_loop = counter_instance;


	result = 0;

	exit:

	array_clean(&token_array);
	if (token_search_bound != NULL){
		free(token_search_bound);
	}

	return result;
}


int32_t loopEngine_remove_redundant_loop(struct loopEngine* engine){
	uint32_t 		i;
	uint32_t 		counter;
	struct loop* 	realloc_loops;

	if (engine->loops != NULL){
		qsort(engine->loops, engine->nb_loop, sizeof(struct loop), loopEngine_sort_redundant_loop);

		for (i = 1, counter = 1; i < engine->nb_loop; i++){
			if (engine->loops[i].offset + (engine->loops[i].length * engine->loops[i].nb_iteration) + engine->loops[i].epilogue > engine->loops[counter - 1].offset + (engine->loops[counter - 1].length * engine->loops[counter - 1].nb_iteration) + engine->loops[counter - 1].epilogue){
				if (i != counter){
					memcpy(engine->loops + counter, engine->loops + i, sizeof(struct loop));
				}
				counter ++;
			}
		}

		engine->nb_loop = counter;
		realloc_loops = (struct loop*)realloc(engine->loops, engine->nb_loop * sizeof(struct loop));
		if (realloc_loops == NULL){
			printf("ERROR: in %s, unable to realloc loops\n", __func__);
		}
		else{
			engine->loops = realloc_loops;
		}

		#ifdef VERBOSE
		printf("LoopEngine: removed redundant loop(s) -> remaining %u loop(s)\n", engine->nb_loop);
		#endif
	}

	return 0;
}

/* We expect that loopEngine_remove_redundant_loop has just been previously run */
int32_t loopEngine_pack_epilogue(struct loopEngine* engine){
	uint32_t 		i;
	uint32_t 		counter;
	struct loop* 	realloc_loops;

	if (engine->loops != NULL){
		for (i = 1, counter = 1; i < engine->nb_loop; i++){
			if (engine->loops[i].offset == 1 + engine->loops[counter - 1].offset + engine->loops[counter -1].epilogue && engine->loops[i].length == engine->loops[counter - 1].length && engine->loops[i].nb_iteration == engine->loops[counter - 1].nb_iteration){
				engine->loops[counter - 1].epilogue ++;
			}
			else{
				if (i != counter){
					memcpy(engine->loops + counter, engine->loops + i, sizeof(struct loop));
				}
				counter ++;
			}
		}

		engine->nb_loop = counter;
		realloc_loops = (struct loop*)realloc(engine->loops, engine->nb_loop * sizeof(struct loop));
		if (realloc_loops == NULL){
			printf("ERROR: in %s, unable to realloc loops\n", __func__);
		}
		else{
			engine->loops = realloc_loops;
		}

		#ifdef VERBOSE
		printf("LoopEngine: packed epilogue-> remaining %u loop(s)\n", engine->nb_loop);
		#endif
	}

	return 0;
}

void loopEngine_print_loop(struct loopEngine* engine){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;

	printer = multiColumnPrinter_create(stdout, 5, NULL, NULL, NULL);
	if (printer == NULL){
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}

	multiColumnPrinter_set_column_size(printer, 0, 8);
	multiColumnPrinter_set_column_size(printer, 1, 6);

	multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
	multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_UINT32);
	multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
	multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);
	multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_UINT32);

	multiColumnPrinter_set_title(printer, 0, (char*)"");
	multiColumnPrinter_set_title(printer, 1, (char*)"Nb it");
	multiColumnPrinter_set_title(printer, 2, (char*)"Offset");
	multiColumnPrinter_set_title(printer, 3, (char*)"Length");
	multiColumnPrinter_set_title(printer, 4, (char*)"Epilogue");

	multiColumnPrinter_print_header(printer);

	if (engine->loops != NULL){
		for (i = 0; i < engine->nb_loop; i++){
			multiColumnPrinter_print(printer, i, engine->loops[i].nb_iteration, engine->loops[i].offset, engine->loops[i].length, engine->loops[i].epilogue, NULL);
		}
	}

	multiColumnPrinter_delete(printer);
}

int32_t loopEngine_export_traceFragment(struct loopEngine* engine, struct array* array){
	uint32_t 				i;
	uint32_t 				j;
	int32_t 				result = -1;
	struct traceFragment 	fragment;

	if (engine->loops != NULL){
		for (i = 0; i < engine->nb_loop; i++){
			if (traceFragment_init(&fragment)){
				printf("ERROR: in %s, unable to init traceFragment\n", __func__);
				return result;
			}

			for (j = 0; j < engine->loops[i].length * engine->loops[i].nb_iteration + engine->loops[i].epilogue; j++){
				/* We can do something way faster here - read multiple - write multiple - copy multiple */
				if (traceFragment_add_instruction(&fragment, (struct instruction*)array_get(&(engine->element_array), j + engine->loops[i].offset)) < 0){
					printf("ERROR: in %s, unable to add instruction to traceFragment\n", __func__);
				}
			}

			if (array_add(array, &fragment) < 0){
				printf("ERROR: in %s, unable to add traceFragment %u to array\n", __func__, i);
				traceFragment_clean(&fragment);
				return result;
			}
		}

		result = 0;
	}
	else{
		printf("ERROR: in %s, loopEngine does not contain loops- cannot export\n", __func__);
	}

	return result;
}

void loopEngine_clean(struct loopEngine* engine){
	if (engine != NULL){
		array_clean(&(engine->element_array));
		
		if (engine->loops != NULL){
			free(engine->loops);
			engine->loops = NULL;
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

/* ===================================================================== */
/* Sorting routine(s)	    				                             */
/* ===================================================================== */

int32_t loopEngine_search_previous_occurrence(struct instruction* ins1, struct instruction* ins2){
	return (int32_t)(ins1->opcode - ins2->opcode);
}

int32_t loopEngine_search_matching_loop(struct loopToken* loop, struct loopIteration* iteration){
	return !(loop->offset == iteration->offset && loop->length == iteration->length);
}

int32_t loopEngine_sort_redundant_loop(const void* arg1, const void* arg2){
	struct loop* loop1 = (struct loop*)arg1;
	struct loop* loop2 = (struct loop*)arg2;

	if (loop1->offset != loop2->offset){
		return (int32_t)(loop1->offset - loop2->offset);
	}
	else{
		return (int32_t)(loop2->epilogue + loop2->length * loop2->nb_iteration - loop1->epilogue + loop1->length * loop1->nb_iteration);
	}
}