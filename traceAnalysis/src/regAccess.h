#ifndef REGACCESS_H
#define REGACCESS_H

#include <stdint.h>

#include "argSet.h"
#include "instruction.h"
#include "array.h"

#define REGACCESS_MAX_NB_BRUTE_FORCE 6

struct regAccess{
	uint32_t 	value;
	enum reg 	reg;
	uint8_t 	size;
	uint32_t 	order;
};

void regAccess_print(struct regAccess* reg_access, int nb_reg_access);

void regAccess_propagate_read(struct regAccess* reg_access, int nb_reg_access);
void regAccess_propagate_write(struct regAccess* reg_access, int nb_reg_access);

int32_t regAccess_extract_arg_large_pure_read(struct argSet* set, struct regAccess* reg_access, int nb_reg_access);

int32_t regAccess_extract_arg_large_mix_read(struct argSet* set, struct regAccess* reg_access, int nb_reg_access);

int32_t regAccess_extract_arg_large_write(struct argSet* set, struct regAccess* reg_access, int nb_reg_access);

#endif