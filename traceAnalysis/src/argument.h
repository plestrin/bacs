#ifndef ARGUMENT_H
#define ARGUMENT_H

#include <stdint.h>

#include "instruction.h"
#include "array.h"
#include "multiColumn.h"

#define ARGUMENT_STRING_DESC_LENGTH		40
#define ARGUMENT_ACCESS_SIZE_UNDEFINED 	-1

enum argFragType{
	ARGFRAG_MEM,
	ARGFRAG_REG
};

struct argFragDesc{
	enum argFragType 	type;
	union{
		ADDRESS 		address;
		enum reg 		reg;
	} 					location;
	uint8_t  			size;
};

struct inputStub{
	struct array* 		array;
	union{
		int32_t 		index;
		void* 			pointer;
	} 					id;
};

struct inputArgument{
	void* 				dyn_mem;
	struct argFragDesc* desc;
	uint8_t 			nb_desc;
	uint32_t 			size;
	int8_t 				access_size;
	char* 				data;
	struct inputStub* 	stub;
};

int32_t inputArgument_init(struct inputArgument* arg, uint32_t size, uint8_t nb_desc, int8_t access_size);
void inputArgument_set_operand(struct inputArgument* arg, uint8_t index, uint32_t offset, struct operand* operand, char* data);
uint32_t inputArgument_print_desc(char* string, uint32_t string_length, struct inputArgument* arg);
void inputArgument_print(struct inputArgument* arg, struct multiColumnPrinter* printer);
int32_t inputArgument_search(struct inputArgument* arg, char* buffer, uint32_t buffer_size);

int32_t inputArgument_is_mem(struct inputArgument* arg);
int32_t inputArgument_is_reg(struct inputArgument* arg);

int32_t inputArgument_compare(struct inputStub* stub1, struct inputStub* stub2);

#define inputArgument_clean(arg) 	free((arg)->dyn_mem)

#define OUTPUTARGUMENT_MAX_SIZE 32

struct outputArgument{
	struct argFragDesc 	desc;
	char 				data[OUTPUTARGUMENT_MAX_SIZE];
};

int32_t outputArgument_init(struct outputArgument* arg, struct operand* operand, char* data);
uint32_t outputArgument_print_desc(char* string, uint32_t string_length, struct outputArgument* arg);
void outputArgument_print(struct outputArgument* arg, struct multiColumnPrinter* printer);

#define OUTPUTARGUMENT_IS_MEM(arg) 	((arg)->desc.type == ARGFRAG_MEM)
#define OUTPUTARGUMENT_IS_REG(arg) 	((arg)->desc.type == ARGFRAG_REG)

#endif