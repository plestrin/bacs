#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "irMemory.h"
#include "irVariableRange.h"
#include "irRenameEngine.h"
#include "dagPartialOrder.h"
#include "base.h"

#define IRMEMORY_ALIAS_HEURISTIC_ESP 		1
#define IRMEMORY_ALIAS_HEURISTIC_CONCRETE 	1
#define IRMEMORY_ALIAS_HEURISTIC_TOTAL 		1

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
		putchar('\n');
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

struct node* ir_normalize_search_alias_conflict(struct node* node1, struct node* node2, enum aliasType alias_type, uint32_t ir_range_seed){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct edge* 			edge_cursor;
	struct node* 			addr1 = NULL;
	struct node* 			addr_cursor;
	struct addrFingerprint 	addr1_fgp;

	node_cursor = ir_node_get_operation(node1)->operation_type.mem.next;
	if (node_cursor == NULL){
		log_err("found NULL pointer instead of the expected element");
		return NULL;
	}

	while(node_cursor != node2){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (alias_type == ALIAS_ALL || (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM && alias_type == ALIAS_IN) || (operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM && alias_type == ALIAS_OUT)){
			#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE == 1
			if (ir_node_get_operation(node1)->operation_type.mem.con_addr != MEMADDRESS_INVALID && ir_node_get_operation(node_cursor)->operation_type.mem.con_addr != MEMADDRESS_INVALID){
				if (ir_node_get_operation(node1)->operation_type.mem.con_addr == ir_node_get_operation(node_cursor)->operation_type.mem.con_addr){
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

		#if IRMEMORY_ALIAS_HEURISTIC_TOTAL == 1
		next:
		#endif
		node_cursor = operation_cursor->operation_type.mem.next;
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

void ir_normalize_simplify_memory_access_(struct ir* ir, uint8_t* modification, enum aliasingStrategy strategy){
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
					if (operation_prev->operation_type.mem.con_addr != MEMADDRESS_INVALID && operation_next->operation_type.mem.con_addr != MEMADDRESS_INVALID){
						if (operation_prev->operation_type.mem.con_addr != operation_next->operation_type.mem.con_addr){
							log_err_m("memory operations has the same address operand but different concrete addresses: 0x%08x - 0x%08x", operation_prev->operation_type.mem.con_addr, operation_next->operation_type.mem.con_addr);
							printf("  - %p ", (void*)access_list[i - 1]); ir_print_node(operation_prev, stdout); putchar('\n');
							printf("  - %p ", (void*)access_list[i - 0]); ir_print_node(operation_next, stdout); putchar('\n');

							operation_prev->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;
							operation_next->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR;

							continue;
						}
					}
					#endif

					/* STORE -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){

						switch(strategy){
							case ALIASING_STRATEGY_CHECK 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, ir->range_seed);
								if (alias){
									continue;
								}
								break;
							}
							case ALIASING_STRATEGY_PRINT 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, ir->range_seed);
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
							case ALIASING_STRATEGY_CHECK 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, ir->range_seed);
								if (alias){
									continue;
								}
								break;
							}
							case ALIASING_STRATEGY_PRINT 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_OUT, ir->range_seed);
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
							case ALIASING_STRATEGY_CHECK 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_IN, ir->range_seed);
								if (alias){
									continue;
								}
								break;
							}
							case ALIASING_STRATEGY_PRINT 	: {
								alias = ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], ALIAS_IN, ir->range_seed);
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

				if (graph_copy_src_edge(&(ir->graph), new_inst, node2)){
					log_err("unable to copy edge(s)");
				}
				if (operation2->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
					irRenameEngine_change_node(ir->alias_buffer, node2, new_inst);
				}

				result = 1;
			}
			else if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_MACRO && operation1->size < operation2->size){
				log_warn_m("simplification of memory access of different size (case STORE:%u -> LOAD:%u)", operation1->size, operation2->size);
			}
			else{
				if (graph_copy_src_edge(&(ir->graph), edge_get_src(edge_cursor), node2)){
					log_err("unable to copy edge(s)");
				}
				if (operation2->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
					irRenameEngine_change_node(ir->alias_buffer, node2, edge_get_src(edge_cursor));
				}

				result = 1;
			}
		}
	}

	if (result == 1){
		ir_remove_node(ir, node2);
		ir_drop_range(ir);
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

	ir_merge_equivalent_node(ir, node1, node2);

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

	ir_merge_equivalent_node(ir, node2, node1);

	return 0;
}

struct memAccessToken{
	struct node* 	node;
	ADDRESS 		address;
	uint32_t 		offset;
	uint32_t 		size;
};

static int32_t memAccessToken_is_alive(struct memAccessToken* token, struct node* current_node){
	struct node* node_cursor;

	for (node_cursor = ir_node_get_operation(current_node)->operation_type.mem.prev; node_cursor != NULL; node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.prev){
		if (node_cursor == token->node){
			return 1;
		}
	}

	return 0;
}

static int32_t compare_address_memToken(const void* arg1, const void* arg2);

#define CONCRETE_MEMORY_ACCESS_MAX_SIZE 		32
#define CONCRETE_MEMORY_ACCESS_MAX_NB_FRAGMENT  (CONCRETE_MEMORY_ACCESS_MAX_SIZE / 8)

struct memAccessFragment{
	struct memAccessToken* 	token;
	uint32_t 				offset;
	uint32_t 				size;
};

static int32_t ir_memory_concrete_push_full_access(void** binary_tree_root, struct node* node){
	struct memAccessToken* 	new_token;
	struct memAccessToken** result;

	if ((new_token = (struct memAccessToken*)malloc(sizeof(struct memAccessToken))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	new_token->node 	= node;
	new_token->address 	= ir_node_get_operation(node)->operation_type.mem.con_addr;
	new_token->offset 	= 0;
	new_token->size 	= ir_node_get_operation(node)->size / 8;

	result = (struct memAccessToken**)tsearch((void*)new_token, binary_tree_root, compare_address_memToken);
	if (result == NULL){
		log_err("unable to insert token in the binary tree");
		free(new_token);
		return -1;
	}
	else if (*result != new_token){
		log_err("a token is already available at this address");
		free(new_token);
		return -1;
	}

	return 0;
}

static struct node* ir_memory_concrete_push_frag_access(struct ir* ir, void** binary_tree_root, struct node* node, uint32_t offset, uint32_t size){
	struct node* addr;
	struct node* access;

	if (node->nb_edge_dst == 0){
		log_err("memory access has not dst edge");
		return NULL;
	}

	addr = edge_get_src(node_get_head_edge_dst(node));

	if (offset){
		struct node* off_val;
		struct node* add;

		off_val = ir_add_immediate(ir, 32, offset);
		add = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_ADD, IR_OPERATION_DST_UNKOWN);

		if (off_val != NULL && add != NULL){
			if (ir_add_dependence(ir, off_val, add, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add new dependency to IR");
			}
			if (ir_add_dependence(ir, addr, add, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add new dependency to IR");
			}
			addr = add;
		}
		else{
			log_err("unable to add nodes to IR");
		}
	}

	access = ir_add_in_mem_(ir, ir_node_get_operation(node)->index, size*8, addr, node, ir_node_get_operation(node)->operation_type.mem.con_addr + offset);
	if (access == NULL){
		log_err("unable to add memory access to IR");
		return NULL;
	}

	if (ir_memory_concrete_push_full_access(binary_tree_root, access)){
		log_err("unable to push full memory access");
	}

	return access;
}

static int32_t ir_memory_concrete_shift_full_access(void** binary_tree_root, struct memAccessToken* token, uint32_t disp){
	struct memAccessToken** result;

	if (tdelete(token, binary_tree_root, compare_address_memToken) == NULL){
		log_err("unable to delete token from the binary tree");
		return -1;
	}

	token->address 	+= disp;
	token->offset 	+= disp;
	token->size 	-= disp;

	result = (struct memAccessToken**)tsearch((void*)token, binary_tree_root, compare_address_memToken);
	if (result == NULL){
		log_err("unable to insert token in the binary tree");
		free(token);
		return -1;
	}
	else if (*result != token){
		log_err("a token is already available at this address");
		free(token);
		return -1;
	}

	return 0;
}

static int32_t ir_memory_concrete_split_full_access(void** binary_tree_root, struct memAccessToken* token, uint32_t l_bound, uint32_t h_bound){
	struct memAccessToken* 	new_token;
	struct memAccessToken** result;

	token->size = l_bound;

	if ((new_token = (struct memAccessToken*)malloc(sizeof(struct memAccessToken))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	new_token->node 	= token->node;
	new_token->address 	= token->address + h_bound;
	new_token->offset 	= h_bound;
	new_token->size 	= token->size - h_bound;

	result = (struct memAccessToken**)tsearch((void*)new_token, binary_tree_root, compare_address_memToken);
	if (result == NULL){
		log_err("unable to insert token in the binary tree");
		free(new_token);
		return -1;
	}
	else if (*result != new_token){
		log_err("a token is already available at this address");
		free(new_token);
		return -1;
	}

	return 0;

}

static struct node* ir_memory_concrete_get_access_value(struct ir* ir, struct memAccessFragment* mem_access_fragment){
	struct node* 		raw_value 	= NULL;
	struct edge* 		edge_cursor;

	if (ir_node_get_operation(mem_access_fragment->token->node)->type == IR_OPERATION_TYPE_IN_MEM){
		raw_value = mem_access_fragment->token->node;
	}
	else if (ir_node_get_operation(mem_access_fragment->token->node)->type == IR_OPERATION_TYPE_OUT_MEM){
		for (edge_cursor = node_get_head_edge_dst(mem_access_fragment->token->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
				break;
			}
		}
		if (edge_cursor == NULL){
			log_err("unable to get data operand of a memory store");
			return NULL;
		}
		else{
			raw_value = edge_get_src(edge_cursor);
		}
	}
	else{
		log_err("incorrect operation type");
		return NULL;
	}

	if (mem_access_fragment->offset + mem_access_fragment->token->offset){
		struct node* disp_value;
		struct node* shift;

		disp_value = ir_add_immediate(ir, ir_node_get_operation(raw_value)->size, (mem_access_fragment->offset + mem_access_fragment->token->offset) * 8);
		shift = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(raw_value)->size, IR_SHR, IR_OPERATION_DST_UNKOWN);

		if (disp_value != NULL && shift != NULL){
			if (ir_add_dependence(ir, disp_value, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
				log_err("unable to add new dependency to IR");
			}
			if (ir_add_dependence(ir, raw_value, shift, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add new dependency to IR");
			}
			raw_value = shift;
		}
		else{
			log_err("unable to add nodes to IR");
		}
	}

	if (mem_access_fragment->size != ir_node_get_operation(raw_value)->size / 8){
		struct node* part = NULL;

		if (mem_access_fragment->size == 1 && ir_node_get_operation(raw_value)->size == 32){
			part = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
		}
		else if (mem_access_fragment->size == 1 && ir_node_get_operation(raw_value)->size == 16){
			part = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
		}
		else if (mem_access_fragment->size == 2 && ir_node_get_operation(raw_value)->size == 32){
			part = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 16, IR_PART1_16, IR_OPERATION_DST_UNKOWN);
		}
		else{
			log_err_m("incorrect size selection %u byte(s) from %u", mem_access_fragment->size, ir_node_get_operation(raw_value)->size / 8);
		}

		if (part != NULL){
			if (ir_add_dependence(ir, raw_value, part, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add new dependency to IR");
			}
			raw_value = part;
		}
		else{
			log_err("unable to add instruction to IR");
		}
	}

	return raw_value;
}

static int32_t ir_memory_concrete_build_compound_value(struct ir* ir, struct node** current_value, struct node* new_value, uint32_t offset, uint32_t size){
	struct node* or;
	struct node* movzx;
	struct node* disp_value;
	struct node* shift;

	if (new_value == NULL){
		log_err("new value is NULL");
		return -1;
	}

	if (*current_value == NULL){
		*current_value = new_value;
	}
	else{
		if (ir_node_get_operation(*current_value)->size < size){
			movzx = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
			if (movzx != NULL){
				if (ir_add_dependence(ir, *current_value, movzx, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add new dependency to IR");
				}
				*current_value = movzx;
			}
			else{
				log_err("unable to add instruction to IR");
			}
		}

		if (ir_node_get_operation(new_value)->size < size){
			movzx = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
			if (movzx != NULL){
				if (ir_add_dependence(ir, new_value, movzx, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add new dependency to IR");
				}
				new_value = movzx;
			}
			else{
				log_err("unable to add instruction to IR");
			}
		}

		if (offset){
			disp_value = ir_add_immediate(ir, size, offset * 8);
			shift = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_SHL, IR_OPERATION_DST_UNKOWN);

			if (disp_value != NULL && shift != NULL){
				if (ir_add_dependence(ir, disp_value, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
					log_err("unable to add new dependency to IR");
				}
				if (ir_add_dependence(ir, new_value, shift, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add new dependency to IR");
				}
				new_value = shift;
			}
			else{
				log_err("unable to add nodes to IR");
			}
		}

		or = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_OR, IR_OPERATION_DST_UNKOWN);
		if (or != NULL){
			if (ir_add_dependence(ir, *current_value, or, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add new dependency to IR");
			}
			if (ir_add_dependence(ir, new_value, or, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add new dependency to IR");
			}
			*current_value = or;
		}
		else{
			log_err("unable to add instruction to IR");
			return -1;
		}
	}

	return 0;
}

void ir_simplify_concrete_memory_access(struct ir* ir, uint8_t* modification){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	struct node* 				node_cursor;
	struct node* 				next_node_cursor;
	struct irOperation* 		operation_cursor;
	void* 						binary_tree_root = NULL;
	struct memAccessToken 		fetch_token;
	struct memAccessToken** 	existing_token;
	struct memAccessFragment 	access_fragment[CONCRETE_MEMORY_ACCESS_MAX_NB_FRAGMENT];
	uint32_t 					nb_mem_access_fragment;
	uint8_t 					taken_fragment[CONCRETE_MEMORY_ACCESS_MAX_NB_FRAGMENT];

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM || operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
			break;
		}
	}

	if (node_cursor == NULL){
		return;
	}

	while (operation_cursor->operation_type.mem.prev != NULL){
		node_cursor = operation_cursor->operation_type.mem.prev;
		operation_cursor = ir_node_get_operation(node_cursor);
	}

	for ( ; node_cursor != NULL; node_cursor = next_node_cursor){
		operation_cursor = ir_node_get_operation(node_cursor);
		next_node_cursor = operation_cursor->operation_type.mem.next;

		if (operation_cursor->operation_type.mem.con_addr == MEMADDRESS_INVALID){
			log_warn("a memory access has an incorrect concrete address, further simplifications might be incorrect");
			continue;
		}

		if (operation_cursor->size > CONCRETE_MEMORY_ACCESS_MAX_SIZE){
			log_warn_m("memory access size (%u) exceeds max limit (%u), further simplification might be incorrect", operation_cursor->size, CONCRETE_MEMORY_ACCESS_MAX_SIZE);
			continue;
		}

		fetch_token.node 	= NULL;
		fetch_token.address 	= operation_cursor->operation_type.mem.con_addr;

		memset(taken_fragment, 0, sizeof(uint8_t) * CONCRETE_MEMORY_ACCESS_MAX_NB_FRAGMENT);

		for (i = 0, j = 0, nb_mem_access_fragment = 0; i < operation_cursor->size / 8; ){
			int32_t is_alive = 1;

			existing_token = (struct memAccessToken**)tfind((void*)&fetch_token, &binary_tree_root, compare_address_memToken);
			if (existing_token != NULL && (is_alive = memAccessToken_is_alive(*existing_token, node_cursor)) && (*existing_token)->size > j){
				access_fragment[nb_mem_access_fragment].token 	= *existing_token;
				access_fragment[nb_mem_access_fragment].offset 	= j;
				access_fragment[nb_mem_access_fragment].size 	= min((*existing_token)->size - j, (operation_cursor->size / 8) - i);

				memset(taken_fragment + i, 1, access_fragment[nb_mem_access_fragment].size);

				i += access_fragment[nb_mem_access_fragment].size;
				fetch_token.address += access_fragment[nb_mem_access_fragment].size + j;
				nb_mem_access_fragment ++;
				j = 0;
			}
			else{
				if (!is_alive){
					if (tdelete(*existing_token, &binary_tree_root, compare_address_memToken) == NULL){
						log_err("unable to delete token from the binary tree");
					}
					free(*existing_token);
				}
				if (i == 0){
					if (j + 1 == CONCRETE_MEMORY_ACCESS_MAX_NB_FRAGMENT){
						i ++;
						fetch_token.address += j + 1;
						j = 0;
					}
					else{
						j ++;
						fetch_token.address --;
					}
				}
				else{
					i ++;
					fetch_token.address ++;
				}
			}
		}

		if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM){
			struct node* value = NULL;

			for (i = 0, j = 0, k = 0; i < operation_cursor->size / 8; ){
				if (taken_fragment[i]){
					if (k){
						if (ir_memory_concrete_build_compound_value(ir, &value, ir_memory_concrete_push_frag_access(ir, &binary_tree_root, node_cursor, i - k, k), i - k, operation_cursor->size)){
							log_err_m("unable to get memory access fragment of size %u at offset %u", access_fragment[j].offset, access_fragment[j].offset);
						}
						k = 0;
					}
					if (ir_memory_concrete_build_compound_value(ir, &value, ir_memory_concrete_get_access_value(ir, access_fragment + j), i, operation_cursor->size)){
						log_err_m("unable to get memory access fragment of size %u at offset %u", access_fragment[j].offset, access_fragment[j].offset);
					}
					
					i += access_fragment[j].size;
					j ++;
				}
				else{
					k ++;
					i ++;
				}
			}
			if (k){
				if (k == i){
					ir_memory_concrete_push_full_access(&binary_tree_root, node_cursor);
				}
				else{
					if (ir_memory_concrete_build_compound_value(ir, &value, ir_memory_concrete_push_frag_access(ir, &binary_tree_root, node_cursor, i - k, k), i - k, operation_cursor->size)){
						log_err_m("unable to get memory access fragment of size %u at offset %u", access_fragment[j].offset, access_fragment[j].offset);
					}
				}
			}
			if (value != NULL){
				ir_merge_equivalent_node(ir, value, node_cursor);
				*modification = 1;
			}
		}
		else{
			for (i = 0; i < nb_mem_access_fragment; i++){
				if (access_fragment[i].offset == 0 && access_fragment[i].size == access_fragment[i].token->size){
					if (tdelete(access_fragment[i].token, &binary_tree_root, compare_address_memToken) == NULL){
						log_err("unable to delete token from the binary tree");
					}
					free(access_fragment[i].token);
				}
				else if (access_fragment[i].offset != 0 && access_fragment[i].size + access_fragment[i].offset == access_fragment[i].token->size){
					access_fragment[i].token->size = access_fragment[i].offset;
				}
				else if (access_fragment[i].offset == 0 && access_fragment[i].size < access_fragment[i].token->size){
					if (ir_memory_concrete_shift_full_access(&binary_tree_root, access_fragment[i].token, access_fragment[i].size)){
						log_err("unable to shit full memory access");
					}
				}
				else{
					if (ir_memory_concrete_split_full_access(&binary_tree_root, access_fragment[i].token, access_fragment[i].offset, access_fragment[i].offset + access_fragment[i].size)){
						log_err("unable to split full memory access");
					}
				}
			}
			ir_memory_concrete_push_full_access(&binary_tree_root, node_cursor);
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

	if (op1->operation_type.mem.order < op2->operation_type.mem.order){
		return -1;
	}
	else if (op1->operation_type.mem.order > op2->operation_type.mem.order){
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
