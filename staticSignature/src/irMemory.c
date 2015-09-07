#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "irMemory.h"
#include "irVariableRange.h"
#include "dagPartialOrder.h"
#include "base.h"

#define IRMEMORY_ALIAS_HEURISTIC_ESP 		1
#define IRMEMORY_ALIAS_HEURISTIC_CONCRETE 	1
#define IRMEMORY_ALIAS_HEURISTIC_TOTAL 		0

static int32_t compare_order_memoryNode(const void* arg1, const void* arg2);

static void ir_normalize_print_alias_conflict(struct node* node1, struct node* node2, const char* type){
	struct node* addr1 = NULL;
	struct node* addr2 = NULL;
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
			addr1 = edge_get_src(edge_cursor);
			break;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node2); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
			addr2 = edge_get_src(edge_cursor);
			break;
		}
	}

	if (addr1 && addr2){
		printf("Aliasing conflict %s: ", type);
		ir_print_location_node(addr1, NULL);
		printf(", ");
		ir_print_location_node(addr2, NULL);
		printf("\n");
	}
	else{
		log_err("unable to get memory access address");
	}
}

static enum irOpcode ir_normalize_choose_part_opcode(uint8_t size_src, uint8_t size_dst){
	if (size_src == 32 && size_dst == 8){
		return IR_PART1_8;
	}
	else if (size_src == 32 && size_dst == 16){
		return IR_PART1_16;
	}
	else{
		log_err_m("this case is not implemented (src=%u, dst=%u)", size_src, size_dst);
		return IR_PART1_8;
	}
}

enum aliasResult{
	CANNOT_ALIAS 	= 0x00000000,
	MAY_ALIAS 		= 0x00000001,
	MUST_ALIAS 		= 0x00000003
};

#define ADDRESS_NB_MAX_DEPENDENCE 64 /* it must not exceed 0x0fffffff because the last bit of the flag is reversed for leave tagging */
#define FINGERPRINT_MAX_RECURSION_LEVEL 5

struct addrFingerprint{
	uint32_t 			nb_dependence;
	uint32_t 			flag;
	struct {
		struct node* 	node;
		uint32_t 		flag;
		uint32_t 		label;
	} 					dependence[ADDRESS_NB_MAX_DEPENDENCE];
};

static void addrFingerprint_init(struct node* node, struct addrFingerprint* addr_fgp, uint32_t recursion_level, uint32_t parent_label){
	struct irOperation* operation;
	struct edge* 		operand_cursor;
	uint32_t 			nb_dependence;

	if (recursion_level == 0){
		addr_fgp->nb_dependence = 0;
		addr_fgp->flag 			= 0;
	}
	else if (recursion_level == FINGERPRINT_MAX_RECURSION_LEVEL){
		addr_fgp->flag |= 0x0000002;
		return;
	}

	if (addr_fgp->nb_dependence < ADDRESS_NB_MAX_DEPENDENCE){
		addr_fgp->dependence[addr_fgp->nb_dependence].node = node;
		addr_fgp->dependence[addr_fgp->nb_dependence].flag = parent_label & 0x7fffffff;
		addr_fgp->dependence[addr_fgp->nb_dependence].label = addr_fgp->nb_dependence + 1;

		nb_dependence = ++ addr_fgp->nb_dependence;
	}
	else{
		log_err("the max number of dependence has been reached");
		return;
	}

	operation = ir_node_get_operation(node);

	switch(operation->type){
		case IR_OPERATION_TYPE_INST 	: {
			if (operation->operation_type.inst.opcode == IR_ADD){
				for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
					addrFingerprint_init(edge_get_src(operand_cursor), addr_fgp, recursion_level + 1, nb_dependence);
				}
			}
			break;
		}
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		case IR_OPERATION_TYPE_IN_REG 	: {
			if (operation->operation_type.in_reg.reg == IR_REG_ESP){
				addr_fgp->flag |= 0x00000001;
			}
			break;
		}
		#endif
		default 						: {
			break;
		}
	}

	if (nb_dependence == addr_fgp->nb_dependence){
		addr_fgp->dependence[nb_dependence - 1].flag |= 0x80000000;
	}

	return;
}

static void addrFingerprint_remove(struct addrFingerprint* addr_fgp, uint32_t index){
	uint32_t i;
	uint32_t label;

	label = addr_fgp->dependence[index].label;

	if (index + 1  < addr_fgp->nb_dependence){
		addr_fgp->dependence[index].node 	= addr_fgp->dependence[addr_fgp->nb_dependence - 1].node;
		addr_fgp->dependence[index].flag 	= addr_fgp->dependence[addr_fgp->nb_dependence - 1].flag;
		addr_fgp->dependence[index].label 	= addr_fgp->dependence[addr_fgp->nb_dependence - 1].label;
	}
	addr_fgp->nb_dependence --;

	for (i = 0; i < addr_fgp->nb_dependence; ){
		if ((addr_fgp->dependence[i].flag & 0x7fffffff) == label){
			addrFingerprint_remove(addr_fgp, i);
		}
		else{
			i ++;
		}
	}
}

static enum aliasResult ir_normalize_alias_analysis(struct addrFingerprint* addr1_fgp_ptr, struct node* addr2, uint32_t ir_range_seed){
	struct addrFingerprint 	addr1_fgp;
	struct addrFingerprint 	addr2_fgp;
	uint32_t 				i;
	uint32_t 				j;
	struct variableRange 	range1;
	struct variableRange 	range2;
	uint32_t 				nb_dependence_left1;
	uint32_t 				nb_dependence_left2;
	struct node* 			dependence_left1[ADDRESS_NB_MAX_DEPENDENCE];
	struct node* 			dependence_left2[ADDRESS_NB_MAX_DEPENDENCE];

	memcpy(&addr1_fgp, addr1_fgp_ptr, sizeof(struct addrFingerprint));
	addrFingerprint_init(addr2, &addr2_fgp, 0, 0);

	for (i = 0; i < addr1_fgp.nb_dependence; ){
		for (j = 0; j < addr2_fgp.nb_dependence; j++){
			if (addr1_fgp.dependence[i].node == addr2_fgp.dependence[j].node){
				addrFingerprint_remove(&addr1_fgp, i);
				addrFingerprint_remove(&addr2_fgp, j);

				goto next;
			}
		}
		i ++;
		next:;
	}

	for (i = 0, nb_dependence_left1 = 0; i < addr1_fgp.nb_dependence; i++){
		if (addr1_fgp.dependence[i].flag & 0x80000000){
			dependence_left1[nb_dependence_left1 ++] = addr1_fgp.dependence[i].node;
		}
	}

	for (j = 0, nb_dependence_left2 = 0; j < addr2_fgp.nb_dependence; j++){
		if (addr2_fgp.dependence[j].flag & 0x80000000){
			dependence_left2[nb_dependence_left2 ++] = addr2_fgp.dependence[j].node;
		}
	}

	irVariableRange_get_range_add_buffer(&range1, dependence_left1, nb_dependence_left1, 32, ir_range_seed);
	irVariableRange_get_range_add_buffer(&range2, dependence_left2, nb_dependence_left2, 32, ir_range_seed);

	if (variableRange_is_cst(&range1) && variableRange_is_cst(&range2)){
		if (variableRange_get_cst(&range1) == variableRange_get_cst(&range2)){
			return MUST_ALIAS;
		}
		else{
			return CANNOT_ALIAS;
		}
	}
	else if (variableRange_intersect(&range1, &range2)){
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		if (addr1_fgp.flag & 0x00000002 || addr2_fgp.flag & 0x00000002){
			return MAY_ALIAS;
		}
		else if ((addr1_fgp.flag & 0x00000001) == (addr2_fgp.flag & 0x00000001)){
			return MAY_ALIAS;
		}
		else{
			return CANNOT_ALIAS;
		}
		#else
		return MAY_ALIAS;
		#endif
	}

	return CANNOT_ALIAS;
}

struct node* ir_normalize_search_alias_conflict(struct node* node1, struct node* node2, enum aliasType alias_type, enum aliasingStrategy strategy, uint32_t ir_range_seed){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct edge* 			edge_cursor;
	struct node* 			addr1 = NULL;
	struct node* 			addr_cursor;
	struct addrFingerprint 	addr1_fgp;

	node_cursor = ir_node_get_operation(node1)->operation_type.mem.access.next;
	if (node_cursor == NULL){
		log_err("found NULL pointer instead of the expected element");
		return NULL;
	}

	while(node_cursor != node2){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (alias_type == ALIAS_ALL || (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM && alias_type == ALIAS_IN) || (operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM && alias_type == ALIAS_OUT)){
			if (strategy == ALIASING_STRATEGY_STRICT){
				return node_cursor;
			}
			else{
				#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE == 1
				if (ir_node_get_operation(node1)->operation_type.mem.access.con_addr != MEMADDRESS_INVALID && ir_node_get_operation(node_cursor)->operation_type.mem.access.con_addr != MEMADDRESS_INVALID){
					if (ir_node_get_operation(node1)->operation_type.mem.access.con_addr == ir_node_get_operation(node_cursor)->operation_type.mem.access.con_addr){
						return node_cursor;
					}
					#if IRMEMORY_ALIAS_HEURISTIC_TOTAL == 1
					else{
						goto next;
					}
					#endif
				}
				#endif

				if (addr1 == NULL){
					for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
							addr1 = edge_get_src(edge_cursor);
							addrFingerprint_init(addr1, &addr1_fgp, 0, 0);
							break;
						}
					}
				}

				for (edge_cursor = node_get_head_edge_dst(node_cursor), addr_cursor = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
						addr_cursor = edge_get_src(edge_cursor);
						break;
					}
				}

				if (addr1 != NULL && addr_cursor != NULL){
					if (ir_normalize_alias_analysis(&addr1_fgp, addr_cursor, ir_range_seed) != CANNOT_ALIAS){
						return node_cursor;
					}
				}
				else{
					log_err("memory access with no address operand");
				}
			}
		}

		#if IRMEMORY_ALIAS_HEURISTIC_TOTAL == 1
		next:
		#endif
		node_cursor = operation_cursor->operation_type.mem.access.next;
		if (node_cursor == NULL){
			log_err("found NULL pointer instead of the expected element");
			break;
		}
	}

	return NULL;
}

/*
	Return value description:
	< 0 : error
	0   : node2 overwrites node1
	> 0 : node1 overwrites node2
*/

static int32_t irMemory_simplify_WR(struct ir* ir, struct node* node1, struct node* node2);
static int32_t irMemory_simplify_RR(struct ir* ir, struct node* node1, struct node* node2);
static int32_t irMemory_simplify_WW(struct ir* ir, struct node* node1, struct node* node2);

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification, enum aliasingStrategy strategy){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	uint32_t 				nb_mem_access;
	struct node** 			access_list 			= NULL;
	uint32_t 				access_list_alloc_size;
	uint32_t 				i;
	struct node*			alias;
	int32_t 				return_code;

	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_src > 1){
			for (edge_cursor = node_get_head_edge_src(node_cursor), nb_mem_access = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
					nb_mem_access ++;
				}
			}
			if (nb_mem_access > 1){
				if (access_list == NULL || access_list_alloc_size < nb_mem_access * sizeof(struct node*)){
					if (access_list != NULL){
						free(access_list);
					}
					access_list = (struct node**)malloc(sizeof(struct node*) * nb_mem_access);
					access_list_alloc_size = sizeof(struct node*) * nb_mem_access;
					if (access_list == NULL){
						log_err("unable to allocate memory");
						continue;
					}
				}

				for (edge_cursor = node_get_head_edge_src(node_cursor), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
						access_list[i++] = edge_get_dst(edge_cursor);
					}
				}

				qsort(access_list, nb_mem_access, sizeof(struct node*), compare_order_memoryNode);

				for (i = 1; i < nb_mem_access; i++){
					struct irOperation* operation_prev;
					struct irOperation* operation_next;

					operation_prev = ir_node_get_operation(access_list[i - 1]);
					operation_next = ir_node_get_operation(access_list[i]);

					#ifdef IR_FULL_CHECK
					if (operation_prev->operation_type.mem.access.con_addr != MEMADDRESS_INVALID && operation_next->operation_type.mem.access.con_addr != MEMADDRESS_INVALID){
						if (operation_prev->operation_type.mem.access.con_addr != operation_next->operation_type.mem.access.con_addr){
							log_err_m("memory operations has the same address operand but different concrete addresses: 0x%08x - 0x%08x", operation_prev->operation_type.mem.access.con_addr, operation_next->operation_type.mem.access.con_addr);

							operation_prev->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							operation_next->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;

							continue;
						}
					}
					#endif

					/* STORE -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){

						switch(strategy){
							case ALIASING_STRATEGY_WEAK 	: {
								break;
							}
							case ALIASING_STRATEGY_STRICT 	:
							case ALIASING_STRATEGY_CHECK 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, strategy, ir->range_seed);
								if (alias){
									continue;
								}
								break;
							}
							case ALIASING_STRATEGY_PRINT 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, strategy, ir->range_seed);
								if (alias){
									ir_normalize_print_alias_conflict(access_list[i - 1], alias, "STORE -> LOAD ");
								}
								else{
									log_warn("possible memory simplification, but strategy is print");
								}
								continue;
							}
						}

						return_code = irMemory_simplify_WR(ir, access_list[i - 1], access_list[i]);
						goto decode_return_code;
					}

					/* LOAD -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_IN_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){

						switch(strategy){
							case ALIASING_STRATEGY_WEAK 	: {
								break;
							}
							case ALIASING_STRATEGY_STRICT 	:
							case ALIASING_STRATEGY_CHECK 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, strategy, ir->range_seed);
								if (alias){
									continue;
								}
								break;
							}
							case ALIASING_STRATEGY_PRINT 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, strategy, ir->range_seed);
								if (alias){
									ir_normalize_print_alias_conflict(access_list[i - 1], alias, "LOAD  -> LOAD ");
								}
								else{
									log_warn("possible memory simplification, but strategy is print");
								}
								continue;
							}
						}

						return_code = irMemory_simplify_RR(ir, access_list[i - 1], access_list[i]);
						goto decode_return_code;
					}

					/* STORE -> STORE */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_OUT_MEM){

						switch(strategy){
							case ALIASING_STRATEGY_WEAK 	: {
								break;
							}
							case ALIASING_STRATEGY_STRICT 	:
							case ALIASING_STRATEGY_CHECK 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_IN, strategy, ir->range_seed);
								if (alias){
									continue;
								}
								break;
							}
							case ALIASING_STRATEGY_PRINT 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_IN, strategy, ir->range_seed);
								if (alias){
									ir_normalize_print_alias_conflict(access_list[i - 1], alias, "STORE -> STORE");
								}
								else{
									log_warn("possible memory simplification, but strategy is print");
								}
								continue;
							}
						}

						return_code = irMemory_simplify_WW(ir, access_list[i - 1], access_list[i]);
						goto decode_return_code;
					}

					continue;

					decode_return_code:
					switch(return_code){
						case 0  : {
							*modification = 1;
							break;
						}
						case 1  : {
							access_list[i] = access_list[i - 1];
							*modification = 1;
							break;
						}
						default : {
							log_err_m("simplification failed, return code: %d", return_code);
							break;
						}
					}
				}
			}
		}
	}

	if (access_list != NULL){
		free(access_list);
	}
}

static int32_t irMemory_simplify_WR(struct ir* ir, struct node* node1, struct node* node2){
	struct irOperation* operation1;
	struct irOperation* operation2;
	struct edge*		edge_cursor;
	struct node* 		new_inst;
	int32_t 			result = -1;

	operation1 = ir_node_get_operation(node1);
	operation2 = ir_node_get_operation(node2);

	for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_ADDRESS){
			if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_MACRO && operation1->size > operation2->size){
				if ((new_inst = ir_add_inst(ir, operation2->index, operation2->size, ir_normalize_choose_part_opcode(operation1->size, operation2->size), IR_OPERATION_DST_UNKOWN)) == NULL){
					log_err("unable to add instruction to IR");
					continue;
				}

				if (ir_add_dependence(ir, edge_get_src(edge_cursor), new_inst, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add new dependency to IR");
				}

				graph_transfert_src_edge(&(ir->graph), new_inst, node2);
				ir_remove_node(ir, node2);

				ir_drop_range(ir);

				result = 1;
			}
			else if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_MACRO && operation1->size < operation2->size){
				log_warn("simplification of memory access of different size (case STORE -> LOAD)");
			}
			else{
				graph_transfert_src_edge(&(ir->graph), edge_get_src(edge_cursor), node2);
				ir_remove_node(ir, node2);

				ir_drop_range(ir);

				result = 1;
			}
		}
	}

	return result;
}

static int32_t irMemory_simplify_RR(struct ir* ir, struct node* node1, struct node* node2){
	struct irOperation* operation1;
	struct irOperation* operation2;
	struct edge*		edge_cursor;

	operation1 = ir_node_get_operation(node1);
	operation2 = ir_node_get_operation(node2);

	if (operation1->size > operation2->size){
		ir_convert_node_to_inst(node2, operation2->index, operation2->size, ir_normalize_choose_part_opcode(operation1->size, operation2->size));

		for (edge_cursor = node_get_head_edge_dst(node2); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
				ir_remove_dependence(ir, edge_cursor);
				break;
			}
		}

		if (ir_add_dependence(ir, node1, node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
			log_err("unable to add new dependency to IR");
		}

		return 1;
	}

	if (operation1->size < operation2->size){
		ir_convert_node_to_inst(node1, operation1->index, operation1->size, ir_normalize_choose_part_opcode(operation2->size, operation1->size));

		for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
				ir_remove_dependence(ir, edge_cursor);
				break;
			}
		}

		if (ir_add_dependence(ir, node2, node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
			log_err("unable to add new dependency to IR");
		}

		return 0;
	}

	graph_transfert_src_edge(&(ir->graph), node1, node2);
	ir_remove_node(ir, node2);

	return 1;
}

static int32_t irMemory_simplify_WW(struct ir* ir, struct node* node1, struct node* node2){
	struct irOperation* operation1;
	struct irOperation* operation2;

	operation1 = ir_node_get_operation(node1);
	operation2 = ir_node_get_operation(node2);

	if (operation1->size > operation2->size){
		log_warn_m("simplification of memory access of different size (case STORE (%u bits) -> STORE (%u bits))", operation1->size, operation2->size);
		return -1;
	}

	ir_remove_node(ir, node1);

	return 0;
}

struct memAccessToken{
	struct node* 	node;
	ADDRESS 		address;
};

static int32_t memAccessToken_is_alive(struct memAccessToken* token, struct node* current_node){
	struct node* node_cursor;

	for (node_cursor = ir_node_get_operation(current_node)->operation_type.mem.access.prev; node_cursor != NULL; node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.access.prev){
		if (node_cursor == token->node){
			return 1;
		}
	}

	return 0;
}

static int32_t compare_address_memToken(const void* arg1, const void* arg2);

void ir_simplify_concrete_memory_access(struct ir* ir){
	struct node* 			node_cursor;
	struct node* 			next_node_cursor;
	struct irOperation* 	operation_cursor;
	struct irOperation* 	operation_prev;
	void* 					binary_tree_root = NULL;
	struct memAccessToken* 	new_token;
	struct memAccessToken** existing_token;
	int32_t 				return_code;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM || operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
			break;
		}
	}

	if (node_cursor == NULL){
		return;
	}

	while (operation_cursor->operation_type.mem.access.prev != NULL){
		node_cursor = operation_cursor->operation_type.mem.access.prev;
		operation_cursor = ir_node_get_operation(node_cursor);
	}

	for ( ; node_cursor != NULL; node_cursor = next_node_cursor){
		operation_cursor = ir_node_get_operation(node_cursor);
		next_node_cursor = operation_cursor->operation_type.mem.access.next;

		if (operation_cursor->operation_type.mem.access.con_addr == MEMADDRESS_INVALID){
			log_warn("a memory access has an incorrect concrete address, further simplications might be incorrect");
			continue;
		}

		if ((new_token = (struct memAccessToken*)malloc(sizeof(struct memAccessToken))) == NULL){
			log_err("unable to allocate memory");
			continue;
		}

		new_token->node 	= node_cursor;
		new_token->address 	= operation_cursor->operation_type.mem.access.con_addr;

		existing_token = (struct memAccessToken**)tsearch((void*)new_token, &binary_tree_root, compare_address_memToken);
		if (existing_token == NULL){
			log_err("tsearch failed");
			free(new_token);
		}
		else if (*existing_token != new_token){
			if (memAccessToken_is_alive(*existing_token, node_cursor)){
				operation_prev = ir_node_get_operation((*existing_token)->node);
				if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_cursor->type == IR_OPERATION_TYPE_IN_MEM){
					return_code = irMemory_simplify_WR(ir, (*existing_token)->node, node_cursor);
				}
				else if(operation_prev->type == IR_OPERATION_TYPE_IN_MEM && operation_cursor->type == IR_OPERATION_TYPE_IN_MEM){
					return_code = irMemory_simplify_RR(ir, (*existing_token)->node, node_cursor);
				}
				else if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
					return_code = irMemory_simplify_WW(ir, (*existing_token)->node, node_cursor);
				}
				else{
					return_code = 0;
				}

				switch(return_code){
					case 0  : {
						memcpy(*existing_token, new_token, sizeof(struct memAccessToken));
						break;
					}
					case 1  : {
						break;
					}
					default : {
						log_err_m("simplification failed, return code: %d", return_code);
						break;
					}
				}
			}
			else{
				memcpy(*existing_token, new_token, sizeof(struct memAccessToken));
			}

			free(new_token);
		}
	}

	tdestroy(binary_tree_root, free);
}


/* ===================================================================== */
/* Sorting routines						                                 */
/* ===================================================================== */

static int32_t compare_order_memoryNode(const void* arg1, const void* arg2){
	struct node* access1 = *(struct node**)arg1;
	struct node* access2 = *(struct node**)arg2;
	struct irOperation* op1 = ir_node_get_operation(access1);
	struct irOperation* op2 = ir_node_get_operation(access2);

	if (op1->operation_type.mem.access.order < op2->operation_type.mem.access.order){
		return -1;
	}
	else if (op1->operation_type.mem.access.order > op2->operation_type.mem.access.order){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t compare_address_memToken(const void* arg1, const void* arg2){
	struct memAccessToken* token1 = (struct memAccessToken*)arg1;
	struct memAccessToken* token2 = (struct memAccessToken*)arg2;

	if (token1->address < token2->address){
		return -1;
	}
	else if (token1->address > token2->address){
		return 1;
	}
	else{
		return 0;
	}
}
