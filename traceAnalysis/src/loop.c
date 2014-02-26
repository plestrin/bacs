#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "loop.h"
#include "workPercent.h"
#include "multiColumn.h"
#include "traceFragment.h" /*tmp - a voir */


int32_t loopEngine_sort_redundant_loop(const void* arg1, const void* arg2);

static int32_t loopEngine_is_loop_unrolled(struct loopEngine* engine, uint32_t index);


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

int32_t loopEngine_process(struct loopEngine* engine){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	uint32_t 				stack;
	uint32_t 				length;
	uint8_t 				loop;
	uint32_t 				nb_iteration;
	struct array 			token_array;
	struct loopToken 		token;
	struct loopToken* 		token_pointer;
	uint32_t 				iteration_counter = 0;
	#ifdef VERBOSE
	struct workPercent 		work;
	#endif


	if (array_init(&token_array, sizeof(struct loopToken))){
		printf("ERROR: in %s, unable to init token array\n", __func__);
		return -1;
	}

	#ifdef VERBOSE
	printf("LoopEngine: %u element(s), loop core min length: %u\n", engine->trace->nb_instruction, LOOP_MINIMAL_CORE_LENGTH);
	workPercent_init(&work, "LoopEngine initial traversal: ", WORKPERCENT_ACCURACY_1, engine->trace->nb_instruction - 2*LOOP_MINIMAL_CORE_LENGTH + 1);
	#endif

	for (i = 0; i < engine->trace->nb_instruction - 2*LOOP_MINIMAL_CORE_LENGTH + 1; i++){
		for (j = i, stack = 0; j < i + 2*(LOOP_MINIMAL_CORE_LENGTH - 1); j += 2){
			stack = stack ^ engine->trace->instructions[j].opcode;
			stack = stack ^ engine->trace->instructions[j + 1].opcode;
		}

		for (; j < engine->trace->nb_instruction - 1; j += 2){
			stack = stack ^ engine->trace->instructions[j].opcode;
			stack = stack ^ engine->trace->instructions[j + 1].opcode;
			if (stack == 0){
				length 			= (j + 2- i) / 2;
				loop 			= 1;
				nb_iteration 	= 1;
				do{
					for (k = 0; k < length; k++){
						if (engine->trace->instructions[i + k].opcode != engine->trace->instructions[i + k + length * nb_iteration].opcode){
							loop = 0;
							break;
						}
					}
					nb_iteration += loop;
				} while(loop && i + (nb_iteration + 1) * length < engine->trace->nb_instruction);

				if (nb_iteration > 1){
					loop = 1;

					if (i >= length){
						for (k = 0; k < length; k++){
							if (engine->trace->instructions[i - length + k].opcode != engine->trace->instructions[i + k].opcode){
								loop = 0;
								break;
							}
						}
					}
					else{
						loop = 0;
					}

					if (!loop){
						iteration_counter += nb_iteration;

						token.offset = i;
						token.length = length;
						token.nb_iteration = nb_iteration;

						if (array_add(&token_array, &token) < 0){
							printf("ERROR: in %s, unable to add token to array\n", __func__);
							continue;
						}
					}
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
			if (loopEngine_is_loop_unrolled(engine, i)){
				multiColumnPrinter_print(printer, i, engine->loops[i].nb_iteration, engine->loops[i].offset, engine->loops[i].length, engine->loops[i].epilogue, "unrolled", NULL);
			}
			else{
				multiColumnPrinter_print(printer, i, engine->loops[i].nb_iteration, engine->loops[i].offset, engine->loops[i].length, engine->loops[i].epilogue, "rolled", NULL);
			}
		}
	}

	multiColumnPrinter_delete(printer);
}

#pragma GCC diagnostic ignored "-Wunused-parameter" /* Due to the interface format */
int32_t loopEngine_export_it(struct loopEngine* engine, struct array* frag_array, uint32_t loop_index, uint32_t iteration_index){
	/*struct traceFragment 	fragment;

	if (engine->loops != NULL){
		if (loop_index < engine->nb_loop){
			if ((uint32_t)iteration_index < engine->loops[loop_index].nb_iteration){
				if (traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_NONE, NULL, NULL)){
					printf("ERROR: in %s, unable to init traceFragment\n", __func__);
					return -1;
				}

				if (array_copy(&(engine->element_array), &(fragment.instruction_array), engine->loops[loop_index].offset + engine->loops[loop_index].length * iteration_index, engine->loops[loop_index].length) !=  (int32_t)engine->loops[loop_index].length){
					printf("ERROR: in %s, unable to copy instruction from element_array to traceFragment\n", __func__);
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
	}*/

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter" /* Due to the interface format */
int32_t loopEngine_export_all(struct loopEngine* engine, struct array* frag_array, int32_t loop_index){
	/*uint32_t 				i;
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
			if (array_copy(&(engine->element_array), &(fragment.instruction_array), engine->loops[i].offset, total_length) !=  (int32_t)total_length){
				printf("ERROR: in %s, unable to copy instruction from element_array to traceFragment\n", __func__);
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
	}*/

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter" /* Due to the interface format */
int32_t loopEngine_export_noEp(struct loopEngine* engine, struct array* frag_array, int32_t loop_index){
	/*uint32_t 				i;
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
			if (array_copy(&(engine->element_array), &(fragment.instruction_array), engine->loops[i].offset, total_length) !=  (int32_t)total_length){
				printf("ERROR: in %s, unable to copy instruction from element_array to traceFragment\n", __func__);
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
	}*/

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

static int32_t loopEngine_is_loop_unrolled(struct loopEngine* engine, uint32_t index){
	uint32_t i;
	uint32_t j;
	ADDRESS pc1;
	ADDRESS pc2;

	for (i = 1; i < engine->loops[index].nb_iteration; i++){
		for (j = 0; j < engine->loops[index].length; j++){
			pc1 = engine->trace->instructions[engine->loops[index].offset + j].pc;
			pc2 = engine->trace->instructions[engine->loops[index].offset + j + (engine->loops[index].length * i)].pc;

			if (pc1 != pc2){
				return 1;
			}
		}
	}

	return 0;
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