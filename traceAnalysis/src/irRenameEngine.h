#ifndef IRRENAMEENGINE_H
#define IRRENAMEENGINE_H

#define _GNU_SOURCE

#include <search.h>

#include "instruction.h"
#include "graph.h"
#include "ir.h"

enum aliasType{
	ALIAS_MEM_WRITE = 0x00000000,
	ALIAS_MEM_READ 	= 0x00000001,
	ALIAS_REG_WRITE = 0x00000002,
	ALIAS_REG_READ 	= 0x00000003
};

#define ALIAS_WRITE 	0x00000000
#define ALIAS_READ 		0x00000001
#define ALIAS_MEM 		0x00000000
#define ALIAS_REG 		0x00000002

#define ALIAS_IS_WRITE(type) 	(((type) & 0x00000001) == ALIAS_WRITE)
#define ALIAS_IS_READ(type) 	(((type) & 0x00000001) == ALIAS_READ)
#define ALIAS_IS_MEM(type) 		(((type) & 0x00000002) == ALIAS_MEM)
#define ALIAS_IS_REG(type) 		(((type) & 0x00000002) == ALIAS_REG)

struct aliasMem{
	struct node* 			ir_node;
	ADDRESS 				address;
	uint8_t 				size;
	struct irRenameEngine* 	engine;
	struct alias* 			next;
	struct alias* 			prev;
};

struct aliasReg{
	struct node* 			ir_node;
};

struct alias{
	enum aliasType 			type;
	union{
		struct aliasMem 	mem;
		struct aliasReg 	reg;
	} 						alias_type;
};

struct irRenameEngine{
	void* 					memory_bintree;
	struct alias 			register_table[NB_REGISTER];
	struct ir* 				ir;
};

#define irRenameEngine_init(engine, ir_) 													\
	(engine).memory_bintree = NULL; 														\
	memset(&((engine).register_table), 0, sizeof(struct alias)*NB_REGISTER); 				\
	(engine).ir = (ir_);

void irRenameEngine_free_memory_node(void* alias);

struct node* irRenameEngine_get_ref(struct irRenameEngine* engine, struct operand* operand);
struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum reg reg, struct operand* operand);

int32_t irRenameEngine_set_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node);
int32_t irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum reg reg, struct node* node);

int32_t irRenameEngine_set_new_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node);
int32_t irRenameEngine_set_register_new_ref(struct irRenameEngine* engine, enum reg reg, struct node* node);

#define irRenameEngine_clean(engine) 														\
	tdestroy((engine).memory_bintree, irRenameEngine_free_memory_node);

#endif