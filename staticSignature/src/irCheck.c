#include <stdlib.h>
#include <stdio.h>

#include "irCheck.h"

void ir_check(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irOperation* 	operand;
	struct irDependence* 	dependence;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		switch(operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				if (node_cursor->nb_edge_dst){
					printf("ERROR: in %s, input register %s has %u dst edge(s)\n", __func__, irRegister_2_string(operation_cursor->operation_type.in_reg.reg), node_cursor->nb_edge_dst);
				}
				if (!node_cursor->nb_edge_src){
					printf("WARNING: in %s, input register node is orphan\n", __func__);
				}
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				if (node_cursor->nb_edge_dst == 1){
					edge_cursor = node_get_head_edge_dst(node_cursor);
					operand = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence = ir_edge_get_dependence(edge_cursor);

					if (dependence->type == IR_DEPENDENCE_TYPE_ADDRESS){
						if (operand->size != 32){
							printf("WARNING: in %s, memory load has an operand of size: %u bits\n", __func__, operand->size);
						}
					}
					else{
						printf("ERROR: in %s, memory load's operand has an incorrect type\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, memory load has %u dst edge(s)\n", __func__, node_cursor->nb_edge_dst);
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				if (node_cursor->nb_edge_dst == 2){
					for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						operand = ir_node_get_operation(edge_get_src(edge_cursor));
						dependence = ir_edge_get_dependence(edge_cursor);

						switch(dependence->type){
							case IR_DEPENDENCE_TYPE_ADDRESS 	: {
								if (operand->size != 32){
									printf("WARNING: in %s, memory store address is %u bits large\n", __func__, operand->size);
								}
								break;
							}
							case IR_DEPENDENCE_TYPE_DIRECT 		: {
								if (operation_cursor->size != operand->size){
									printf("WARNING: in %s, incorrect operand size for memory store: (%u -> %u)\n", __func__, operand->size, operation_cursor->size);	
								}
								break;
							}
							default 							: {
								printf("ERROR: in %s, memory store's operand has an incorrect type\n", __func__);
								break;
							}
						}
					}
				}
				else{
					printf("ERROR: in %s, memory store has %u dst edge(s)\n", __func__, node_cursor->nb_edge_dst);
				}
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				if (node_cursor->nb_edge_dst){
					printf("ERROR: in %s, immediate has %u dst edge(s)\n", __func__, node_cursor->nb_edge_dst);
				}
				if (!node_cursor->nb_edge_src){
					printf("ERROR: in %s, immediate node is orphan\n", __func__);
				}
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				if (!node_cursor->nb_edge_dst){
					printf("ERROR: in %s, inst %s has no input operand\n", __func__, irRegister_2_string(operation_cursor->operation_type.in_reg.reg));
				}
				if (!node_cursor->nb_edge_dst && (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL) == 0){
					printf("WARNING: in %s, inst %s has no child but is not flagged as final\n", __func__, irRegister_2_string(operation_cursor->operation_type.in_reg.reg));
				}
				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					operand = ir_node_get_operation(edge_get_src(edge_cursor));
					dependence = ir_edge_get_dependence(edge_cursor);

					switch(dependence->type){
						case IR_DEPENDENCE_TYPE_ADDRESS 	: {
							printf("ERROR: in %s, get address dependence to a node that is not a memory access\n", __func__);
							break;
						}
						case IR_DEPENDENCE_TYPE_DIRECT 		: {
							switch(operand->type){
								case IR_OPERATION_TYPE_IN_REG 	: 
								case IR_OPERATION_TYPE_IN_MEM 	: 
								case IR_OPERATION_TYPE_IMM 		: 
								case IR_OPERATION_TYPE_INST 	: {
									switch(operation_cursor->operation_type.inst.opcode){
										case IR_MOVZX 	: {
											if (operation_cursor->size <= operand->size){
												printf("WARNING: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
											}
											break;
										}
										case IR_PART1_8 : {
											if (operation_cursor->size != 8 || operand->size % 8 != 0 || operand->size <= 8){
												printf("WARNING: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
											}
											break;
										}
										case IR_PART2_8 : {
											if (operation_cursor->size != 8 || operand->size % 8 != 0 || operand->size < 16){
												printf("WARNING: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
											}
											break;
										}
										case IR_ROL 	:
										case IR_ROR 	:
										case IR_SHL 	:
										case IR_SHR 	: {
											if (dependence->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
												if (operand->size != 8){
													printf("WARNING: in %s, incorrect operand size for displacement: %u\n", __func__, operand->size);
												}
											}
											else{
												if (operation_cursor->size != operand->size){
													printf("WARNING: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
												}
											}
											break;
										}
										default 		: {
											if (operation_cursor->size != operand->size){
												printf("WARNING: in %s, found size mismatch (%u -> %s:%u)\n", __func__, operand->size, irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
											}
											break;
										}
									}
									break;
								}
								case IR_OPERATION_TYPE_OUT_MEM 	: {
									printf("ERROR: in %s, memory load is used as an operand for another operation\n", __func__);
									break;
								}
								case IR_OPERATION_TYPE_SYMBOL 	: {
									printf("ERROR: in %s, symbol node has an incorrect dependence type\n", __func__);
									break;
								}
							}
							break;
						}
						case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
							switch(operand->type){
								case IR_OPERATION_TYPE_IN_REG 	: 
								case IR_OPERATION_TYPE_IN_MEM 	: 
								case IR_OPERATION_TYPE_IMM 		: 
								case IR_OPERATION_TYPE_INST 	: {
									switch(operation_cursor->operation_type.inst.opcode){
										case IR_ROL 	:
										case IR_ROR 	:
										case IR_SHL 	:
										case IR_SHR 	: {
											if (operand->size != 8){
												printf("WARNING: in %s, incorrect operand size for displacement: %u\n", __func__, operand->size);
											}
											break;
										}
										default 		: {
											printf("ERROR: in %s, incorrect operation (%s), for dependence of type SHIFT_DISP\n", __func__, irOpcode_2_string(operation_cursor->operation_type.inst.opcode));
											break;
										}
									}
									break;
								}
								case IR_OPERATION_TYPE_OUT_MEM 	: {
									printf("ERROR: in %s, memory load is used as an operand for another operation\n", __func__);
									break;
								}
								case IR_OPERATION_TYPE_SYMBOL 	: {
									printf("ERROR: in %s, symbol node has an incorrect dependence type\n", __func__);
									break;
								}
							}
							break;
						}
						case IR_DEPENDENCE_TYPE_MACRO 		: {
							break;
						}
					}
				}
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				break;
			}
		}
	}
}