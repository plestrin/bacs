#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#include <stdint.h>
#include <string.h>

#include "instruction.h"
#include "array.h"

#define TRACEFRAGMENT_TAG_LENGTH 32

struct memAccess{
	uint32_t 	order;
	uint32_t 	value;
	ADDRESS 	address;
	uint8_t 	size;
	uint32_t 	opcode;
};

struct regAccess{
	uint32_t 	value;
	enum reg 	reg;
	uint8_t 	size;
};

struct traceFragment{
	char 				tag[TRACEFRAGMENT_TAG_LENGTH];
	struct array 		instruction_array;
	struct memAccess* 	read_memory_array;
	struct memAccess* 	write_memory_array;
	uint32_t 			nb_memory_read_access;
	uint32_t 			nb_memory_write_access;
	struct regAccess* 	read_register_array;
	struct regAccess* 	write_register_array;
	uint32_t 			nb_register_read_access;
	uint32_t 			nb_register_write_access;
};

struct traceFragment* codeFragment_create();
int32_t traceFragment_init(struct traceFragment* frag);

static inline void traceFragment_set_tag(struct traceFragment* frag, char* tag){
	strncpy(frag->tag, tag, TRACEFRAGMENT_TAG_LENGTH);
}

static inline int32_t traceFragment_add_instruction(struct traceFragment* frag, struct instruction* ins){
	return array_add(&(frag->instruction_array), ins);
}

static inline int32_t traceFragment_search_pc(struct traceFragment* frag, struct instruction* ins){
	return array_search_seq_up(&(frag->instruction_array), 0, array_get_length(&(frag->instruction_array)), ins, (int32_t(*)(void*,void*))instruction_compare_pc);
}

static inline uint32_t traceFragment_get_nb_instruction(struct traceFragment* frag){
	return array_get_length(&(frag->instruction_array));
}

static inline struct instruction* traceFragment_get_instruction(struct traceFragment* frag, uint32_t index){
	return (struct instruction*)array_get(&(frag->instruction_array), index);
}

struct instruction* traceFragment_get_last_instruction(struct traceFragment* frag);
int32_t traceFragment_clone(struct traceFragment* frag_src, struct traceFragment* frag_dst);
double traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode);

int32_t traceFragment_create_mem_array(struct traceFragment* frag);
void traceFragment_print_mem_array(struct memAccess* mem_access, int nb_mem_access);
void traceFragment_remove_read_after_write(struct traceFragment* frag);

int32_t traceFragment_create_reg_array(struct traceFragment* frag);
void traceFragment_print_reg_array(struct regAccess* reg_access, int nb_reg_access);

int32_t traceFragment_extract_mem_arg_adjacent_read(struct array* array, struct memAccess* mem_access, int nb_mem_access);
int32_t traceFragment_extract_mem_arg_adjacent_write(struct array* array, struct memAccess* mem_access, int nb_mem_access);
int32_t traceFragment_extract_mem_arg_adjacent_size_read(struct array* array, struct memAccess* mem_access, int nb_mem_access);
int32_t traceFragment_extract_mem_arg_adjacent_size_opcode_read(struct array* array, struct memAccess* mem_access, int nb_mem_access);

int32_t traceFragment_extract_reg_arg_large_pure(struct array* array, struct regAccess* reg_access, int nb_reg_access);

void traceFragment_delete(struct traceFragment* frag);
void traceFragment_clean(struct traceFragment* frag);

#endif