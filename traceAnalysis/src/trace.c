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
	char 		file_path[TRACE_PATH_MAX_LENGTH];
	uint64_t 	map_size;

	snprintf(file_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_INS_FILE_NAME);
	trace->instructions = mapFile_map(file_path, &map_size);
	trace->alloc_size_ins = map_size;

	snprintf(file_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_OP_FILE_NAME);
	trace->operands = mapFile_map(file_path, &map_size);
	trace->alloc_size_op = map_size;

	snprintf(file_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_DATA_FILE_NAME);
	trace->data = mapFile_map(file_path, &map_size);
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
				printf("ERROR: in %s, instruction %u, expected operand offset %u, but get %u - (previous instruction nb operand(s): %u, opcode: %s, offset: %u)\n", __func__, i, expected_offset, trace->instructions[i].operand_offset, trace->instructions[i - 1].nb_operand, instruction_opcode_2_string(trace->instructions[i - 1].opcode), trace->instructions[i - 1].operand_offset);
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

/* Beginning of the experimental party */

static void trace_analysis_propagate_mem_group(struct operand* operands, uint32_t* group, uint32_t nb_operand){
	uint32_t i;

	if (nb_operand > 1){
		if (group[0] && OPERAND_IS_MEM(operands[0])){
			for (i = 1; i < nb_operand; i++){
				if (OPERAND_IS_MEM(operands[i])){
					/* this test is weak improve later */
					if (operands[i].location.address == operands[0].location.address){
						if (OPERAND_IS_READ(operands[i])){
							if (!group[i]){
								group[i] = group[0];
							}
							else{
								printf("ERROR: in %s, unable to propagate group because element already belongs to one\n", __func__);
							}
						}
						else{
							break;
						}
					}
				}
			}
		}
		else{
			printf("ERROR: in %s, incorrect group value (%u) to propagate or operand type\n", __func__, group[0]);
		}
	}
}

static void trace_analysis_propagate_reg_group(struct operand* operands, uint32_t* group, uint32_t nb_operand){
	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint8_t other_register[NB_REGISTER];

	if (nb_operand > 1){
		if (group[0] && OPERAND_IS_REG(operands[0])){
			for (j =0; j < NB_REGISTER; j++){
				if (reg_is_contained_in((enum reg)operands[0].location.reg, (enum reg)(j + 1))){
					other_register[j] = 1;
				}
				else{
					other_register[j] = 0;
				}
			}

			for (i = 1; i < nb_operand; i++){
				if (OPERAND_IS_REG(operands[i])){
					for (j = 0; j < NB_REGISTER; j++){
						if (other_register[j]){
							if (reg_is_contained_in((enum reg)operands[i].location.reg, (enum reg)(j + 1))){
								if ((group[i] == 0 || reg_is_contained_in((enum reg)operands[i].location.reg, (enum reg)operands[0].location.reg)) && OPERAND_IS_READ(operands[i])){
									group[i] = group[0];
									break;
								}
								else if (group[i]){
									printf("WARNING: in %s, the register %s already belongs to a group\n", __func__, reg_2_string((enum reg)operands[i].location.reg));
									other_register[j] = 0;
								}
								else{
									for (k = 0; k < NB_REGISTER; k++){
										if (reg_is_contained_in((enum reg)(k + 1), (enum reg)operands[i].location.reg)){
											other_register[k] = 0;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else{
			printf("ERROR: in %s, incorrect group value (%u) to propagate or operand type\n", __func__, group[0]);
		}
	}
}

static void trace_analysis_print_group(struct trace* trace, uint32_t* group, uint32_t* op_to_ins, uint32_t nb_operand, uint32_t nb_group){
	uint32_t 	i;
	uint32_t 	j;
	uint8_t 	start_group;
	uint32_t 	nb_grp = 0;

	#define FILTER_INPUT 1

	printf("Group information:\n");
	#if FILTER_INPUT == 1
	printf("Filter on INPUT values:\n");
	#endif

	for (i = 0; i < nb_group; i++){
		for (j = 0, start_group = 0; j < nb_operand; j++){
			if (group[j] == i + 1){
				if (!start_group){
					if (OPERAND_IS_READ(trace->operands[j])){
						printf("- Gr: %03u {%02u:R:%s", i+1, op_to_ins[j], instruction_opcode_2_string(trace->instructions[op_to_ins[j]].opcode));
					}
					else if (OPERAND_IS_WRITE(trace->operands[j])){
						#if FILTER_INPUT == 1
						break;
						#else
						printf("- Gr: %03u {%02u:W:%s", i+1, op_to_ins[j], instruction_opcode_2_string(trace->instructions[op_to_ins[j]].opcode));
						#endif
					}
					else{
						printf("ERROR: in %s, incorrect operand type\n", __func__);
					}
					start_group = 1;
				}
				else{
					if (OPERAND_IS_READ(trace->operands[j])){
						printf(", %02u:R:%s", op_to_ins[j], instruction_opcode_2_string(trace->instructions[op_to_ins[j]].opcode));
					}
					else if (OPERAND_IS_WRITE(trace->operands[j])){
						printf(", %02u:W:%s", op_to_ins[j], instruction_opcode_2_string(trace->instructions[op_to_ins[j]].opcode));
					}
					else{
						printf("ERROR: in %s, incorrect operand type\n", __func__);
					}
				}
			}
		}
		if (start_group){
			printf("}\n");
			nb_grp ++;
		}
	}

	printf("Number of group: %u\n", nb_grp);
}

#include "array.h"

#define TRACE_ANALYSIS_GRP_MAX_SIZE 64

enum variableRole{
	ROLE_UNKNOWN,
	ROLE_READ_DIRECT,
	ROLE_READ_BASE,
	ROLE_READ_INDEX
};

static char* variableRole_2_string(enum variableRole role){
	switch(role){
		case ROLE_UNKNOWN 			: {return "UN";}
		case ROLE_READ_DIRECT 		: {return "DI";}
		case ROLE_READ_BASE 		: {return "BA";}
		case ROLE_READ_INDEX 		: {return "IN";}
	}

	return NULL;
}

static enum variableRole trace_analysis_get_operand_role(struct trace* trace, uint32_t index_op){
	enum variableRole role = ROLE_UNKNOWN;


	if (OPERAND_IS_MEM(trace->operands[index_op])){
		role = ROLE_READ_DIRECT;
	}
	else{
		if (OPERAND_IS_BASE(trace->operands[index_op])){
			role = ROLE_READ_BASE;
		}
		else if (OPERAND_IS_INDEX(trace->operands[index_op])){
			role = ROLE_READ_INDEX;
		}
		else{
			role = ROLE_READ_DIRECT;
		}
	}

	return role;
}

struct action{
	xed_iclass_enum_t 	opcode;
	enum variableRole 	role;
};

static uint32_t trace_analysis_extract_spec_seq(struct trace* trace, uint32_t* group, uint32_t group_id, uint32_t* op_to_ins, struct action* sequence, uint32_t sequence_size, uint32_t input, enum variableRole parent_role){
	uint32_t j;
	uint32_t nb_operand = trace_get_nb_operand(trace);
	uint32_t seq_offset = 0;

	for (j = 0; j < nb_operand; j++){
		if (group[j] == group_id){
			if (input && seq_offset == 0 && OPERAND_IS_WRITE(trace->operands[j])){
				break;
			}

			if (seq_offset == sequence_size){
				printf("ERROR: in %s, sequence size has been reached\n", __func__);
				break;
			}

			/* we suppose that the previous MOV was a read ... we should make further verification */
			if ((xed_iclass_enum_t)trace->instructions[op_to_ins[j]].opcode == XED_ICLASS_MOV && OPERAND_IS_WRITE(trace->operands[j]) && sequence[seq_offset - 1].opcode == XED_ICLASS_MOV){
				seq_offset --;
			}
			else{
				if ((xed_iclass_enum_t)trace->instructions[op_to_ins[j]].opcode == XED_ICLASS_MOV && (trace_analysis_get_operand_role(trace, j) == ROLE_READ_INDEX || trace_analysis_get_operand_role(trace, j) == ROLE_READ_BASE)){
					seq_offset += trace_analysis_extract_spec_seq(trace, group, group[trace->instructions[op_to_ins[j]].operand_offset], op_to_ins, sequence + seq_offset, sequence_size - seq_offset, 0, trace_analysis_get_operand_role(trace, j));
				}
				else{
					if (parent_role == ROLE_UNKNOWN || parent_role == ROLE_READ_DIRECT){
						sequence[seq_offset].opcode = (xed_iclass_enum_t)trace->instructions[op_to_ins[j]].opcode;
						sequence[seq_offset ++].role = trace_analysis_get_operand_role(trace, j);
					}
					else{
						if (trace_analysis_get_operand_role(trace, j) == ROLE_READ_DIRECT){
							sequence[seq_offset].opcode = (xed_iclass_enum_t)trace->instructions[op_to_ins[j]].opcode;
							sequence[seq_offset ++].role = parent_role;
						}
					}
				}
			}
		}
	}

	return seq_offset;
}

static struct array* trace_analysis_extract_instruction_seq(struct trace* trace, uint32_t* group, uint32_t* op_to_ins, uint32_t nb_group){
	uint32_t 			i;
	uint8_t 			ins_offset;
	struct array* 		array;
	struct action 		ins_sequence[TRACE_ANALYSIS_GRP_MAX_SIZE];

	array = array_create(sizeof(struct action) * TRACE_ANALYSIS_GRP_MAX_SIZE);
	if (array == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		return NULL;
	}

	for (i = 0; i < nb_group; i++){
		ins_offset = trace_analysis_extract_spec_seq(trace, group, i+1, op_to_ins, ins_sequence, TRACE_ANALYSIS_GRP_MAX_SIZE - 1, 1, ROLE_UNKNOWN);
		if (ins_offset != 0){
			ins_sequence[ins_offset].opcode = XED_ICLASS_INVALID;
			if (array_add(array, &ins_sequence) < 0){
				printf("ERROR: in %s, unable to add instruction sequence to aray\n", __func__);
			}
		}
	}

	return array;
}

static void trace_analysis_print_ins_seq(struct array* array){
	uint32_t 			i;
	xed_iclass_enum_t 	j;
	struct action* 		ins_sequence;

	for (i = 0; i < array_get_length(array); i++){
		ins_sequence = (struct action*)array_get(array, i);
		j = 0;

		printf("- seq %03u: {", i);
		while(ins_sequence[j].opcode != XED_ICLASS_INVALID){
			printf("%s:%s, ", variableRole_2_string(ins_sequence[j].role), instruction_opcode_2_string(ins_sequence[j].opcode));
			j++;
		}
		printf("}\n");
	}
}

static uint32_t* trace_analyse_create_op_to_ins(struct trace* trace){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t* 	op_to_ins;

	op_to_ins = (uint32_t*)malloc(sizeof(uint32_t) * trace_get_nb_operand(trace));
	if (op_to_ins == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for (i = 0; i < trace->nb_instruction; i++){
			for (j = 0; j < trace->instructions[i].nb_operand; j++){
				op_to_ins[trace->instructions[i].operand_offset + j] = i;
			}
		}
	}

	return op_to_ins;
}

void trace_analyse_operand(struct trace* trace){
	uint32_t 			i;
	uint32_t 			j;
	struct operand* 	operands;
	uint32_t*			group;
	uint32_t 			group_id_generator = 1;
	uint32_t 			nb_operand = trace_get_nb_operand(trace);
	uint32_t* 			op_to_ins;

	group = (uint32_t*)calloc(nb_operand, sizeof(uint32_t));
	if (group == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	op_to_ins = trace_analyse_create_op_to_ins(trace);
	if (op_to_ins == NULL){
		printf("ERROR: in %s, unable to create op_to_ins\n", __func__);
		return;
	}

	for (i = 0; i < trace->nb_instruction; i++){
		operands = trace_get_ins_operands(trace, i);

		switch(trace->instructions[i].opcode){
			/* je pense qu'avec les nouvelles informations ça ne devrait pas être trop dure de reécrire ça correctement */
			case XED_ICLASS_MOV : {
				struct operand* src_op = NULL;
				struct operand* dst_op = NULL;
				uint32_t 		src_grp;
				/* group value */

				for (j = 0; j < trace->instructions[i].nb_operand; j++){
					if (OPERAND_IS_READ(operands[j])){
						if (!group[trace->instructions[i].operand_offset + j]){
							group[trace->instructions[i].operand_offset + j] = group_id_generator++;
							if (OPERAND_IS_MEM(operands[j])){
								trace_analysis_propagate_mem_group(trace->operands + trace->instructions[i].operand_offset + j, group + trace->instructions[i].operand_offset + j, nb_operand - (trace->instructions[i].operand_offset + j));
							}
							else if (OPERAND_IS_REG(operands[j])){
								trace_analysis_propagate_reg_group(trace->operands + trace->instructions[i].operand_offset + j, group + trace->instructions[i].operand_offset + j, nb_operand - (trace->instructions[i].operand_offset + j));
							}
							else{
								printf("ERROR: in %s, incorrect operand type\n", __func__);
							}
						}
						if (src_op == NULL && !OPERAND_IS_BASE(operands[j]) && !OPERAND_IS_INDEX(operands[j])){
							src_op = operands + j;
							src_grp = group[trace->instructions[i].operand_offset + j];
						}
					}
					else if (OPERAND_IS_WRITE(operands[j])){
						if (dst_op == NULL){
							dst_op = operands + j;
							if (src_op != NULL && src_op->size == dst_op->size && !memcmp(trace->data + src_op->data_offset, trace->data + dst_op->data_offset, src_op->size)){
								group[trace->instructions[i].operand_offset + j] = src_grp;
								if (OPERAND_IS_MEM(operands[j])){
									trace_analysis_propagate_mem_group(trace->operands + trace->instructions[i].operand_offset + j, group + trace->instructions[i].operand_offset + j, nb_operand - (trace->instructions[i].operand_offset + j));
								}
								else if (OPERAND_IS_REG(operands[j])){
									trace_analysis_propagate_reg_group(trace->operands + trace->instructions[i].operand_offset + j, group + trace->instructions[i].operand_offset + j, nb_operand - (trace->instructions[i].operand_offset + j));
								}
								else{
									printf("ERROR: in %s, incorrect operand type\n", __func__);
								}
							}
							else{
								printf("ERROR: in %s, incorrect instruction operands\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, too many write operand for a MOV instruction\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, incorrect operand type\n", __func__);
					}
				}

				break;
			}
			default 			: {
				for (j = 0; j < trace->instructions[i].nb_operand; j++){
					if (!group[trace->instructions[i].operand_offset + j]){
						group[trace->instructions[i].operand_offset + j] = group_id_generator++;
						if (OPERAND_IS_MEM(operands[j])){
							trace_analysis_propagate_mem_group(trace->operands + trace->instructions[i].operand_offset + j, group + trace->instructions[i].operand_offset + j, nb_operand - (trace->instructions[i].operand_offset + j));
						}
						else if (OPERAND_IS_REG(operands[j])){
							trace_analysis_propagate_reg_group(trace->operands + trace->instructions[i].operand_offset + j, group + trace->instructions[i].operand_offset + j, nb_operand - (trace->instructions[i].operand_offset + j));
						}
						else{
							printf("ERROR: in %s, incorrect operand type\n", __func__);
						}
					}
				}

				break;
			}
		}		
	}

	trace_analysis_print_group(trace, group, op_to_ins, nb_operand, group_id_generator - 1);
	{
		struct array* array;

		array = trace_analysis_extract_instruction_seq(trace, group, op_to_ins, group_id_generator - 1);
		if (array == NULL){
			printf("ERROR: in %s, unable to extract instruction sequence\n", __func__);
		}
		else{
			trace_analysis_print_ins_seq(array);
			array_delete(array);
		}

	}

	free(op_to_ins);
	free(group);
}
