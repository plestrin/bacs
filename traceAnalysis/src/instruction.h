#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

#include "xed-iclass-enum.h"

#define INSTRUCTION_MAX_NB_DATA 1


/* Data type value:
 * - bit 1: set to 0 -> INVALID 	set to 1 VALID
 * - bit 2: set to 0 -> MEM 		set to 1 REG
 * - bit 3: set to 0 -> READ 		set to 1 WRITE
 */

#define INSTRUCTION_DATA_TYPE_SET_INVALID(type) (type) &= 0xfffffffe;
#define INSTRUCTION_DATA_TYPE_SET_VALID(type) 	(type) |= 0x00000001;
#define INSTRUCTION_DATA_TYPE_SET_MEM(type)		(type) &= 0xfffffffd;
#define INSTRUCTION_DATA_TYPE_SET_REG(type)		(type) |= 0x00000002;
#define INSTRUCTION_DATA_TYPE_SET_READ(type)	(type) &= 0xfffffffb;
#define INSTRCUTION_DATA_TYPE_SET_WRITE(type)	(type) |= 0x00000004;

#define INSTRUCTION_DATA_TYPE_IS_INVALID(type) 	(((type) & 0x00000001) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_VALID(type) 	(((type) & 0x00000001) == 0x00000001)
#define INSTRUCTION_DATA_TYPE_IS_MEM(type) 		(((type) & 0x00000002) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_REG(type)		(((type) & 0x00000002) == 0x00000002)
#define INSTRUCTION_DATA_TYPE_IS_READ(type)		(((type) & 0x00000004) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_WRITE(type) 	(((type) & 0x00000004) == 0x00000004)



enum insDataType{
	INSDATA_INVALID 	= 0x00000000,
	INSDATA_MEM_READ	= 0x00000001,
	INSDATA_REG_READ 	= 0x00000003,
	INSDATA_MEM_WRITE	= 0x00000005,
	INSDATA_REG_WRITE	= 0x00000007
};

struct insData{
	enum insDataType 	type;
	union {
		unsigned long 	address;
		/* some stuff for registers */
	}					location;
	uint32_t 			value;
	uint8_t 			size;
};

/* statuer sur la taille des address mémoire */
struct instruction{
	unsigned long 	pc;
	unsigned long 	pc_next; /* je ne sais pas si c'est encore justifié avec les opcodes ?? Avoir lorsque l'on va reprendre le control flow graph */
	uint32_t 		opcode;
	struct insData 	data[INSTRUCTION_MAX_NB_DATA];
};

void instruction_print(struct instruction *ins);


static inline int instruction_compare_pc(struct instruction* ins1, struct instruction* ins2){
	return (ins1->pc == ins2->pc);
}

#endif