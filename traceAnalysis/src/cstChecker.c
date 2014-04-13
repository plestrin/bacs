#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cstChecker.h"
#include "multiColumn.h"
#include "cstReaderJSON.h"
#include "printBuffer.h"
#ifdef VERBOSE
#include "workPercent.h"
#endif

char* constantType_to_string(enum constantType type){
	switch (type){
		case CST_TYPE_INVALID 	: {return "INVALID";}
		case CST_TYPE_CST_8 	: {return "CST_8";}
		case CST_TYPE_CST_16 	: {return "CST_16";}
		case CST_TYPE_CST_32 	: {return "CST_32";}
		case CST_TYPE_TAB_8 	: {return "TAB_8";}
		case CST_TYPE_TAB_16 	: {return "TAB_16";}
		case CST_TYPE_TAB_32 	: {return "TAB_32";}
		case CST_TYPE_LST_8 	: {return "LST_8";}
		case CST_TYPE_LST_16 	: {return "LST_16";}
		case CST_TYPE_LST_32 	: {return "LST_32";}
	}

	return NULL;
}

enum constantType constantType_from_string(const char* arg, uint32_t length){
	if (!strncmp(arg, "CST_8", length)){
		return CST_TYPE_CST_8;
	}
	else if (!strncmp(arg, "CST_16", length)){
		return CST_TYPE_CST_16;
	}
	else if (!strncmp(arg, "CST_32", length)){
		return CST_TYPE_CST_32;
	}
	else if (!strncmp(arg, "TAB_8", length)){
		return CST_TYPE_TAB_8;
	}
	else if (!strncmp(arg, "TAB_16", length)){
		return CST_TYPE_TAB_16;
	}
	else if (!strncmp(arg, "TAB_32", length)){
		return CST_TYPE_TAB_32;
	}
	else if (!strncmp(arg, "LST_8", length)){
		return CST_TYPE_LST_8;
	}
	else if (!strncmp(arg, "LST_16", length)){
		return CST_TYPE_LST_16;
	}
	else if (!strncmp(arg, "LST_32", length)){
		return CST_TYPE_LST_32;
	}
	else{
		printf("ERROR: in %s, incorrect constant type string\n", __func__);
		return CST_TYPE_INVALID;
	}
}

void constant_clean(struct constant* cst){
	if (CONSTANT_IS_TAB(cst->type)){
		free(cst->content.tab.buffer);
	}
	else if (CONSTANT_IS_LST(cst->type)){
		free(cst->content.list.buffer);
	}
}

struct cstChecker* cstChecker_create(){
	struct cstChecker* checker;

	checker = (struct cstChecker*)malloc(sizeof(struct cstChecker));
	if (checker != NULL){
		if (cstChecker_init(checker)){
			free(checker);
			checker = NULL;
		}
	}

	return checker;
}

int32_t cstChecker_init(struct cstChecker* checker){
	if (array_init(&(checker->cst_array), sizeof(struct constant))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return -1;
	}

	return 0;
}

void cstChecker_load(struct cstChecker* checker, char* arg){
	if (cstReaderJSON_parse(arg, &(checker->cst_array))){
		printf("ERROR: in %s, unable to parse constant file: \"%s\"\n", __func__, (char*)arg);
	}
}

void cstChecker_print(struct cstChecker* checker){
	uint32_t 					i;
	struct constant* 			cst;
	struct multiColumnPrinter* 	printer;
	char* 						content_string;
	uint32_t 					content_string_length;
	char* 						content;
	uint32_t 					content_size;

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 32);
		multiColumnPrinter_set_column_size(printer, 1, 10);

		multiColumnPrinter_set_title(printer, 0, (char*)"NAME");
		multiColumnPrinter_set_title(printer, 1, (char*)"TYPE");
		multiColumnPrinter_set_title(printer, 2, (char*)"VALUE");

		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UNBOUND_STRING);

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(checker->cst_array)); i++){
			cst = (struct constant*)array_get(&(checker->cst_array), i);

			switch(cst->type){
				case CST_TYPE_INVALID 	: {
					printf("ERROR: in %s, constant type is INVALID\n", __func__);
					continue;
				}
				case CST_TYPE_CST_8 		: {
					content = (char*)&(cst->content.value);
					content_size = 1;
					break;
				}
				case CST_TYPE_CST_16 		: {
					content = (char*)&(cst->content.value);
					content_size = 2;
					break;
				}
				case CST_TYPE_CST_32 		: {
					content = (char*)&(cst->content.value);
					content_size = 4;
					break;
				}
				case CST_TYPE_TAB_8 		: {
					content = (char*)cst->content.tab.buffer;
					content_size = cst->content.tab.nb_element;
					break;
				}
				case CST_TYPE_TAB_16 	: {
					content = (char*)cst->content.tab.buffer;
					content_size = 2 * cst->content.tab.nb_element;
					break;
				}
				case CST_TYPE_TAB_32 	: {
					content = (char*)cst->content.tab.buffer;
					content_size = 4 * cst->content.tab.nb_element;
					break;
				}
				case CST_TYPE_LST_8 		: {
					content = (char*)cst->content.list.buffer;
					content_size = cst->content.list.nb_element;
					break;
				}
				case CST_TYPE_LST_16 	: {
					content = (char*)cst->content.list.buffer;
					content_size = 2 * cst->content.list.nb_element;
					break;
				}
				case CST_TYPE_LST_32 	: {
					content = (char*)cst->content.list.buffer;
					content_size = 4 * cst->content.list.nb_element;
					break;
				}
				default 				: {
					printf("ERROR: in %s, incorrect constant type\n", __func__);
					continue;
				}
			}

			content_string_length = PRINTBUFFER_GET_STRING_SIZE(content_size);
			content_string = (char*)malloc(content_string_length);
			if (content_string == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				continue;
			}
			printBuffer_raw_string(content_string, content_string_length, content, content_size);

			multiColumnPrinter_print(printer, cst->name, constantType_to_string(cst->type), content_string, NULL);

			free(content_string);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

void cstChecker_empty(struct cstChecker* checker){
	uint32_t i;

	for (i = 0; i < array_get_length(&(checker->cst_array)); i++){
		constant_clean((struct constant*)array_get(&(checker->cst_array), i));
	}

	array_empty(&(checker->cst_array));
}

int32_t cstChecker_check(struct cstChecker* checker, struct trace* trace){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	uint32_t 					l;
	union cstResult* 			cst_result;
	struct constant* 			cst;
	struct operand* 			operands;
	struct cstTableAccess		tab_hit;
	struct multiColumnPrinter* 	printer;
	#define STRING_HIT_LENGTH	24
	#define STRING_INFO_LENGTH 	24
	char 						string_hit[STRING_HIT_LENGTH];
	char 						string_info[STRING_INFO_LENGTH];
	#ifdef VERBOSE
	struct workPercent 			work;
	#endif

	cst_result = (union cstResult*)malloc(sizeof(union cstResult) * array_get_length(&(checker->cst_array)));
	if (cst_result == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for (i = 0; i < array_get_length(&(checker->cst_array)); i++){
		cst = (struct constant*)array_get(&(checker->cst_array), i);

		if (CONSTANT_IS_CST(cst->type)){
			cst_result[i].cst_hit_counter = 0;
		}
		else if (CONSTANT_IS_TAB(cst->type)){
			if (array_init(&(cst_result[i].tab_hit_counter), sizeof(struct cstTableAccess))){
				printf("ERROR: in %s, unable to init array\n", __func__);
			}
		}
		else if (CONSTANT_IS_LST(cst->type)){
			cst_result[i].lst_hit_counter = (uint32_t*)calloc(cst->content.list.nb_element, sizeof(uint32_t));
			if (cst_result[i].lst_hit_counter == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, incorrect constant type\n", __func__);
		}
	}

	#ifdef VERBOSE
	if (workPercent_init(&work, "Searching constant(s) ", WORKPERCENT_ACCURACY_1, trace->nb_instruction)){
		printf("ERROR: in %s, unable to init workPercent\n", __func__);
	}
	#endif

	for (j = 0; j < trace->nb_instruction; j++){
		operands = trace_get_ins_operands(trace, j);
		for (k = 0; k < trace->instructions[j].nb_operand; k++){
			if (OPERAND_IS_READ(operands[k])){
				for (i = 0; i < array_get_length(&(checker->cst_array)); i++){
					cst = (struct constant*)array_get(&(checker->cst_array), i);

					if (CONSTANT_GET_ELEMENT_SIZE(cst->type) == operands[k].size){
						if (CONSTANT_IS_CST(cst->type)){
							if (!memcmp(&(cst->content.value), trace_get_ins_op_data(trace, j, k), operands[k].size)){
								cst_result[i].cst_hit_counter ++;
							}
						}
						else if (CONSTANT_IS_TAB(cst->type) && OPERAND_IS_MEM(operands[k])){
							for (l = 0; l < cst->content.tab.nb_element; l++){
								if (!memcmp((char*)cst->content.tab.buffer + l * operands[k].size, trace_get_ins_op_data(trace, j, k), operands[k].size)){
									tab_hit.offset = l;
									tab_hit.address = operands[k].location.address;
									if (array_add(&(cst_result[i].tab_hit_counter), &tab_hit) < 0){
										printf("ERROR: in %s, unable to add element to array\n", __func__);
									}
								}
							}
						}
						else if (CONSTANT_IS_LST(cst->type)){
							/* a completer */
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
	#endif

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_title(printer, 0, (char*)"NAME");
		multiColumnPrinter_set_title(printer, 1, (char*)"HIT");
		multiColumnPrinter_set_title(printer, 2, (char*)"INFO");

		multiColumnPrinter_print_header(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}

	for (i = 0; i < array_get_length(&(checker->cst_array)); i++){
		cst = (struct constant*)array_get(&(checker->cst_array), i);
		
		if (CONSTANT_IS_CST(cst->type)){
			if (cst_result[i].cst_hit_counter > 0){
				snprintf(string_hit, STRING_HIT_LENGTH, "%u", cst_result[i].cst_hit_counter);
				memset(string_info, '\0', STRING_INFO_LENGTH);
				if (printer != NULL){
					multiColumnPrinter_print(printer, cst->name, string_hit, string_info, NULL);
				}
			}
		}
		else if (CONSTANT_IS_TAB(cst->type)){
			ADDRESS 				tab_address;
			ADDRESS 				tab_max_address;
			uint32_t				tab_score;
			uint32_t				tab_max_score = 0;
			struct cstTableAccess* 	access1;
			struct cstTableAccess* 	access2;
			uint8_t* 				taken;
			uint8_t 				element_size = CONSTANT_GET_ELEMENT_SIZE(cst->type);
			
			taken = (uint8_t*)malloc(cst->content.tab.nb_element);
			if (taken == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				continue;
			}

			for (j = 0; j < array_get_length(&(cst_result[i].tab_hit_counter)); j++){
				access1 = array_get(&(cst_result[i].tab_hit_counter), j);
				if (access1->address != 0){
					tab_score = 1;
					tab_address = access1->address - (access1->offset * element_size);
					memset(taken, 0, cst->content.tab.nb_element);
					taken[access1->offset] = 1;

					for (k = j + 1; k < array_get_length(&(cst_result[i].tab_hit_counter)); k++){
						access2 = array_get(&(cst_result[i].tab_hit_counter), k);
						if (access2->address != 0 && tab_address + (access2->offset * element_size) == access2->address){
							if(!taken[access2->offset]){
								tab_score ++;
								taken[access2->offset] = 1;
							}
							access2->address = 0;
						}
					}

					if (tab_score > tab_max_score){
						tab_max_score = tab_score;
						tab_max_address = tab_address;
					}

					access1->address = 0;
				}
			}

			if (tab_max_score >= CONSTANT_THRESHOLD_TAB){
				snprintf(string_hit, STRING_HIT_LENGTH, "%u", tab_max_score);
				#if defined ARCH_32
				snprintf(string_info, STRING_INFO_LENGTH, "0x%08x", tab_max_address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				snprintf(string_info, STRING_INFO_LENGTH, "0x%llx", tab_max_address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif

				if (printer != NULL){
					multiColumnPrinter_print(printer, cst->name, string_hit, string_info, NULL);
				}
			}

			free(taken);
			array_clean(&(cst_result[i].tab_hit_counter));
		}
		else if (CONSTANT_IS_LST(cst->type)){
			/*a completer */
			free(cst_result[i].lst_hit_counter);
		}
	}

	if (printer != NULL){
		multiColumnPrinter_delete(printer);
	}

	free(cst_result);

	return 0;

	#undef STRING_HIT_LENGTH
	#undef STRING_INFO_LENGTH
}

void cstChecker_clean(struct cstChecker* checker){
	uint32_t i;

	for (i = 0; i < array_get_length(&(checker->cst_array)); i++){
		constant_clean((struct constant*)array_get(&(checker->cst_array), i));
	}

	array_clean(&(checker->cst_array));
}

void cstChecker_delete(struct cstChecker* checker){
	if (checker != NULL){
		cstChecker_clean(checker);
	}
}