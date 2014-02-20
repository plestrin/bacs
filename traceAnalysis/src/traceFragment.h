#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#include <stdint.h>
#include <string.h>

#include "codeMap.h"
#include "instruction.h"
#include "memAccess.h"
#include "regAccess.h"
#include "array.h"

#define TRACEFRAGMENT_TAG_LENGTH 32

enum fragmentType{
	TRACEFRAGMENT_TYPE_NONE,
	TRACEFRAGMENT_TYPE_LOOP
};

struct fragmentCallback{
	void(*specific_delete)(void*);
	void*(*specific_clone)(void*);
};

struct traceFragment{
	char 						tag[TRACEFRAGMENT_TAG_LENGTH];
	struct array 				instruction_array;
	enum fragmentType 			type;
	void* 						specific_data;
	struct fragmentCallback*  	callback;
	

	struct memAccess* 			read_memory_array;
	struct memAccess* 			write_memory_array;
	uint32_t 					nb_memory_read_access;
	uint32_t 					nb_memory_write_access;

	struct regAccess* 			read_register_array;
	struct regAccess* 			write_register_array;
	uint32_t 					nb_register_read_access;
	uint32_t 					nb_register_write_access;
};

struct traceFragment* codeFragment_create(enum fragmentType type, void* specific_data, struct fragmentCallback* callback);
int32_t traceFragment_init(struct traceFragment* frag, enum fragmentType type, void* specific_data, struct fragmentCallback* callback);

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
void traceFragment_remove_read_after_write(struct traceFragment* frag);

int32_t traceFragment_create_reg_array(struct traceFragment* frag);

void traceFragment_print_location(struct traceFragment* frag, struct codeMap* cm);

void traceFragment_delete(struct traceFragment* frag);
void traceFragment_clean(struct traceFragment* frag);

#endif