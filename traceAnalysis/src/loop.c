#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "loop.h"
#include "workPercent.h"
#include "multiColumn.h"
#include "traceFragment.h"


int32_t loopEngine_sort_redundant_loop(const void* arg1, const void* arg2);

static int32_t loopEngine_is_loop_rolled(struct loopEngine* engine, uint32_t index);


struct loopEngine* loopEngine_create(struct trace* trace){
	struct loopEngine* engine;

	engine = (struct loopEngine*)malloc(sizeof(struct loopEngine));
	if (engine == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		if (loopEngine_init(engine, trace)){
			printf("ERROR: in %s, unable to init loopEngine\n", __func__);
			free(engine);
			engine = NULL;
		}
	}

	return engine;
}

int32_t loopEngine_init(struct loopEngine* engine, struct trace* trace){
	trace_get_reference(trace);
	engine->trace = trace;
	engine->loops = NULL;
	engine->nb_loop = 0;	

	return 0;
}

#define PREDICATE_NOT_EQUAL(a, b)		((((a) - (b)) | ((b) - (a))) >> 31)
#define PREDICATE_EQUAL_ZERO(a) 		((uint32_t)(~((a) | (-(a)))) >> 31)

int32_t loopEngine_process_strict(struct loopEngine* engine){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	struct array 			token_array;
	struct loopToken 		token;
	struct loopToken* 		token_pointer;
	uint32_t 				iteration_counter = 0;
	uint32_t 				pop_count;
	uint32_t* 				iteration_count;
	uint32_t 				loop_max_length = (engine->trace->nb_instruction > 2*LOOP_MAXIMAL_CORE_LENGTH) ? LOOP_MAXIMAL_CORE_LENGTH : (engine->trace->nb_instruction / 2);
	#ifdef VERBOSE
	struct workPercent 		work;
	#endif

	iteration_count = (uint32_t*)malloc(sizeof(uint32_t) * (engine->trace->nb_instruction - LOOP_MINIMAL_CORE_LENGTH + 1));
	if (iteration_count == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	if (array_init(&token_array, sizeof(struct loopToken))){
		printf("ERROR: in %s, unable to init token array\n", __func__);
		free(iteration_count);
		return -1;
	}

	#ifdef VERBOSE
	printf("LoopEngine: %u element(s), loop core min length: %u, max length: %u, min # iteration(s): %u\n", engine->trace->nb_instruction, LOOP_MINIMAL_CORE_LENGTH, LOOP_MAXIMAL_CORE_LENGTH, LOOP_MINIMAL_NB_ITERATION);
	workPercent_init(&work, "LoopEngine initial traversal: ", WORKPERCENT_ACCURACY_1, loop_max_length - LOOP_MINIMAL_CORE_LENGTH + 1);
	#endif

	for (i = LOOP_MINIMAL_CORE_LENGTH; i <= loop_max_length; i++){
		for (k = 0, pop_count = 0; k < i; k++){
			pop_count += PREDICATE_NOT_EQUAL(engine->trace->instructions[k].opcode, engine->trace->instructions[i + k].opcode);
		}

		iteration_count[i] = PREDICATE_EQUAL_ZERO(pop_count);

		for (j = 1; j < i; j++){
			pop_count -= PREDICATE_NOT_EQUAL(engine->trace->instructions[j - 1].opcode, engine->trace->instructions[j + i -1].opcode);
			pop_count += PREDICATE_NOT_EQUAL(engine->trace->instructions[j + i - 1].opcode, engine->trace->instructions[j + (2*i) -1].opcode);

			iteration_count[j + i] = PREDICATE_EQUAL_ZERO(pop_count);
		}

		for (; j <= engine->trace->nb_instruction - (2*i); j++){
			pop_count -= PREDICATE_NOT_EQUAL(engine->trace->instructions[j - 1].opcode, engine->trace->instructions[j + i -1].opcode);
			pop_count += PREDICATE_NOT_EQUAL(engine->trace->instructions[j + i - 1].opcode, engine->trace->instructions[j + (2*i) -1].opcode);

			if (pop_count == 0){
				iteration_count[j + i] = iteration_count[j] + 1;
			}
			else{
				iteration_count[j + i] = 0;
				if (iteration_count[j] && (iteration_count[j] + 1) >= LOOP_MINIMAL_NB_ITERATION){
					iteration_counter += iteration_count[j] + 1;

					token.offset = j - (iteration_count[j] * i);
					token.length = i;
					token.nb_iteration = iteration_count[j] + 1;

					if (array_add(&token_array, &token) < 0){
						printf("ERROR: in %s, unable to add token to array\n", __func__);
					}
				}
			}
		}

		for (; j <= engine->trace->nb_instruction - i; j++){
			if (iteration_count[j] && (iteration_count[j] + 1) >= LOOP_MINIMAL_NB_ITERATION){
				iteration_counter += iteration_count[j] + 1;

				token.offset = j - (iteration_count[j] * i);
				token.length = i;
				token.nb_iteration = iteration_count[j] + 1;

				if (array_add(&token_array, &token) < 0){
					printf("ERROR: in %s, unable to add token to array\n", __func__);
				}
			}
		}

		#ifdef VERBOSE
		workPercent_notify(&work, 1);
		#endif
	}

	#ifdef VERBOSE
	workPercent_conclude(&work);
	printf("LoopEngine: found %u iteration(s) distributed over %u raw loop(s) - formatting\n", iteration_counter, array_get_length(&token_array));
	#endif

	free(iteration_count);

	if (engine->loops != NULL){
		free(engine->loops);
		engine->loops = NULL;
		engine->nb_loop = 0;
	}

	engine->loops = (struct loop*)malloc(array_get_length(&token_array) * sizeof(struct loop));
	if (engine->loops == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for (i = 0; i < array_get_length(&token_array); i++){
			token_pointer = (struct loopToken*)array_get(&token_array, i);
			
			engine->loops[i].offset 		= token_pointer->offset;
			engine->loops[i].length 		= token_pointer->length;
			engine->loops[i].nb_iteration 	= token_pointer->nb_iteration;
			engine->loops[i].epilogue 		= 0;
		}
		engine->nb_loop = array_get_length(&token_array);
	}

	array_clean(&token_array);
	
	return 0;
}

int32_t loopEngine_process_norder(struct loopEngine* engine){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	struct array 			token_array;
	struct loopToken 		token;
	struct loopToken* 		token_pointer;
	uint32_t 				iteration_counter = 0;
	uint32_t 				pop_count;
	uint32_t* 				iteration_count;
	int32_t* 				occurence_counter;
	uint32_t 				loop_max_length = (engine->trace->nb_instruction > 2*LOOP_MAXIMAL_CORE_LENGTH) ? LOOP_MAXIMAL_CORE_LENGTH : (engine->trace->nb_instruction / 2);
	#ifdef VERBOSE
	struct workPercent 		work;
	#endif

	iteration_count = (uint32_t*)malloc(sizeof(uint32_t) * (engine->trace->nb_instruction - LOOP_MINIMAL_CORE_LENGTH + 1));
	occurence_counter = (int32_t*)malloc(sizeof(int32_t) * 2048); /* upper bound for the xed instruction enumeration */
	
	if (iteration_count == NULL || occurence_counter == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		if (iteration_count != NULL){
			free(iteration_count);
		}
		if (occurence_counter != NULL){
			free(occurence_counter);
		}
		return -1;
	}

	if (array_init(&token_array, sizeof(struct loopToken))){
		printf("ERROR: in %s, unable to init token array\n", __func__);
		free(iteration_count);
		free(occurence_counter);
		return -1;
	}

	#ifdef VERBOSE
	printf("LoopEngine: %u element(s), loop core min length: %u, max length: %u, min # iteration(s): %u\n", engine->trace->nb_instruction, LOOP_MINIMAL_CORE_LENGTH, LOOP_MAXIMAL_CORE_LENGTH, LOOP_MINIMAL_NB_ITERATION);
	workPercent_init(&work, "LoopEngine initial traversal: ", WORKPERCENT_ACCURACY_1, loop_max_length - LOOP_MINIMAL_CORE_LENGTH + 1);
	#endif

	for (i = LOOP_MINIMAL_CORE_LENGTH; i <= loop_max_length; i++){
		memset(occurence_counter, 0, sizeof(int32_t) * 2048);
		for (k = 0, pop_count = 0; k < i; k++){
			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[k].opcode]);
			occurence_counter[engine->trace->instructions[k].opcode] ++;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[k].opcode]);

			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[i + k].opcode]);
			occurence_counter[engine->trace->instructions[i + k].opcode] --;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[i + k].opcode]);
		}

		iteration_count[i] = PREDICATE_EQUAL_ZERO(pop_count);

		for (j = 1; j < i; j++){
			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j - 1].opcode]);
			occurence_counter[engine->trace->instructions[j - 1].opcode] --;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j - 1].opcode]);

			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + i - 1].opcode]);
			occurence_counter[engine->trace->instructions[j + i - 1].opcode] += 2;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + i - 1].opcode]);

			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + (2*i) - 1].opcode]);
			occurence_counter[engine->trace->instructions[j + (2*i) - 1].opcode] --;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + (2*i) - 1].opcode]);

			iteration_count[j + i] = PREDICATE_EQUAL_ZERO(pop_count);
		}

		for (; j <= engine->trace->nb_instruction - (2*i); j++){
			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j - 1].opcode]);
			occurence_counter[engine->trace->instructions[j - 1].opcode] --;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j - 1].opcode]);

			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + i - 1].opcode]);
			occurence_counter[engine->trace->instructions[j + i - 1].opcode] += 2;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + i - 1].opcode]);
			
			pop_count += PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + (2*i) - 1].opcode]);
			occurence_counter[engine->trace->instructions[j + (2*i) - 1].opcode] --;
			pop_count -= PREDICATE_EQUAL_ZERO(occurence_counter[engine->trace->instructions[j + (2*i) - 1].opcode]);

			if (pop_count == 0){
				iteration_count[j + i] = iteration_count[j] + 1;
			}
			else{
				iteration_count[j + i] = 0;
				if (iteration_count[j] && (iteration_count[j] + 1) >= LOOP_MINIMAL_NB_ITERATION){
					iteration_counter += iteration_count[j] + 1;

					token.offset = j - (iteration_count[j] * i);
					token.length = i;
					token.nb_iteration = iteration_count[j] + 1;

					if (array_add(&token_array, &token) < 0){
						printf("ERROR: in %s, unable to add token to array\n", __func__);
					}
				}
			}
		}

		for (; j <= engine->trace->nb_instruction - i; j++){
			if (iteration_count[j] && (iteration_count[j] + 1) >= LOOP_MINIMAL_NB_ITERATION){
				iteration_counter += iteration_count[j] + 1;

				token.offset = j - (iteration_count[j] * i);
				token.length = i;
				token.nb_iteration = iteration_count[j] + 1;

				if (array_add(&token_array, &token) < 0){
					printf("ERROR: in %s, unable to add token to array\n", __func__);
				}
			}
		}

		#ifdef VERBOSE
		workPercent_notify(&work, 1);
		#endif
	}

	#ifdef VERBOSE
	workPercent_conclude(&work);
	printf("LoopEngine: found %u iteration(s) distributed over %u raw loop(s) - formatting\n", iteration_counter, array_get_length(&token_array));
	#endif

	free(iteration_count);
	free(occurence_counter);

	if (engine->loops != NULL){
		free(engine->loops);
		engine->loops = NULL;
		engine->nb_loop = 0;
	}

	engine->loops = (struct loop*)malloc(array_get_length(&token_array) * sizeof(struct loop));
	if (engine->loops == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for (i = 0; i < array_get_length(&token_array); i++){
			token_pointer = (struct loopToken*)array_get(&token_array, i);
			
			engine->loops[i].offset 		= token_pointer->offset;
			engine->loops[i].length 		= token_pointer->length;
			engine->loops[i].nb_iteration 	= token_pointer->nb_iteration;
			engine->loops[i].epilogue 		= 0;
		}
		engine->nb_loop = array_get_length(&token_array);
	}

	array_clean(&token_array);
	
	return 0;
}

#define loop_include_in(a, b) 		(((a).offset >= (b).offset) && ((a).offset + ((a).length * (a).nb_iteration) + (a).epilogue <= (b).offset + ((b).length * (b).nb_iteration) + (b).epilogue))
#define loop_not_include(a, b) 		(((a).offset < (b).offset) || ((a).offset + ((a).length * (a).nb_iteration) + (a).epilogue > (b).offset + ((b).length * (b).nb_iteration) + (b).epilogue))
#define loop_nested_in(a, b) 		((((a).offset - (b).offset) % (b).length) + ((a).length * (a).nb_iteration) + (a).epilogue <= (b).length)
#define loop_not_nested_in(a, b) 	((((a).offset - (b).offset) % (b).length) + ((a).length * (a).nb_iteration) + (a).epilogue > (b).length)
#define loop_epilogue_of(a, b)		(((a).offset == 1 + (b).offset + (b).epilogue) && ((a).length == (b).length) && ((a).nb_iteration == (b).nb_iteration))

int32_t loopEngine_remove_redundant_loop_strict(struct loopEngine* engine){
	uint32_t 		i;
	uint32_t 		counter;
	struct loop* 	realloc_loops;

	if (engine->loops != NULL){
		qsort(engine->loops, engine->nb_loop, sizeof(struct loop), loopEngine_sort_redundant_loop);

		for (i = 1, counter = 1; i < engine->nb_loop; i++){
			if (loop_not_include(engine->loops[i], engine->loops[counter - 1])){
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
		printf("LoopEngine: removed redundant loop(s) STRICT -> remaining %u loop(s)\n", engine->nb_loop);
		#endif
	}

	return 0;
}

int32_t loopEngine_remove_redundant_loop_packed(struct loopEngine* engine){
	uint32_t 		i;
	uint32_t 		counter;
	struct loop* 	realloc_loops;

	if (engine->loops != NULL){
		qsort(engine->loops, engine->nb_loop, sizeof(struct loop), loopEngine_sort_redundant_loop);

		for (i = 1, counter = 1; i < engine->nb_loop; i++){
			if (loop_not_include(engine->loops[i], engine->loops[counter - 1])){
				if (loop_epilogue_of(engine->loops[i], engine->loops[counter - 1])){
					engine->loops[counter - 1].epilogue ++;
				}
				else{
					if (i != counter){
						memcpy(engine->loops + counter, engine->loops + i, sizeof(struct loop));
					}
					counter ++;
				}
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
		printf("LoopEngine: removed redundant loop(s) PACKED -> remaining %u loop(s)\n", engine->nb_loop);
		#endif
	}

	return 0;
}

#define LOOP_MAX_NB_DOMINANT_LOOP 16

int32_t loopEngine_remove_redundant_loop_nested(struct loopEngine* engine){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		counter;
	uint32_t 		min_overlapping_index;
	uint8_t 		epilogue;
	struct loop* 	realloc_loops;
	uint32_t 		dominant_loop_index[LOOP_MAX_NB_DOMINANT_LOOP];
	uint8_t 		dominant_loop_state[LOOP_MAX_NB_DOMINANT_LOOP];
	uint32_t 		nb_dominant_loop;
	uint32_t 		nb_true_dominant_loop;

	if (engine->loops != NULL){
		qsort(engine->loops, engine->nb_loop, sizeof(struct loop), loopEngine_sort_redundant_loop);

		for (i = 1, counter = 1, min_overlapping_index = 0; i < engine->nb_loop; i++){
			for (j = counter, nb_dominant_loop = 0, nb_true_dominant_loop = 0; j > min_overlapping_index; j--){
				if (loop_include_in(engine->loops[i], engine->loops[j - 1])){
					if (loop_nested_in(engine->loops[i], engine->loops[j - 1])){
						if (nb_true_dominant_loop){
							for (k = 0; k < nb_dominant_loop; k++){
								if (dominant_loop_state[k] && loop_not_nested_in(engine->loops[dominant_loop_index[k]], engine->loops[j - 1])){
									dominant_loop_state[k] = 0;
									nb_true_dominant_loop --;
								}
							}
						}
						else{
							break;
						}
					}
					else{
						if (nb_dominant_loop < LOOP_MAX_NB_DOMINANT_LOOP){
							dominant_loop_index[nb_dominant_loop] = j - 1;
							dominant_loop_state[nb_dominant_loop] = 1;
							nb_dominant_loop ++;
							nb_true_dominant_loop ++;
						}
						else{
							printf("ERROR: in %s, constant LOOP_MAX_NB_DOMINANT_LOOP=%u is too small, increment\n", __func__, LOOP_MAX_NB_DOMINANT_LOOP);
						}
					}
				}
			}

			if (nb_true_dominant_loop == 0){
				for (j = counter, epilogue = 0; j > min_overlapping_index; j--){
					if (loop_epilogue_of(engine->loops[i], engine->loops[j - 1])){
						engine->loops[j - 1].epilogue ++;
						epilogue = 1;
						break;
					}
				}

				if (!epilogue){
					if (i != counter){
						memcpy(engine->loops + counter, engine->loops + i, sizeof(struct loop));
					}
					counter ++;
				}
			}

			while (engine->loops[min_overlapping_index].offset + (engine->loops[min_overlapping_index].length * engine->loops[min_overlapping_index].nb_iteration) + engine->loops[min_overlapping_index].epilogue < engine->loops[i].offset){
				min_overlapping_index ++;
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
		printf("LoopEngine: removed redundant loop(s) NESTED -> remaining %u loop(s)\n", engine->nb_loop);
		#endif
	}

	return 0;
}

void loopEngine_print_loop(struct loopEngine* engine){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	uint32_t 					coverage = 0;
	uint32_t 					offset = 0;

	printer = multiColumnPrinter_create(stdout, 6, NULL, NULL, NULL);
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
	multiColumnPrinter_set_title(printer, 5, (char*)"State");

	multiColumnPrinter_print_header(printer);

	if (engine->loops != NULL){
		for (i = 0; i < engine->nb_loop; i++){
			if (loopEngine_is_loop_rolled(engine, i)){
				multiColumnPrinter_print(printer, i, engine->loops[i].nb_iteration, engine->loops[i].offset, engine->loops[i].length, engine->loops[i].epilogue, "rolled", NULL);
			}
			else{
				multiColumnPrinter_print(printer, i, engine->loops[i].nb_iteration, engine->loops[i].offset, engine->loops[i].length, engine->loops[i].epilogue, "unrolled", NULL);
			}

			if (engine->loops[i].offset > offset){
				coverage += (engine->loops[i].nb_iteration * engine->loops[i].length) + engine->loops[i].epilogue;
				offset = engine->loops[i].offset + (engine->loops[i].nb_iteration * engine->loops[i].length) + engine->loops[i].epilogue;
			}
			else{
				coverage += (engine->loops[i].offset + (engine->loops[i].nb_iteration * engine->loops[i].length) + engine->loops[i].epilogue) - offset;
				offset = engine->loops[i].offset + (engine->loops[i].nb_iteration * engine->loops[i].length) + engine->loops[i].epilogue;
			}
		}

		printf("Loop coverage: %u/%u (%.3f%%) WARNING this value is wrong if no sorting has been performed\n", coverage, engine->trace->nb_instruction, 100*((double)coverage/(double)engine->trace->nb_instruction));
	}

	multiColumnPrinter_delete(printer);
}

int32_t loopEngine_export_it(struct loopEngine* engine, struct array* frag_array, uint32_t loop_index, uint32_t iteration_index){
	struct traceFragment fragment;

	if (engine->loops != NULL){
		if (loop_index < engine->nb_loop){
			if ((uint32_t)iteration_index < engine->loops[loop_index].nb_iteration){
				if (traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_NONE, NULL, NULL)){
					printf("ERROR: in %s, unable to init traceFragment\n", __func__);
					return -1;
				}

				if (trace_extract_segment(engine->trace, &(fragment.trace), engine->loops[loop_index].offset + engine->loops[loop_index].length * iteration_index, engine->loops[loop_index].length)){
					printf("ERROR: in %s, unable to extract sub trace\n", __func__);
					return -1;
				}

				snprintf(fragment.tag, TRACEFRAGMENT_TAG_LENGTH, "Loop %u - it %u", loop_index, iteration_index);

				if (array_add(frag_array, &fragment) < 0){
					printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
					traceFragment_clean(&fragment);
					return -1;
				}
			}
			else{
				printf("WARNING: in %s, iteration index is larger than the loop number of iteration (%u)\n", __func__, engine->loops[loop_index].nb_iteration);
			}
		}
		else{
			printf("WARNING: in %s, loopindex is larger than the number of loops (%u)\n", __func__, engine->nb_loop);
		}
	}
	else{
		printf("WARNING: in %s, loopEngine does not contain loops - cannot export\n", __func__);
	}

	return 0;
}

int32_t loopEngine_export_all(struct loopEngine* engine, struct array* frag_array, int32_t loop_index){
	uint32_t 				i;
	uint32_t 				start_index;
	uint32_t 				stop_index;
	struct traceFragment 	fragment;
	uint32_t 				total_length;

	if (loop_index > 0){
		start_index = loop_index;
		stop_index = ((uint32_t)loop_index + 1 < engine->nb_loop) ? ((uint32_t)loop_index + 1) : engine->nb_loop;
	}
	else{
		start_index = 0;
		stop_index = engine->nb_loop;
	}

	if (engine->loops != NULL){
		for (i = start_index; i < stop_index; i++){
			if (traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_LOOP, (void*)engine->loops[i].length, NULL)){
				printf("ERROR: in %s, unable to init traceFragment\n", __func__);
				return -1;
			}

			total_length = engine->loops[i].length * engine->loops[i].nb_iteration + engine->loops[i].epilogue;
			if (trace_extract_segment(engine->trace, &(fragment.trace), engine->loops[i].offset, total_length)){
				printf("ERROR: in %s, unable to extract sub trace\n", __func__);
				continue;
			}

			snprintf(fragment.tag, TRACEFRAGMENT_TAG_LENGTH, "Loop %u", i);

			if (array_add(frag_array, &fragment) < 0){
				printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
				traceFragment_clean(&fragment);
				return -1;
			}
		}
	}
	else{
		printf("WARNING: in %s, loopEngine does not contain loops - cannot export\n", __func__);
	}

	return 0;
}

int32_t loopEngine_export_noEp(struct loopEngine* engine, struct array* frag_array, int32_t loop_index){
	uint32_t 				i;
	uint32_t 				start_index;
	uint32_t 				stop_index;
	struct traceFragment 	fragment;
	uint32_t 				total_length;

	if (loop_index > 0){
		start_index = loop_index;
		stop_index = ((uint32_t)loop_index + 1 < engine->nb_loop) ? ((uint32_t)loop_index + 1) : engine->nb_loop;
	}
	else{
		start_index = 0;
		stop_index = engine->nb_loop;
	}

	if (engine->loops != NULL){
		for (i = start_index; i < stop_index; i++){
			if (traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_LOOP, (void*)engine->loops[i].length, NULL)){
				printf("ERROR: in %s, unable to init traceFragment\n", __func__);
				return -1;
			}

			total_length = engine->loops[i].length * engine->loops[i].nb_iteration;
			if (trace_extract_segment(engine->trace, &(fragment.trace), engine->loops[i].offset, total_length)){
				printf("ERROR: in %s, unable to extract sub trace\n", __func__);
				continue;
			}

			snprintf(fragment.tag, TRACEFRAGMENT_TAG_LENGTH, "Loop %u", i);

			if (array_add(frag_array, &fragment) < 0){
				printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
				traceFragment_clean(&fragment);
				return -1;
			}
		}
	}
	else{
		printf("WARNING: in %s, loopEngine does not contain loops - cannot export\n", __func__);
	}

	return 0;
}

void loopEngine_clean(struct loopEngine* engine){
	if (engine != NULL){
		trace_delete(engine->trace);
		
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

static int32_t loopEngine_is_loop_rolled(struct loopEngine* engine, uint32_t index){
	uint32_t i;
	uint32_t j;
	ADDRESS pc1;
	ADDRESS pc2;

	for (i = 1; i < engine->loops[index].nb_iteration; i++){
		for (j = 0; j < engine->loops[index].length; j++){
			pc1 = engine->trace->instructions[engine->loops[index].offset + j].pc;
			pc2 = engine->trace->instructions[engine->loops[index].offset + j + (engine->loops[index].length * i)].pc;

			if (pc1 != pc2){
				return 0;
			}
		}
	}

	return 1;
}

/* ===================================================================== */
/* Sorting routine(s)	    				                             */
/* ===================================================================== */

int32_t loopEngine_sort_redundant_loop(const void* arg1, const void* arg2){
	struct loop* loop1 = (struct loop*)arg1;
	struct loop* loop2 = (struct loop*)arg2;

	if (loop1->offset != loop2->offset){
		return (int32_t)(loop1->offset - loop2->offset);
	}
	else{
		if (loop2->epilogue + loop2->length * loop2->nb_iteration != loop1->epilogue + loop1->length * loop1->nb_iteration){
			return (int32_t)(loop2->epilogue + loop2->length * loop2->nb_iteration) - (int32_t)(loop1->epilogue + loop1->length * loop1->nb_iteration);
		}
		else{
			/* We promote loops with a high number of iteration */
			return (int32_t)loop2->nb_iteration - loop1->nb_iteration;
		}
	}
}