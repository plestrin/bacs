#ifndef IRSATURATE_H
#define IRSATURATE_H

#include <stdint.h>

#include "array.h"
#include "ir.h"
#include "codeSignature.h"

#define ASSOSIG_MAX_INPUT 	5
#define ASSOSIG_MAX_OUTPUT	2

#define irSaturate_opcode_is_associative(opcode) (((opcode) == IR_ADD) || ((opcode) == IR_XOR))

struct assoSig{
	uint32_t 		id;
	enum irOpcode 	opcode;
	uint32_t 		nb_input;
	uint32_t 		buffer_input[ASSOSIG_MAX_INPUT];
	uint32_t 		nb_output;
	uint32_t 		buffer_output[ASSOSIG_MAX_OUTPUT];
	uint32_t 		nb_node;
	uint32_t 		nb_reschedule_pending_in;
	uint32_t 		buffer_reschedule_pending_in[ASSOSIG_MAX_INPUT];
};

struct saturateRules{
	struct array 	asso_sig_array;
};

#define saturateRules_init(saturate_rules) 										\
	array_init(&((saturate_rules)->asso_sig_array), sizeof(struct assoSig))

void saturateRules_learn_associative_conflict(struct saturateRules* saturate_rules, struct codeSignatureCollection* collection);
void saturateRules_print(struct saturateRules* saturate_rules);

void irSaturate_saturate(struct saturateRules* saturate_rules, struct ir* ir);

#define saturateRules_reset(saturate_rules) 									\
	array_empty(&((saturate_rules)->asso_sig_array));

#define saturateRules_clean(saturate_rules) 									\
	array_clean(&((saturate_rules)->asso_sig_array));

#endif