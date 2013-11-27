#ifndef PRIMITIVEREFERENCE_H
#define PRIMITIVEREFERENCE_H

#include <stdint.h>

#include "argBuffer.h"

#define PRIMITIVEREFERENCE_MAX_NAME_SIZE 256
#define PRIMITIVEREFERENCE_UNDEFINED_LENGTH -1

/* Arg specifier value:
 * - bit [1..2]		: set to 0 -> SIZE_EXACT_VALUE 	|set to 1 -> SIZE_MULTIPLE_OF 	|set to 2 -> SIZE_UNDEFINED 	|set to 3 -> ARG_IS_IMPLICIT_SIZE_OF
 * - bit [3..32] 	: size value 					|size factor 					|undefined 						|argument index
 */

#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(s, v)	(s) = ((v) << 2) | 0x00000000
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(s, v)	(s) = ((v) << 2) | 0x00000001
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(s)		(s) = 0x00000002
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(s, v)	(s) = ((v) << 2) | 0x00000003

#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(s)		(((s) & 0x00000003) == 0x00000000)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MULTIPLE(s)		(((s) & 0x00000003) == 0x00000001)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_UNDEFINED(s)		(((s) & 0x00000003) == 0x00000002)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_IMPLICIT_SIZE(s)		(((s) & 0x00000003) == 0x00000003)

#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(s) 	((s) >> 2)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MULTIPLE(s) 		((s) >> 2)
#define PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_IMPLICIT_SIZE(s) 		((s) >> 2)

struct primitiveReference{
	char 		name[PRIMITIVEREFERENCE_MAX_NAME_SIZE];
	uint8_t 	nb_input;
	uint8_t 	nb_output;
	uint32_t* 	input_specifier;
	uint32_t* 	output_specifier;
	void(*func)(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
};

int32_t primitiveReference_init(struct primitiveReference* primitive, char* name, uint8_t nb_input, uint8_t nb_output, uint32_t* input_specifier, uint32_t* output_specifier, void(*func)(void** input, uint8_t nb_input, void** output, uint8_t nb_output));
int32_t primitiveReference_test(struct primitiveReference* primitive, uint8_t nb_input, uint8_t nb_output, struct argBuffer* input, struct argBuffer* output);
void primitiveReference_print(struct primitiveReference* primitive);
void primitiveReference_clean(struct primitiveReference* primitive);

#endif