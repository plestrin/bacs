#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "trace.h"

static void trace_clean_(struct trace* trace);

struct trace* trace_create(struct array* array){ /*tmp*/
	struct trace* trace;

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace != NULL){
		if (trace_init(trace, array)){ /* a completer */
			free(trace);
			trace= NULL;
		}
	}

	return trace;
}

int32_t trace_init(struct trace* trace, struct array* array){ /*tmp*/
	uint32_t 				i;
	uint32_t 				j;
	struct instruction* 	ins;

	int32_t 				operand_counter = 0;
	int32_t 				data_counter = 0;

	for (i = 0; i < array_get_length(array); i++){
		ins = (struct instruction*)array_get(array, i);
		for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
			if (INSTRUCTION_DATA_TYPE_IS_VALID(ins->data[j].type)){
				operand_counter ++;
				data_counter +=  ins->data[j].size;
			}
		}
	}

	trace->alloc_size_ins	= array_get_length(array) * sizeof(struct _instruction);
	trace->alloc_size_op 	= operand_counter * sizeof(struct operand);
	trace->alloc_size_data 	= data_counter * sizeof(uint8_t);
	trace->nb_instruction 	= array_get_length(array);
	trace->reference_count 	= 1;
	trace->allocation_type 	= TRACEALLOCATION_MMAP;

	trace->instructions = (struct _instruction*)mmap(NULL, trace->alloc_size_ins, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	trace->operands = (struct operand*)mmap(NULL, trace->alloc_size_op, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	trace->data = (uint8_t*)mmap(NULL, trace->alloc_size_data, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (trace->instructions == NULL || trace->operands == NULL || trace->data == NULL){
		printf("ERROR: in %s, unable to map memory\n", __func__);
		trace_clean(trace);
		return -1;
	}

	operand_counter = 0;
	data_counter = 0;

	for (i = 0; i < array_get_length(array); i++){
		ins = (struct instruction*)array_get(array, i);
		
		trace->instructions[i].pc = ins->pc;
		trace->instructions[i].opcode = ins->opcode;
		trace->instructions[i].operand_offset = operand_counter;

		for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
			if (INSTRUCTION_DATA_TYPE_IS_VALID(ins->data[j].type)){
				memcpy(trace->data + data_counter, &(ins->data[j].value), ins->data[j].size);

				trace->operands[operand_counter].type = ins->data[j].type;
				if (INSTRUCTION_DATA_TYPE_IS_MEM(ins->data[j].type)){
					trace->operands[operand_counter].location.address = ins->data[j].location.address;
				}
				else if (INSTRUCTION_DATA_TYPE_IS_REG(ins->data[j].type)){
					trace->operands[operand_counter].location.reg = ins->data[j].location.reg;
				}
				else{
					printf("ERROR: in %s, incorrect operand type\n", __func__);
				}
				trace->operands[operand_counter].size = ins->data[j].size;
				trace->operands[operand_counter].data_offset = data_counter;

				data_counter += ins->data[j].size;
				operand_counter ++;
			}
		}
		trace->instructions[i].nb_operand = operand_counter - trace->instructions[i].operand_offset;
	}

	return 0;
}

struct multiColumnPrinter* trace_create_multiColumnPrinter(){
	struct multiColumnPrinter* printer;

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		#if defined ARCH_32
		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_HEX_32);
		#elif defined ARCH_64
		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_HEX_64);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif

		multiColumnPrinter_set_column_size(printer, 2, 96);
		multiColumnPrinter_set_column_size(printer, 3, 32);

		multiColumnPrinter_set_title(printer, 0, "PC");
		multiColumnPrinter_set_title(printer, 1, "Opcode");
		multiColumnPrinter_set_title(printer, 2, "Read Access");
		multiColumnPrinter_set_title(printer, 3, "Write Access");
	}
	else{
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
	}

	return printer;
}

void trace_print(struct trace* trace, uint32_t start, uint32_t stop, struct multiColumnPrinter* printer){
	uint32_t 		i;
	uint32_t 		j;
	uint8_t 		delete_printer = 0;
	struct operand* operands;
	char 			read_access[MULTICOLUMN_STRING_MAX_SIZE];
	char 			write_access[MULTICOLUMN_STRING_MAX_SIZE];
	uint32_t 		str_read_offset;
	uint32_t 		str_write_offset;
	char* 			current_str;
	uint32_t*		current_offset;
	uint32_t		nb_byte_written;


	if (printer == NULL){
		printer = trace_create_multiColumnPrinter();
		delete_printer = 1;
		if (printer == NULL){
			printf("ERROR: in %s, unable to create specific multiColumnPrinter\n", __func__);
			return;
		}

		if (start < stop && start < trace->nb_instruction){
			multiColumnPrinter_print_header(printer);
		}
	}

	for (i = start; i < stop && i < trace->nb_instruction; i++){
		operands = trace->operands + trace->instructions[i].operand_offset;
		read_access[0] 		= '\0';
		write_access[0] 	= '\0';
		str_read_offset 	= 0;
		str_write_offset 	= 0;

		for (j = 0; j < trace->instructions[i].nb_operand; j++){
			if (INSTRUCTION_DATA_TYPE_IS_READ(operands[j].type)){
				current_str = read_access + str_read_offset;
				current_offset = &str_read_offset;
			}
			else if (INSTRUCTION_DATA_TYPE_IS_WRITE(operands[j].type)){
				current_str = write_access + str_write_offset;
				current_offset = &str_write_offset;
			}
			else{
				printf("ERROR: in %s, unexpected data type (READ or WRITE)\n", __func__);
				break;
			}

			if (INSTRUCTION_DATA_TYPE_IS_MEM(operands[j].type)){
				#if defined ARCH_32
				nb_byte_written = snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "{Mem @ %08x ", operands[j].location.address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				nb_byte_written = snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "{Mem @ %llx ", operands[j].location.address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
				current_str += nb_byte_written;
				*current_offset += nb_byte_written;
			}
			else if (INSTRUCTION_DATA_TYPE_IS_REG(operands[j].type)){
				nb_byte_written = snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "{%s ", reg_2_string(operands[j].location.reg));
				current_str += nb_byte_written;
				*current_offset += nb_byte_written;
			}
			else{
				printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
				break;
			}

			switch(operands[j].size){
				case 1 	: {*current_offset += snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "0x%02x}", *(uint8_t* )(trace->data + operands[j].data_offset)); break;}
				case 2 	: {*current_offset += snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "0x%04x}", *(uint16_t*)(trace->data + operands[j].data_offset)); break;}
				case 4 	: {*current_offset += snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "0x%08x}", *(uint32_t*)(trace->data + operands[j].data_offset)); break;}
				default : {printf("ERROR: in %s, unexpected data size\n", __func__); break;}
			}
		}
		multiColumnPrinter_print(printer, trace->instructions[i].pc, instruction_opcode_2_string(trace->instructions[i].opcode), read_access, write_access, NULL);
	}

	if (delete_printer){
		multiColumnPrinter_delete(printer);
	}
}

int32_t trace_extract_segment(struct trace* trace_src, struct trace* trace_dst, uint32_t offset, uint32_t length){
	uint32_t i;
	uint32_t j;
	uint32_t nb_operand;
	uint32_t nb_data;
	uint32_t offset_operand;
	uint32_t offset_data;

	if (offset + length > trace_src->nb_instruction || length == 0){
		printf("ERROR: in %s, incorrect parameters: offset: %u, length: %u\n", __func__, offset, length);
		return -1;
	}

	j = offset + length - 1;
	while(trace_src->instructions[j].nb_operand == 0 && j > offset){
		j --;
	}
	if (j == offset){
		nb_operand = 0;
		nb_data = 0;
		offset_operand = 0;
		offset_data = 0;
	}
	else{
		nb_operand = trace_src->instructions[j].operand_offset + trace_src->instructions[j].nb_operand;
		nb_data = trace_src->operands[trace_src->instructions[j].operand_offset + trace_src->instructions[j].nb_operand - 1].data_offset + trace_src->operands[trace_src->instructions[j].operand_offset + trace_src->instructions[j].nb_operand - 1].size;
	
		j = offset;
		while(trace_src->instructions[j].nb_operand == 0 && j < length){
			j ++;
		}
		nb_operand -= trace_src->instructions[j].operand_offset;
		nb_data -= trace_src->operands[trace_src->instructions[j].operand_offset].data_offset;
		offset_operand = trace_src->instructions[j].operand_offset;
		offset_data = trace_src->operands[offset_operand].data_offset;
	}

	trace_dst->alloc_size_ins	= length * sizeof(struct _instruction);
	trace_dst->alloc_size_op 	= nb_operand * sizeof(struct operand);
	trace_dst->alloc_size_data 	= nb_data * sizeof(uint8_t);
	trace_dst->nb_instruction 	= length;
	trace_dst->reference_count 	= 1;
	trace_dst->allocation_type 	= TRACEALLOCATION_MALLOC;


	trace_dst->instructions 	= (struct _instruction*)malloc(trace_dst->alloc_size_ins);
	trace_dst->operands 		= (struct operand*)malloc(trace_dst->alloc_size_op);
	trace_dst->data 			= (uint8_t*)malloc(trace_dst->alloc_size_data);

	if (trace_dst->instructions == NULL || trace_dst->operands == NULL || trace_dst->data == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		trace_clean(trace_dst);
		return -1;
	}

	memcpy(trace_dst->instructions, trace_src->instructions + offset, trace_dst->alloc_size_ins);
	memcpy(trace_dst->operands, trace_src->operands + offset_operand, trace_dst->alloc_size_op);
	memcpy(trace_dst->data, trace_src->data + offset_data, trace_dst->alloc_size_data);

	for (i = 0; i < length; i++){
		if (trace_dst->instructions[i].nb_operand > 0){
			trace_dst->instructions[i].operand_offset -= offset_operand;
			for (j = 0; j < trace_dst->instructions[i].nb_operand; j++){
				trace_dst->operands[trace_dst->instructions[i].operand_offset + j].data_offset -= offset_data;
			}
		}
	}

	return 0;
}

void trace_clean(struct trace* trace){
	if (trace != NULL && --trace->reference_count == 0){
		trace_clean_(trace);
	}
}

void trace_delete(struct trace* trace){
	if (trace != NULL && --trace->reference_count == 0){
		trace_clean_(trace);
		free(trace);
	}
}

static void trace_clean_(struct trace* trace){
	if (trace->allocation_type == TRACEALLOCATION_MMAP){
		if (trace->instructions != NULL){
			munmap(trace->instructions, trace->alloc_size_ins);
		}
		if (trace->operands != NULL){
			munmap(trace->operands, trace->alloc_size_op);
		}
		if (trace->data != NULL){
			munmap(trace->data, trace->alloc_size_data);
		}
	}
	else if (trace->allocation_type == TRACEALLOCATION_MALLOC){
		if (trace->instructions != NULL){
			free(trace->instructions);
		}
		if (trace->operands != NULL){
			free(trace->operands);
		}
		if (trace->data != NULL){
			free(trace->data);
		}
	}
	else{
		printf("ERROR: in %s, incorrect allocation type\n", __func__);
	}
}
