#ifndef PRIMITIVEREFERENCE_H
#define PRIMITIVEREFERENCE_H

#include <stdint.h>

#include "argBuffer.h"
#include "fastOutputSearch.h"

#define PRIMITIVEREFERENCE_MAX_NAME_SIZE 			256

/* Arg specifier value:
 * 				bit [1..3]									|				bit [4..32]
 * 		set to 0 -> SIZE_EXACT_VALUE 			(BOTH)		|			size value
 * 		set to 1 -> SIZE_MULTIPLE_OF 			(INPUT)		|			size factor
 * 		set to 2 -> SIZE_UNDEFINED 				(INPUT)		|			undefined
 * 		set to 3 -> ARG_IS_IMPLICIT_SIZE_OF 	(INPUT)		|			input argument index
 * 		set to 4 -> SIZE_OF_INPUT_ARG 			(OUTPUT)	| 			input argument index
 * 		set to 5 -> SIZE_MAX 					(INPUT) 	| 			max size value
 */

#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(s, v)		(s) = ((v) << 3) | 0x00000000
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(s, v)		(s) = ((v) << 3) | 0x00000001
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(s)			(s) = 0x00000002
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(s, v)		(s) = ((v) << 3) | 0x00000003
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(s, v) 	(s) = ((v) << 3) | 0x00000004
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MAX(s, v) 			(s) = ((v) << 3) | 0x00000005

#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(s)			(((s) & 0x00000007) == 0x00000000)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MULTIPLE(s)			(((s) & 0x00000007) == 0x00000001)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_UNDEFINED(s)			(((s) & 0x00000007) == 0x00000002)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_IMPLICIT_SIZE(s)			(((s) & 0x00000007) == 0x00000003)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_OF_INPUT_ARG(s) 		(((s) & 0x00000007) == 0x00000004)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MAX(s) 				(((s) & 0x00000007) == 0x00000005)

#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(s) 		((s) >> 3)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MULTIPLE(s) 			((s) >> 3)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(s) 			((s) >> 3)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MAX(s) 				((s) >> 3)

struct primitiveReference{
	char 		name[PRIMITIVEREFERENCE_MAX_NAME_SIZE];
	uint8_t 	nb_input;
	uint8_t 	nb_output;
	uint32_t* 	input_specifier;
	uint32_t* 	output_specifier;
	void* 		lib_handle;
	void(*func)(void** input, void** output);
};

int32_t primitiveReference_init(struct primitiveReference* primitive, char* name, uint8_t nb_input, uint8_t nb_output, uint32_t* input_specifier, uint32_t* output_specifier, char* lib_name, char* func_name);

int32_t primitiveReference_test(struct primitiveReference* primitive, uint8_t nb_input, struct argBuffer* input, struct array* output_args, struct fastOutputSearch* accelerator);

void primitiveReference_snprint_inputs(struct primitiveReference* primitive, char* buffer, uint32_t buffer_length);
void primitiveReference_snprint_outputs(struct primitiveReference* primitive, char* buffer, uint32_t buffer_length);

uint8_t primitiveReference_get_nb_explicit_input(struct primitiveReference* primitive);
uint8_t primitiveReference_get_nb_explicit_output(struct primitiveReference* primitive);

void primitiveReference_clean(struct primitiveReference* primitive);

#endif