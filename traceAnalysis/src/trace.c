#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "trace.h"
#include "mapFile.h"

#define TRACE_PATH_MAX_LENGTH 	256
#define TRACE_INS_FILE_NAME 	"ins.bin"
#define TRACE_OP_FILE_NAME 		"op.bin"
#define TRACE_DATA_FILE_NAME 	"data.bin"
#define TRACE_BLOCKID_FILE_NAME "blockId0.bin"
#define TRACE_BLOCK_FILE_NAME 	"block.bin"

static void trace_clean_(struct trace* trace);

struct trace* trace_create(const char* directory_path){
	struct trace* trace;

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace != NULL){
		if (trace_init(trace, directory_path)){
			free(trace);
			trace= NULL;
		}
	}

	return trace;
}

int32_t trace_init(struct trace* trace, const char* directory_path){
	char 	file1_path[TRACE_PATH_MAX_LENGTH];
	char 	file2_path[TRACE_PATH_MAX_LENGTH];
	size_t 	map_size;

	snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_INS_FILE_NAME);
	trace->instructions = mapFile_map(file1_path, &map_size);
	trace->alloc_size_ins = map_size;

	snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_OP_FILE_NAME);
	trace->operands = mapFile_map(file1_path, &map_size);
	trace->alloc_size_op = map_size;

	snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_DATA_FILE_NAME);
	trace->data = mapFile_map(file1_path, &map_size);
	trace->alloc_size_data = map_size;
	
	trace->reference_count 	= 1;
	trace->allocation_type 	= TRACEALLOCATION_MMAP;

	if (trace->instructions == NULL || trace->operands == NULL || trace->data == NULL){
		printf("ERROR: in %s, unable to map memory\n", __func__);
		trace_clean(trace);
		return -1;
	}

	if (trace->alloc_size_ins % sizeof(struct instruction) != 0){
		printf("ERROR: in %s, incorrect instruction file size %u bytes, must be a multiple of %u\n", __func__, trace->alloc_size_ins, sizeof(struct instruction));
		trace_clean(trace);
		return -1;
	}

	if (trace->alloc_size_op % sizeof(struct operand) != 0){
		printf("ERROR: in %s, incorrect operand file size %u bytes, must be a multiple of %u\n", __func__, trace->alloc_size_op, sizeof(struct operand));
		trace_clean(trace);
		return -1;
	}

	trace->nb_instruction = trace->alloc_size_ins / sizeof(struct instruction);

	snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_BLOCKID_FILE_NAME);
	snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_BLOCK_FILE_NAME);
	if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
		printf("ERROR: in %s, unable to init assembly structure\n", __func__);
	}

	return 0;
}

void trace_check(struct trace* trace){
	uint32_t i;
	uint32_t expected_offset;

	#ifdef VERBOSE
	printf("Trace verification: %u instruction(s), %u operand(s) and %u byte(s) of data\n", trace->nb_instruction, trace_get_nb_operand(trace), trace->alloc_size_data);
	#endif

	/* Operand offset verification */
	for (i = 0, expected_offset = 0; i < trace->nb_instruction; i++){
		if (trace->instructions[i].nb_operand != 0){
			if (trace->instructions[i].operand_offset != expected_offset){
				printf("ERROR: in %s, instruction %u, expected operand offset %u, but get %u - (previous instruction nb operand(s): %u, opcode: %s, offset: %u)\n", __func__, i, expected_offset, trace->instructions[i].operand_offset, trace->instructions[i - 1].nb_operand, xed_iclass_enum_t2str(trace->instructions[i - 1].opcode), trace->instructions[i - 1].operand_offset);
			}
			expected_offset = trace->instructions[i].operand_offset + trace->instructions[i].nb_operand;
			if (expected_offset * sizeof(struct operand) > trace->alloc_size_op){
				printf("ERROR: in %s, instruction %u, operand offset is outside the operand buffer\n", __func__, i);
			}
		}
	}
	if (expected_offset * sizeof(struct operand) != trace->alloc_size_op){
		printf("ERROR: in %s, the end of the operand buffer is not reached\n", __func__);
	}

	/* Data offset verification */
	for (i = 0, expected_offset = 0; i < trace_get_nb_operand(trace); i++){
		if (trace->operands[i].data_offset != expected_offset){
			printf("ERROR: in %s, operand %u, expected data offset %u, but get %u\n", __func__, i, expected_offset, trace->operands[i].data_offset);
		}
		expected_offset = trace->operands[i].data_offset + trace->operands[i].size;
		if (expected_offset > trace->alloc_size_data){
			printf("ERROR: in %s, operand %u, data offset is outside the data buffer\n", __func__, i);
		}
	}
	if (expected_offset != trace->alloc_size_data){
		printf("ERROR: in %s, the end of the data buffer is not reached\n", __func__);
	}

	/* Operand Valid verification */
	for (i = 0; i < trace_get_nb_operand(trace); i++){
		if (OPERAND_IS_INVALID(trace->operands[i])){
			printf("ERROR: in %s, operand %u is invalid\n", __func__, i);
		}
	}

	/* Assembly verification */
	if (trace->nb_instruction != assembly_get_nb_instruction(&(trace->assembly))){
		printf("ERROR: in %s, the number of instruction is different between the trace (%u) and the assembly (%u)\n", __func__, trace->nb_instruction, assembly_get_nb_instruction(&(trace->assembly)));
	}
	else{
		if (assembly_check(&(trace->assembly))){
			printf("ERROR: in %s, assembly check failed\n", __func__);
		}
	}
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
			if (OPERAND_IS_READ(operands[j])){
				current_str = read_access + str_read_offset;
				current_offset = &str_read_offset;
			}
			else if (OPERAND_IS_WRITE(operands[j])){
				current_str = write_access + str_write_offset;
				current_offset = &str_write_offset;
			}
			else{
				printf("ERROR: in %s, unexpected data type (READ or WRITE)\n", __func__);
				break;
			}

			if (OPERAND_IS_MEM(operands[j])){
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
			else if (OPERAND_IS_REG(operands[j])){
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
		multiColumnPrinter_print(printer, trace->instructions[i].pc, xed_iclass_enum_t2str(trace->instructions[i].opcode), read_access, write_access, NULL);
	}

	if (delete_printer){
		multiColumnPrinter_delete(printer);
	}
}

void trace_print_asm(struct trace* trace, uint32_t start, uint32_t stop){
	uint32_t 					i;
	struct instructionIterator 	it;
	char 						buffer[MULTICOLUMN_STRING_MAX_SIZE];
	xed_print_info_t 			print_info;

	if (assembly_get_instruction(&(trace->assembly), &it, start)){
		printf("ERROR: in %s, unable to fetch instruction %u from the assembly\n", __func__, start);
		return;
	}

	xed_init_print_info(&print_info);
	print_info.blen 					= 256;
	print_info.buf  					= buffer;
	print_info.context  				= NULL;
	print_info.disassembly_callback		= NULL;
	print_info.format_options_valid 	= 0;
	print_info.p 						= &(it.xedd);
	print_info.runtime_address 			= it.instruction_address;
	print_info.syntax 					= XED_SYNTAX_INTEL;

	if (xed_format_generic(&print_info)){
		printf("0x%08x  %s\n", it.instruction_address, buffer);
	}
	else{
		printf("ERROR: in %s, xed_format_generic returns an error code\n", __func__);
	}

	for (i = start + 1; i < stop && i < assembly_get_nb_instruction(&(trace->assembly)); i++){
		if (assembly_get_next_instruction(&(trace->assembly), &it)){
			printf("ERROR: in %s, unable to fetch next instruction %u from the assembly\n", __func__, i);
			break;
		}

		xed_init_print_info(&print_info);
		print_info.blen 					= 256;
		print_info.buf  					= buffer;
		print_info.context  				= NULL;
		print_info.disassembly_callback		= NULL;
		print_info.format_options_valid 	= 0;
		print_info.p 						= &(it.xedd);
		print_info.runtime_address 			= it.instruction_address;
		print_info.syntax 					= XED_SYNTAX_INTEL;

		if (xed_format_generic(&print_info)){
			printf("0x%08x  %s\n", it.instruction_address, buffer);
		}
		else{
			printf("ERROR: in %s, xed_format_generic returns an error code\n", __func__);
		}
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
		nb_operand = trace_src->instructions[j].nb_operand;
		if (nb_operand > 0){
			nb_data = trace_src->operands[trace_src->instructions[j].operand_offset + trace_src->instructions[j].nb_operand - 1].data_offset + trace_src->operands[trace_src->instructions[j].operand_offset + trace_src->instructions[j].nb_operand - 1].size - trace_src->operands[trace_src->instructions[j].operand_offset].data_offset;
		}
		else{
			nb_data = 0;
		}
		offset_operand = trace_src->instructions[j].operand_offset;
		offset_data = trace_src->operands[offset_operand].data_offset;
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

	trace_dst->alloc_size_ins	= length * sizeof(struct instruction);
	trace_dst->alloc_size_op 	= nb_operand * sizeof(struct operand);
	trace_dst->alloc_size_data 	= nb_data * sizeof(uint8_t);
	trace_dst->nb_instruction 	= length;
	trace_dst->reference_count 	= 1;
	trace_dst->allocation_type 	= TRACEALLOCATION_MALLOC;


	trace_dst->instructions 	= (struct instruction*)malloc(trace_dst->alloc_size_ins);
	trace_dst->operands 		= (struct operand*)malloc(trace_dst->alloc_size_op);
	trace_dst->data 			= (uint8_t*)malloc(trace_dst->alloc_size_data);

	if (trace_dst->instructions == NULL || trace_dst->operands == NULL || trace_dst->data == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
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

	if (assembly_extract_segment(&(trace_src->assembly), &(trace_dst->assembly), offset, length)){
		printf("ERROR: in %s, unable to extract assembly segment\n", __func__);
		return -1;
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

	assembly_clean(&(trace->assembly));
}
