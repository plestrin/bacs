#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irCheck.h"

void ir_check_size(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irOperation* 	operand;
	struct irDependence* 	dependence;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		switch(operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				if (node_cursor->nb_edge_dst > 1){
					operand = ir_node_get_operation(edge_get_src(node_get_head_edge_dst(node_cursor)));
					if (operand->size != 32){
						printf("ERROR: in %s, memory load address is %u bits large\n", __func__, operand->size);
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					operand = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence = ir_edge_get_dependence(edge_cursor);

					switch(dependence->type){
						case IR_DEPENDENCE_TYPE_ADDRESS : {
							if (operand->size != 32){
								printf("ERROR: in %s, memory store address is %u bits large\n", __func__, operand->size);
							}
							break;
						}
						case IR_DEPENDENCE_TYPE_DIRECT 	: {
							if (operation_cursor->size != operand->size){
								printf("ERROR: in %s, incorrect operand size for memory store: (%u -> %u)\n", __func__, operand->size, operation_cursor->size);	
							}
							break;
						}
						default 						: {
							break;
						}
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					operand = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence = ir_edge_get_dependence(edge_cursor);

					switch(operation_cursor->operation_type.inst.opcode){
						case IR_MOVZX 		: {
							if (operation_cursor->size <= operand->size){
								printf("ERROR: in %s, found size mismatch (%u -> MOVZX:%u)\n", __func__, operand->size, operation_cursor->size);
							}
							break;
						}
						case IR_PART1_8 	: {
							if (operation_cursor->size != 8 || operand->size % 8 != 0 || operand->size <= 8){
								printf("ERROR: in %s, found size mismatch (%u -> PART1_8:%u)\n", __func__, operand->size, operation_cursor->size);
							}
							break;
						}
						case IR_PART2_8 	: {
							if (operation_cursor->size != 8 || operand->size % 8 != 0 || operand->size < 16){
								printf("ERROR: in %s, found size mismatch (%u -> PART2_8:%u)\n", __func__, operand->size, operation_cursor->size);
							}
							break;
						}
						case IR_PART1_16 	: {
							if (operation_cursor->size != 16 || operand->size % 16 != 0 || operand->size <= 16){
								printf("ERROR: in %s, found size mismatch (%u -> PART1_16:%u)\n", __func__, operand->size, operation_cursor->size);
							}
							break;
						}
						case IR_ROL 		:
						case IR_ROR 		:
						case IR_SHL 		:
						case IR_SHR 		:{
							if (dependence->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
								if (operand->size != 8){
									printf("ERROR: in %s, incorrect operand size for displacement: %u\n", __func__, operand->size);
								}
							}
							else{
								if (operation_cursor->size != operand->size){
									printf("ERROR: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
								}
							}
							break;
						}
						default 		: {
							if (operation_cursor->size != operand->size){
								printf("ERROR: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
							}
							break;
						}
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				break;
			}
		}
	}
}

static const uint32_t min_dst_edge[NB_IR_OPCODE] = {
	2, 	/* IR_ADD 						*/
	2, 	/* IR_AND 						*/
	2, 	/* IR_CMOV 						*/
	2, 	/* IR_DIV 						*/
	2, 	/* IR_IMUL 						*/
	0, 	/* IR_LEA 	It doesn't matter 	*/
	0, 	/* IR_MOV 	It doesn't matter 	*/
	1, 	/* IR_MOVZX 					*/
	2, 	/* IR_MUL 						*/
	1, 	/* IR_NOT 						*/
	2, 	/* IR_OR 						*/
	1, 	/* IR_PART1_8 					*/
	1, 	/* IR_PART2_8 					*/
	1, 	/* IR_PART1_16 					*/
	2, 	/* IR_ROL 						*/
	2, 	/* IR_ROR 						*/
	2, 	/* IR_SHL 						*/
	2, 	/* IR_SHR 						*/
	2, 	/* IR_SUB 						*/
	2, 	/* IR_XOR 						*/
	0, 	/* IR_LOAD 	It doesn't matter 	*/
	0, 	/* IR_STORE It doesn't matter 	*/
	0, 	/* IR_JOKER It doesn't matter 	*/
	0, 	/* IR_INVALID It doesn't matter */
};

static const uint32_t max_dst_edge[NB_IR_OPCODE] = {
	0xffffffff, /* IR_ADD 						*/
	0xffffffff, /* IR_AND 						*/
	0x00000002, /* IR_CMOV 						*/
	0x00000002, /* IR_DIV 						*/
	0xffffffff, /* IR_IMUL 						*/
	0x00000000, /* IR_LEA 	It doesn't matter 	*/
	0x00000000, /* IR_MOV 	It doesn't matter 	*/
	0x00000001, /* IR_MOVZX 					*/
	0xffffffff, /* IR_MUL 						*/
	0x00000001, /* IR_NOT 						*/
	0xffffffff, /* IR_OR 						*/
	0x00000001, /* IR_PART1_8 					*/
	0x00000001, /* IR_PART2_8 					*/
	0x00000001, /* IR_PART1_16 					*/
	0x00000002, /* IR_ROL 						*/
	0x00000002, /* IR_ROR 						*/
	0x00000002, /* IR_SHL 						*/
	0x00000002, /* IR_SHR 						*/
	0x00000002, /* IR_SUB 						*/
	0xffffffff, /* IR_XOR 						*/
	0x00000000, /* IR_LOAD 	It doesn't matter 	*/
	0x00000000, /* IR_STORE It doesn't matter 	*/
	0x00000000, /* IR_JOKER It doesn't matter 	*/
	0x00000000, /* IR_INVALID It doesn't matter */
};

void ir_check_connectivity(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irDependence* 	dependence;
	uint32_t 				nb_dependence[NB_DEPENDENCE_TYPE];
	uint32_t 				i;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		switch (operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst){
					printf("ERROR: in %s, input register %s has %u dst edge(s)\n", __func__, irRegister_2_string(operation_cursor->operation_type.in_reg.reg), node_cursor->nb_edge_dst);
				}

				/* Check output edge(s) */
				if (!node_cursor->nb_edge_src){
					printf("ERROR: in %s, input register has no src edge\n", __func__);
				}
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst == 1){
					dependence = ir_edge_get_dependence(node_get_head_edge_dst(node_cursor));

					if (dependence->type != IR_DEPENDENCE_TYPE_ADDRESS){
						printf("ERROR: in %s, memory load has an incorrect type of dependence\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, memory load has %u dst edge(s)\n", __func__, node_cursor->nb_edge_dst);
				}

				/* Check output edge(s) */
				if (node_cursor->nb_edge_src == 0 && (operation_cursor->size & IR_NODE_STATUS_FLAG_FINAL) != 0){
					printf("ERROR: in %s, memory load has no src edge but neither is flagged as final\n", __func__);
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst == 2){
					for (edge_cursor = node_get_head_edge_dst(node_cursor), memset(nb_dependence, 0, sizeof(uint32_t) * NB_DEPENDENCE_TYPE); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						dependence = ir_edge_get_dependence(edge_cursor);
						nb_dependence[dependence->type] ++;

						if (dependence->type != IR_DEPENDENCE_TYPE_ADDRESS && dependence->type != IR_DEPENDENCE_TYPE_DIRECT){
							printf("ERROR: in %s, memory store has an incorrect type of dependence\n", __func__);
						}
					}
					if (nb_dependence[IR_DEPENDENCE_TYPE_ADDRESS] != 1){
						printf("ERROR: in %s, memory store has an incorrect number of address dependence (%u)\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_ADDRESS]);
					}
					if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
						printf("ERROR: in %s, memory store has an incorrect number of direct dependence (%u)\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
					}
				}
				else{
					printf("ERROR: in %s, memory store has %u dst edge(s)\n", __func__, node_cursor->nb_edge_dst);
				}

				/* Check output edge(s) */
				if (node_cursor->nb_edge_src){
					printf("ERROR: in %s, memory store has %u src edge(s)\n", __func__, node_cursor->nb_edge_src);
				}
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				/* Check input edge(s) */
				if (node_cursor->nb_edge_dst){
					printf("ERROR: in %s, immediate has %u dst edge(s)\n", __func__, node_cursor->nb_edge_dst);
				}

				/* Check output edge(s) */
				if (!node_cursor->nb_edge_src){
					printf("ERROR: in %s, immediate node is orphan\n", __func__);
				}
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				if (node_cursor->nb_edge_dst < min_dst_edge[operation_cursor->operation_type.inst.opcode] || node_cursor->nb_edge_dst > max_dst_edge[operation_cursor->operation_type.inst.opcode]){
					printf("ERROR: in %s, inst %s has an incorrect number of dst edge: %u (min=%u, max=%u)\n", __func__, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), node_cursor->nb_edge_dst, min_dst_edge[operation_cursor->operation_type.inst.opcode], max_dst_edge[operation_cursor->operation_type.inst.opcode]);
				}
				if (!node_cursor->nb_edge_dst && (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL) == 0){
					printf("ERROR: in %s, inst %s has no src edge but neither is flagged as final\n", __func__, irOpcode_2_string(operation_cursor->operation_type.inst.opcode));
				}

				for (edge_cursor = node_get_head_edge_dst(node_cursor), memset(nb_dependence, 0, sizeof(uint32_t) * NB_DEPENDENCE_TYPE); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					dependence = ir_edge_get_dependence(edge_cursor);
					nb_dependence[dependence->type] ++;
				}

				switch(operation_cursor->operation_type.inst.opcode){
					case IR_ADD 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst ADD\n", __func__, i);
							}
						}
						break;
					}
					case IR_AND 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst AND\n", __func__, i);
							}
						}
						break;
					}
					case IR_CMOV 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst CMOV\n", __func__, i);
							}
						}
						break;
					}
					case IR_DIV 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst DIV\n", __func__, i);
							}
						}
						break;
					}
					case IR_IMUL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst IMUL\n", __func__, i);
							}
						}
						break;
					}
					case IR_LEA 		: {
						break;
					}
					case IR_MOV 		: {
						break;
					}
					case IR_MOVZX 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst MOVZX\n", __func__, i);
							}
						}
						break;
					}
					case IR_MUL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst MUL\n", __func__, i);
							}
						}
						break;
					}
					case IR_NOT 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst NOT\n", __func__, i);
							}
						}
						break;
					}
					case IR_OR 			: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst OR\n", __func__, i);
							}
						}
						break;
					}
					case IR_PART1_8 	: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst PART1_8\n", __func__, i);
							}
						}
						break;
					}
					case IR_PART2_8 	: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst PART2_8\n", __func__, i);
							}
						}
						break;
					}
					case IR_PART1_16 	: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst PART1_16\n", __func__, i);
							}
						}
						break;
					}
					case IR_ROL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst ROL\n", __func__, i);
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type DIRECT: %u for inst ROL\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type SHIFT_DISP: %u for inst ROL\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						break;
					}
					case IR_ROR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst ROR\n", __func__, i);
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type DIRECT: %u for inst ROR\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type SHIFT_DISP: %u for inst ROR\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						break;
					}

					case IR_SHL 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst SHL\n", __func__, i);
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type DIRECT: %u for inst SHL\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type SHIFT_DISP: %u for inst SHL\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						break;
					}

					case IR_SHR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && (enum irDependenceType)i != IR_DEPENDENCE_TYPE_SHIFT_DISP && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst SHR\n", __func__, i);
							}
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_DIRECT] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type DIRECT: %u for inst SHR\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						if (nb_dependence[IR_DEPENDENCE_TYPE_SHIFT_DISP] != 1){
							printf("ERROR: in %s, incorrect number of dependence of type SHIFT_DISP: %u for inst SHR\n", __func__, nb_dependence[IR_DEPENDENCE_TYPE_DIRECT]);
						}
						break;
					}

					case IR_SUB 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst PART1_16\n", __func__, i);
							}
						}
						break;
					}
					case IR_XOR 		: {
						for (i = 0; i < NB_DEPENDENCE_TYPE; i++){
							if ((enum irDependenceType)i != IR_DEPENDENCE_TYPE_DIRECT && nb_dependence[i] > 0){
								printf("ERROR: in %s, incorrect dependence type %u for inst PART1_16\n", __func__, i);
							}
						}
						break;
					}
					case IR_LOAD 		: {
						break;
					}
					case IR_STORE 		: {
						break;
					}
					case IR_JOKER 		: {
						break;
					}
					case IR_INVALID 	: {
						break;
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				break;
			}
		}
	}
}