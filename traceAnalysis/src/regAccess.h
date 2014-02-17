#ifndef REGACCESS_H
#define REGACCESS_H

#include <stdint.h>

#include "instruction.h"
#include "array.h"

struct regAccess{
	uint32_t 	value;
	enum reg 	reg;
	uint8_t 	size;
};

void regAccess_print(struct regAccess* reg_access, int nb_reg_access);

int32_t regAccess_extract_arg_large_pure_read(struct array* input_arg, struct regAccess* reg_access, int nb_reg_access);

int32_t regAccess_extract_arg_large_mix_read(struct array* input_arg, struct regAccess* reg_access, int nb_reg_access);

int32_t regAccess_extract_arg_large_write(struct array* output_arg, struct regAccess* reg_access, int nb_reg_access);

#endif