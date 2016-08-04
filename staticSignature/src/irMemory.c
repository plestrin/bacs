#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "irMemory.h"
#include "irVariableRange.h"
#include "irBuilder.h"
#include "list.h"
#include "base.h"

#define IRMEMORY_ALIAS_HEURISTIC_ESP 				1
#define IRMEMORY_ALIAS_HEURISTIC_CONCRETE_MAY 		1 /* use concrete memory addresses to determine if two addresses MAY alias */
#define IRMEMORY_ALIAS_HEURISTIC_CONCRETE_CANNOT 	1 /* use concrete memory addresses to determine if two addresses CANNOT alias */

#define IRMEMORY_ACCESS_MAX_SIZE 			32
#define IRMEMORY_ACCESS_MAX_NB_FRAGMENT  	(IRMEMORY_ACCESS_MAX_SIZE / 8)

enum aliasResult{
	CANNOT_ALIAS 	= 0x00000000,
	MAY_ALIAS 		= 0x00000001,
	MUST_ALIAS 		= 0x00000003
};

#define ADDRESS_NB_MAX_DEPENDENCE 64 /* it must not exceed 0x0fffffff because the last byte of the flag is reversed */
#define FINGERPRINT_MAX_RECURSION_LEVEL 5

#define FINGERPRINT_FLAG_ESP 		0x00000001
#define FINGERPRINT_FLAG_INCOMPLETE 0x00000002
#define FINGERPRINT_FLAG_LEAF 		0x80000000
#define FINGERPRINT_FLAG_DISABLE 	0x40000000

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
		addr_fgp->flag |= FINGERPRINT_FLAG_INCOMPLETE;
		return;
	}

	if (addr_fgp->nb_dependence < ADDRESS_NB_MAX_DEPENDENCE){
		addr_fgp->dependence[addr_fgp->nb_dependence].node = node;
		addr_fgp->dependence[addr_fgp->nb_dependence].flag = parent_label & 0x0fffffff;
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
				addr_fgp->flag |= FINGERPRINT_FLAG_ESP;
			}
			break;
		}
		#endif
		default 						: {
			break;
		}
	}

	if (nb_dependence == addr_fgp->nb_dependence){
		addr_fgp->dependence[nb_dependence - 1].flag |= FINGERPRINT_FLAG_LEAF;
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
		if ((addr_fgp->dependence[i].flag & 0x0fffffff) == label){
			addrFingerprint_remove(addr_fgp, i);
		}
		else{
			i ++;
		}
	}
}

static void addrFingerprint_enable_all(struct addrFingerprint* addr_fgp){
	uint32_t i;

	for (i = 0; i < addr_fgp->nb_dependence; i++){
		addr_fgp->dependence[i].flag &= ~FINGERPRINT_FLAG_DISABLE;
	}
}

static void addrFingerprint_disable(struct addrFingerprint* addr_fgp, uint32_t index){
	uint32_t i;
	uint32_t label;

	label = addr_fgp->dependence[index].label;
	addr_fgp->dependence[index].flag |= FINGERPRINT_FLAG_DISABLE;

	for (i = 0; i < addr_fgp->nb_dependence; i++){
		if ((addr_fgp->dependence[i].flag & 0x0fffffff) == label){
			if (addr_fgp->dependence[i].flag & FINGERPRINT_FLAG_LEAF){
				addr_fgp->dependence[i].flag |= FINGERPRINT_FLAG_DISABLE;
			}
			else if (!(addr_fgp->dependence[i].flag & FINGERPRINT_FLAG_DISABLE)){
				addrFingerprint_disable(addr_fgp, i);
			}
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
		if (addr1_fgp.dependence[i].flag & FINGERPRINT_FLAG_LEAF){
			dependence_left1[nb_dependence_left1 ++] = addr1_fgp.dependence[i].node;
		}
	}

	for (j = 0, nb_dependence_left2 = 0; j < addr2_fgp.nb_dependence; j++){
		if (addr2_fgp.dependence[j].flag & FINGERPRINT_FLAG_LEAF){
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
		if (addr1_fgp.flag & FINGERPRINT_FLAG_INCOMPLETE || addr2_fgp.flag & FINGERPRINT_FLAG_INCOMPLETE){
			return MAY_ALIAS;
		}
		else if ((addr1_fgp.flag & FINGERPRINT_FLAG_ESP) == (addr2_fgp.flag & FINGERPRINT_FLAG_ESP)){
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
			#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE_MAY == 1
			if (ir_node_get_operation(node1)->operation_type.mem.con_addr != MEMADDRESS_INVALID && ir_node_get_operation(node_cursor)->operation_type.mem.con_addr != MEMADDRESS_INVALID){
				if (ir_node_get_operation(node1)->operation_type.mem.con_addr == ir_node_get_operation(node_cursor)->operation_type.mem.con_addr){
					return node_cursor;
				}
				#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE_CANNOT == 1
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

		#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE_CANNOT == 1
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





















struct memToken{
	struct node* access;
};

#define memToken_is_read(token) (ir_node_get_operation((token)->access)->type == IR_OPERATION_TYPE_IN_MEM)
#define memToken_is_write(token) (ir_node_get_operation((token)->access)->type == IR_OPERATION_TYPE_OUT_MEM)
#define memToken_get_size(token) (ir_node_get_operation((token)->access)->size / 8)
#define memToken_get_conAddr(token) (ir_node_get_operation((token)->access)->operation_type.mem.con_addr)

struct memTokenDynamic{
	struct memToken 	token;
	ADDRESS 			address;
	uint32_t 			offset;
	uint32_t 			size;
};

struct memTokenStatic{
	struct memToken 		token;
	struct addrFingerprint 	addr_fgp;
};

#define memTokenStatic_is_read(static_token) memToken_is_read(&((static_token)->token))
#define memTokenStatic_is_write(static_token) memToken_is_write(&((static_token)->token))
#define memTokenStatic_get_size(static_token) memToken_get_size(&((static_token)->token))
#define memTokenStatic_get_conAddr(static_token) memToken_get_conAddr(&((static_token)->token))


enum accessFragmentType{
	IRMEMORY_ACCESS_NONE 		= 0x00000000,
	IRMEMORY_ACCESS_ALIAS 		= 0x00000010,
	IRMEMORY_ACCESS_ALIAS_READ 	= 0x00000011,
	IRMEMORY_ACCESS_ALIAS_WRITE = 0x00000012,
	IRMEMORY_ACCESS_ALIAS_ALL 	= 0x00000013,
	IRMEMORY_ACCESS_VALUE 		= 0x00000020,
	IRMEMORY_ACCESS_VALUE_READ 	= 0x00000021,
	IRMEMORY_ACCESS_VALUE_WRITE = 0x00000022,
	IRMEMORY_ACCESS_VALUE_ALL 	= 0x00000023
};

struct accessFragment{
	enum accessFragmentType type;
	struct memToken* 		token;
	uint16_t 				size;
	uint8_t 				offset_src;
	uint8_t 				offset_dst;
};

static void accessFragment_merge(struct accessFragment* access_frag_buffer, const struct accessFragment* new_frag){
	uint32_t 	i;
	uint16_t 	local_size;
	uint8_t 	local_offset;

	local_size = new_frag->size;
	local_offset = new_frag->offset_src;

	for (i = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT && local_size; i++){
		if (access_frag_buffer[i].type == IRMEMORY_ACCESS_NONE){
			access_frag_buffer[i].type 			= new_frag->type;
			access_frag_buffer[i].token 		= new_frag->token;
			access_frag_buffer[i].size 			= local_size;
			access_frag_buffer[i].offset_src 	= local_offset;
			access_frag_buffer[i].offset_dst 	= new_frag->offset_dst + local_offset;

			local_size = 0;
		}
		else if (local_offset < access_frag_buffer[i].offset_src){
			memmove(access_frag_buffer + i + 1, access_frag_buffer + i, sizeof(struct accessFragment) * (IRMEMORY_ACCESS_MAX_NB_FRAGMENT - i - 1));
			access_frag_buffer[i].type 			= new_frag->type;
			access_frag_buffer[i].token 		= new_frag->token;
			access_frag_buffer[i].size 			= min(local_size, access_frag_buffer[i + 1].offset_src - local_offset);
			access_frag_buffer[i].offset_src 	= local_offset;
			access_frag_buffer[i].offset_dst 	= new_frag->offset_dst + local_offset;

			local_size -= access_frag_buffer[i].size;
			local_offset += access_frag_buffer[i].size;
		}
		else if (access_frag_buffer[i].offset_src + access_frag_buffer[i].size > local_offset){
			local_size -= min(local_size, (access_frag_buffer[i].offset_src + access_frag_buffer[i].size) - local_offset);
			local_offset = access_frag_buffer[i].offset_src + access_frag_buffer[i].size;
		}
	}
}

static int32_t accessFragment_is_complete(const struct accessFragment* access_frag_buffer, uint32_t access_size){
	uint32_t i;
	uint32_t size;

	for (i = 0, size = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT; i++){
		if (access_frag_buffer[i].type == IRMEMORY_ACCESS_NONE || access_frag_buffer[i].offset_src != size){
			break;
		}
		size += access_frag_buffer[i].size;
		#ifdef EXTRA_CHECK
		if (size > access_size){
			log_err_m("sum of the sizes over the fragment buffer is larger than the size of the access: %u/%u", size, access_size);
		}
		#endif
		if (size >= access_size){
			return 1;
		}
	}

	return 0;
}

static void accessFragment_remove_token(struct accessFragment* access_frag_buffer, uint32_t index){
	uint32_t i;

	for (i = index + 1; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT && access_frag_buffer[i].type != IRMEMORY_ACCESS_NONE; ){
		if (access_frag_buffer[i].token == access_frag_buffer[index].token){
			memmove(access_frag_buffer + i, access_frag_buffer + i + 1, sizeof(struct accessFragment) * (IRMEMORY_ACCESS_MAX_NB_FRAGMENT - i - 1));
			memset(access_frag_buffer + IRMEMORY_ACCESS_MAX_NB_FRAGMENT - 1, 0, sizeof(struct accessFragment));
		}
		else{
			i ++;
		}
	}
}

static struct node* accessFragment_get(const struct accessFragment* access_fragment, struct ir* ir){
	struct node* raw_value 	= NULL;
	struct edge* edge_cursor;

	if (memToken_is_read(access_fragment->token)){
		raw_value = access_fragment->token->access;
	}
	else if (memToken_is_write(access_fragment->token)){
		for (edge_cursor = node_get_head_edge_dst(access_fragment->token->access); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
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
	#ifdef EXTRA_CHECK
	else{
		log_err("incorrect operation type");
		return NULL;
	}
	#endif

	if (access_fragment->offset_dst){
		struct node* disp_value;
		struct node* shift;

		disp_value = ir_add_immediate(ir, ir_node_get_operation(raw_value)->size, access_fragment->offset_dst * 8);
		shift = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(raw_value)->size, IR_SHR, IR_OPERATION_DST_UNKOWN);

		if (disp_value != NULL && shift != NULL){
			ir_add_dependence_check(ir, disp_value, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP)
			ir_add_dependence_check(ir, raw_value, shift, IR_DEPENDENCE_TYPE_DIRECT)
			raw_value = shift;
		}
		else{
			log_err("unable to add nodes to IR");
		}
	}

	if (access_fragment->size != ir_node_get_operation(raw_value)->size / 8){
		struct node* part = NULL;

		if (access_fragment->size == 1 && ir_node_get_operation(raw_value)->size == 32){
			part = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
		}
		else if (access_fragment->size == 1 && ir_node_get_operation(raw_value)->size == 16){
			part = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
		}
		else if (access_fragment->size == 2 && ir_node_get_operation(raw_value)->size == 32){
			part = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 16, IR_PART1_16, IR_OPERATION_DST_UNKOWN);
		}
		else{
			log_err_m("incorrect size selection %u byte(s) from %u", access_fragment->size, ir_node_get_operation(raw_value)->size / 8);
		}

		if (part != NULL){
			ir_add_dependence_check(ir, raw_value, part, IR_DEPENDENCE_TYPE_DIRECT)
			raw_value = part;
		}
		else{
			log_err("unable to add instruction to IR");
		}
	}

	return raw_value;
}

static int32_t accessFragment_build_compound(const struct accessFragment* access_fragment, struct ir* ir, struct node** current_value, struct node* new_value, uint32_t access_size){
	struct node* or;
	struct node* movzx;
	struct node* disp_value;
	struct node* shift;

	if (new_value == NULL){
		log_err("new value is NULL");
		return -1;
	}

	if (ir_node_get_operation(new_value)->size < access_size){
		if ((movzx = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, access_size, IR_MOVZX, IR_OPERATION_DST_UNKOWN)) != NULL){
			ir_add_dependence_check(ir, new_value, movzx, IR_DEPENDENCE_TYPE_DIRECT)
			new_value = movzx;
		}
		else{
			log_err("unable to add instruction to IR");
		}
	}

	if (access_fragment->offset_src){
		disp_value = ir_add_immediate(ir, access_size, access_fragment->offset_src * 8);
		shift = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, access_size, IR_SHL, IR_OPERATION_DST_UNKOWN);

		if (disp_value != NULL && shift != NULL){
			ir_add_dependence_check(ir, disp_value, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP)
			ir_add_dependence_check(ir, new_value, shift, IR_DEPENDENCE_TYPE_DIRECT)
			new_value = shift;
		}
		else{
			log_err("unable to add nodes to IR");
		}
	}

	if (*current_value == NULL){
		*current_value = new_value;
	}
	else{
		if ((or = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, access_size, IR_OR, IR_OPERATION_DST_UNKOWN)) != NULL){
			ir_add_dependence_check(ir, *current_value, or, IR_DEPENDENCE_TYPE_DIRECT)
			ir_add_dependence_check(ir, new_value, or, IR_DEPENDENCE_TYPE_DIRECT)
			*current_value = or;
		}
		else{
			log_err("unable to add instruction to IR");
			return -1;
		}
	}

	return 0;
}

static void accessFragment_print(const struct accessFragment* access_frag_buffer, const struct memToken* token){
	uint32_t i;

	for (i = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT && access_frag_buffer[i].type != IRMEMORY_ACCESS_NONE; i++){
		switch(access_frag_buffer[i].type){
			case IRMEMORY_ACCESS_ALIAS_READ 	: {
				printf("ALIAS READ  [%u:%u] ", access_frag_buffer[i].offset_src, access_frag_buffer[i].offset_src + access_frag_buffer[i].size);
				if (memToken_is_read(token)){
					puts(ANSI_COLOR_RED "ignore" ANSI_COLOR_RESET " this case is not supposed to happen");
				}
				else{
					puts(ANSI_COLOR_YELLOW "aliasing" ANSI_COLOR_RESET);
				}
				break;
			}
			case IRMEMORY_ACCESS_ALIAS_WRITE 	: {
				printf("ALIAS WRITE [%u:%u] ", access_frag_buffer[i].offset_src, access_frag_buffer[i].offset_src + access_frag_buffer[i].size);
				if (memToken_is_read(token)){
					puts(ANSI_COLOR_YELLOW "aliasing" ANSI_COLOR_RESET);
				}
				else{
					puts(ANSI_COLOR_RED "ignore" ANSI_COLOR_RESET " this case is not supposed to happen");
				}
				break;
			}
			case IRMEMORY_ACCESS_VALUE_READ 	: {
				printf("READ        [%u:%u] ", access_frag_buffer[i].offset_src, access_frag_buffer[i].offset_src + access_frag_buffer[i].size);
				if (memToken_is_read(token)){
					puts(ANSI_COLOR_GREEN "simplification RR" ANSI_COLOR_RESET);
				}
				else{
					puts("no simplification");
				}
				break;
			}
			case IRMEMORY_ACCESS_VALUE_WRITE 	: {
				printf("WRITE       [%u:%u] ", access_frag_buffer[i].offset_src, access_frag_buffer[i].offset_src + access_frag_buffer[i].size);
				if (memToken_is_read(token)){
						puts(ANSI_COLOR_GREEN "simplification WR" ANSI_COLOR_RESET);
				}
				else{
					puts(ANSI_COLOR_GREEN "simplification WW" ANSI_COLOR_RESET);
				}
				break;
			}
			default : {log_err_m("this case is not supposed to happen: type=0x%08x", access_frag_buffer[i].type); break;}
		}

	}
}




































static int32_t memTokenDynamic_is_alive(struct memTokenDynamic* mem_token_dyn, struct node* current_node){
	struct node* node_cursor;

	for (node_cursor = ir_node_get_operation(current_node)->operation_type.mem.prev; node_cursor != NULL; node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.prev){
		if (node_cursor == mem_token_dyn->token.access){
			return 1;
		}
	}

	return 0;
}

static int32_t memTokenDynamic_compare_address(const void* arg1, const void* arg2);

static int32_t ir_memory_concrete_push_full_access(void** binary_tree_root, struct node* node){
	struct memTokenDynamic* 	new_token;
	struct memTokenDynamic** 	result;

	if ((new_token = (struct memTokenDynamic*)malloc(sizeof(struct memTokenDynamic))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	new_token->token.access = node;
	new_token->address 		= ir_node_get_operation(node)->operation_type.mem.con_addr;
	new_token->offset 		= 0;
	new_token->size 		= ir_node_get_operation(node)->size / 8;

	result = (struct memTokenDynamic**)tsearch((void*)new_token, binary_tree_root, memTokenDynamic_compare_address);
	if (result == NULL){
		log_err("unable to insert memTokenDynamic in the binary tree");
		free(new_token);
		return -1;
	}
	else if (*result != new_token){
		log_err("a memTokenDynamic is already available at this address");
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

static int32_t ir_memory_concrete_shift_full_access(void** binary_tree_root, struct memTokenDynamic* token, uint32_t disp){
	struct memTokenDynamic** result;

	if (tdelete(token, binary_tree_root, memTokenDynamic_compare_address) == NULL){
		log_err("unable to delete token from the binary tree");
		return -1;
	}

	token->address 	+= disp;
	token->offset 	+= disp;
	token->size 	-= disp;

	result = (struct memTokenDynamic**)tsearch((void*)token, binary_tree_root, memTokenDynamic_compare_address);
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

static int32_t ir_memory_concrete_split_full_access(void** binary_tree_root, struct memTokenDynamic* token, uint32_t l_bound, uint32_t h_bound){
	struct memTokenDynamic* 	new_token;
	struct memTokenDynamic** 	result;

	token->size = l_bound;

	if ((new_token = (struct memTokenDynamic*)malloc(sizeof(struct memTokenDynamic))) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	new_token->token.access = token->token.access;
	new_token->address 		= token->address + h_bound;
	new_token->offset 		= h_bound;
	new_token->size 		= token->size - h_bound;

	result = (struct memTokenDynamic**)tsearch((void*)new_token, binary_tree_root, memTokenDynamic_compare_address);
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

struct node* irMemory_get_first(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_IN_MEM || operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM){
			break;
		}
	}

	if (node_cursor != NULL){
		while (operation_cursor->operation_type.mem.prev != NULL){
			node_cursor = operation_cursor->operation_type.mem.prev;
			operation_cursor = ir_node_get_operation(node_cursor);
		}
	}

	return node_cursor;
}

/* ===================================================================== */
/* Sorting routines						                                 */
/* ===================================================================== */

static int32_t memTokenDynamic_compare_address(const void* arg1, const void* arg2){
	const struct memTokenDynamic* token1 = (const struct memTokenDynamic*)arg1;
	const struct memTokenDynamic* token2 = (const struct memTokenDynamic*)arg2;

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

/* this is the new stuff BRUTAL */

static int32_t memTokenStatic_init(struct memTokenStatic* mem_token_static, struct node* node){
	struct node* address;

	if (ir_node_get_operation(node)->size > IRMEMORY_ACCESS_MAX_SIZE){
		log_warn_m("memory access size (%u) exceeds max limit (%u)", ir_node_get_operation(node)->size, IRMEMORY_ACCESS_MAX_SIZE);
	}

	mem_token_static->token.access = node;

	if ((address = irMemory_get_address(node)) == NULL){
		log_err("unable to get memory address");
		return -1;
	}

	addrFingerprint_init(address, &(mem_token_static->addr_fgp), 0, 0);

	return 0;
}

static uint32_t memTokenStatic_compare(struct memTokenStatic* token1, struct memTokenStatic* token2, uint32_t ir_range_seed, struct accessFragment* access_frag_buffer){
	uint32_t 				i;
	uint32_t 				j;
	struct variableRange 	range1;
	struct variableRange 	range2;
	uint32_t 				nb_dependence_left1;
	uint32_t 				nb_dependence_left2;
	struct node* 			dependence_left1[ADDRESS_NB_MAX_DEPENDENCE];
	struct node* 			dependence_left2[ADDRESS_NB_MAX_DEPENDENCE];

	#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE_CANNOT == 1
	if (memTokenStatic_get_conAddr(token1) != MEMADDRESS_INVALID && memTokenStatic_get_conAddr(token2) != MEMADDRESS_INVALID){
		if (memTokenStatic_get_conAddr(token1) < memTokenStatic_get_conAddr(token2)){
			if (memTokenStatic_get_conAddr(token2) - memTokenStatic_get_conAddr(token1) > memTokenStatic_get_size(token1)){
				return 0;
			}
		}
		else{
			if (memTokenStatic_get_conAddr(token1) - memTokenStatic_get_conAddr(token2) > memTokenStatic_get_size(token2)){
				return 0;
			}
		}
	}
	#endif

	addrFingerprint_enable_all(&(token1->addr_fgp));
	addrFingerprint_enable_all(&(token2->addr_fgp));

	for (i = 0; i < token1->addr_fgp.nb_dependence; i++){
		if (token1->addr_fgp.dependence[i].flag & FINGERPRINT_FLAG_DISABLE){
			continue;
		}

		for (j = 0; j < token2->addr_fgp.nb_dependence; j++){
			if (token1->addr_fgp.dependence[i].node == token2->addr_fgp.dependence[j].node){
				if (token1->addr_fgp.dependence[j].flag & FINGERPRINT_FLAG_DISABLE){
					continue;
				}
				addrFingerprint_disable(&token1->addr_fgp, i);
				addrFingerprint_disable(&token2->addr_fgp, j);
			}
		}
	}

	for (i = 0, nb_dependence_left1 = 0; i < token1->addr_fgp.nb_dependence; i++){
		if ((token1->addr_fgp.dependence[i].flag & FINGERPRINT_FLAG_LEAF) && !(token1->addr_fgp.dependence[i].flag & FINGERPRINT_FLAG_DISABLE)){
			dependence_left1[nb_dependence_left1 ++] = token1->addr_fgp.dependence[i].node;
		}
	}

	for (j = 0, nb_dependence_left2 = 0; j < token2->addr_fgp.nb_dependence; j++){
		if ((token2->addr_fgp.dependence[j].flag & FINGERPRINT_FLAG_LEAF) && !(token2->addr_fgp.dependence[j].flag & FINGERPRINT_FLAG_DISABLE)){
			dependence_left2[nb_dependence_left2 ++] = token2->addr_fgp.dependence[j].node;
		}
	}

	irVariableRange_get_range_add_buffer(&range1, dependence_left1, nb_dependence_left1, 32, ir_range_seed);
	irVariableRange_get_range_add_buffer(&range2, dependence_left2, nb_dependence_left2, 32, ir_range_seed);

	if (variableRange_is_cst(&range1) && variableRange_is_cst(&range2)){
		if (variableRange_get_cst(&range1) <= variableRange_get_cst(&range2)){
			#ifdef EXTRA_CHECK
			if (memTokenStatic_get_conAddr(token1) != MEMADDRESS_INVALID && memTokenStatic_get_conAddr(token2) != MEMADDRESS_INVALID){
				if (memTokenStatic_get_conAddr(token2) - memTokenStatic_get_conAddr(token1) != variableRange_get_cst(&range2) - variableRange_get_cst(&range1)){
					log_err_m("incoherence between fingerprints and concrete addresses: " PRINTF_ADDR " - " PRINTF_ADDR, memTokenStatic_get_conAddr(token1), memTokenStatic_get_conAddr(token1));
				}
			}
			#endif

			if (variableRange_get_cst(&range2) - variableRange_get_cst(&range1) < memTokenStatic_get_size(token1)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_VALUE;
				access_frag_buffer[0].token 		= (struct memToken*)token2;
				access_frag_buffer[0].size 			= (uint16_t)(min(variableRange_get_cst(&range1) + memTokenStatic_get_size(token1) - variableRange_get_cst(&range2), memTokenStatic_get_size(token2)));
				access_frag_buffer[0].offset_src 	= (uint8_t)(variableRange_get_cst(&range2) - variableRange_get_cst(&range1));
				access_frag_buffer[0].offset_dst 	= 0;

				return 1;
			}
			else{
				return 0;
			}
		}
		else{
			#ifdef EXTRA_CHECK
			if (memTokenStatic_get_conAddr(token1) != MEMADDRESS_INVALID && memTokenStatic_get_conAddr(token2) != MEMADDRESS_INVALID){
				if (memTokenStatic_get_conAddr(token1) - memTokenStatic_get_conAddr(token2) != variableRange_get_cst(&range1) - variableRange_get_cst(&range2)){
					log_err_m("incoherence between fingerprints and concrete addresses: " PRINTF_ADDR " - " PRINTF_ADDR, memTokenStatic_get_conAddr(token1), memTokenStatic_get_conAddr(token1));
				}
			}
			#endif

			if (variableRange_get_cst(&range1) - variableRange_get_cst(&range2) < memTokenStatic_get_size(token2)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_VALUE;
				access_frag_buffer[0].token 		= (struct memToken*)token2;
				access_frag_buffer[0].size 			= (uint16_t)(min(variableRange_get_cst(&range2) + memTokenStatic_get_size(token2) - variableRange_get_cst(&range1), memTokenStatic_get_size(token1)));
				access_frag_buffer[0].offset_src 	= 0;
				access_frag_buffer[0].offset_dst 	= (uint8_t)(variableRange_get_cst(&range1) - variableRange_get_cst(&range2));

				return 1;
			}
			else{
				return 0;
			}
		}
	}
	#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE_MAY == 1  /* fragment are not handled for aliasing */
	if (memTokenStatic_get_conAddr(token1) != MEMADDRESS_INVALID && memTokenStatic_get_conAddr(token2) != MEMADDRESS_INVALID){
		if (memTokenStatic_get_conAddr(token1) < memTokenStatic_get_conAddr(token2)){
			if (memTokenStatic_get_conAddr(token2) - memTokenStatic_get_conAddr(token1) < memTokenStatic_get_size(token1)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_ALIAS;
				access_frag_buffer[0].token 		= (struct memToken*)token2;
				access_frag_buffer[0].size 			= memTokenStatic_get_size(token1);
				access_frag_buffer[0].offset_src 	= 0;
				access_frag_buffer[0].offset_dst 	= 0;

				return 1;
			}
		}
		else{
			if (memTokenStatic_get_conAddr(token1) - memTokenStatic_get_conAddr(token2) < memTokenStatic_get_size(token2)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_ALIAS;
				access_frag_buffer[0].token 		= (struct memToken*)token2;
				access_frag_buffer[0].size 			= memTokenStatic_get_size(token1);
				access_frag_buffer[0].offset_src 	= 0;
				access_frag_buffer[0].offset_dst 	= 0;

				return 1;
			}
		}
	}
	#endif
	else if (variableRange_intersect(&range1, &range2)){ /* fragment are not handled for aliasing */
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		if (!(token1->addr_fgp.flag & FINGERPRINT_FLAG_INCOMPLETE || token2->addr_fgp.flag & FINGERPRINT_FLAG_INCOMPLETE) && (token1->addr_fgp.flag & FINGERPRINT_FLAG_ESP) != (token2->addr_fgp.flag & FINGERPRINT_FLAG_ESP)){
			return 0;
		}
		#endif

		access_frag_buffer[0].type 			= IRMEMORY_ACCESS_ALIAS;
		access_frag_buffer[0].token 		= (struct memToken*)token2;
		access_frag_buffer[0].size 			= memTokenStatic_get_size(token1);
		access_frag_buffer[0].offset_src 	= 0;
		access_frag_buffer[0].offset_dst 	= 0;

		return 1;
	}

	return 0;
}

static void memTokenStatic_search(struct memTokenStatic* token, struct list* token_list, uint32_t ir_range_seed, struct accessFragment* access_frag_buffer){
	uint32_t 				i;
	struct listIterator 	it;
	struct memTokenStatic* 	token_cursor;
	struct accessFragment* 	local_access_frag_buffer;
	uint32_t 				nb_frag;

	local_access_frag_buffer = (struct accessFragment*)alloca(sizeof(struct accessFragment) * memTokenStatic_get_size(token));

	memset(access_frag_buffer, 0, sizeof(struct accessFragment) * IRMEMORY_ACCESS_MAX_NB_FRAGMENT);

	for (listIterator_init(&it, token_list); listIterator_get_prev(&it) != NULL; ){
		token_cursor = listIterator_get_data(it);
		nb_frag = memTokenStatic_compare(token, token_cursor, ir_range_seed, local_access_frag_buffer);

		for (i = 0; i < nb_frag; i++){
			local_access_frag_buffer[i].type |= 0x00000001 << ((memTokenStatic_is_write(token_cursor)) ? 1 : 0);
			if ((memTokenStatic_is_read(token) && local_access_frag_buffer[i].type == IRMEMORY_ACCESS_ALIAS_READ) || (memTokenStatic_is_write(token) && local_access_frag_buffer[i].type == IRMEMORY_ACCESS_ALIAS_WRITE)){
				continue;
			}

			accessFragment_merge(access_frag_buffer, local_access_frag_buffer + i);
		}

		if (accessFragment_is_complete(access_frag_buffer, memTokenStatic_get_size(token))){
			break;
		}
	}
}

int32_t irMemory_simplify(struct ir* ir){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				size;
	int32_t 				result;
	struct node* 			node_cursor;
	struct node* 			next_node_cursor;
	struct list 			mem_token_list;
	struct memTokenStatic 	current_token;
	struct accessFragment 	fragments[IRMEMORY_ACCESS_MAX_NB_FRAGMENT];

	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	list_init(mem_token_list, sizeof(struct memTokenStatic))

	for (node_cursor = irMemory_get_first(ir), result = 0; node_cursor != NULL; node_cursor = next_node_cursor){
		next_node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.next;

		if (memTokenStatic_init(&current_token, node_cursor)){
			log_err("unable to init memTokenStatic, further simplification might be incorrect");
			continue;
		}

		memTokenStatic_search(&current_token, &mem_token_list, ir->range_seed, fragments);

		if (memTokenStatic_is_read(&current_token)){
			for (i = 0, size = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT; i++){
				if (fragments[i].type == IRMEMORY_ACCESS_NONE || (fragments[i].type & IRMEMORY_ACCESS_ALIAS) || fragments[i].offset_src != size){
					break;
				}
				size += fragments[i].size;
			}

			#ifdef EXTRA_CHECK
			if (size > memTokenStatic_get_size(&current_token)){
				log_err_m("sum of the sizes over the fragment buffer is larger than the size of the access: %u/%u", size, memTokenStatic_get_size(&current_token));
			}
			#endif

			if (size >= memTokenStatic_get_size(&current_token)){
				struct node* value;

				for (j = 0, value = NULL; j < i; j++){
					if (accessFragment_build_compound(fragments + j, ir, &value, accessFragment_get(fragments + j, ir), memTokenStatic_get_size(&current_token))){
						log_err("unable to build compound value");
					}
				}

				if (value != NULL){
					ir_merge_equivalent_node(ir, value, node_cursor);
					result = 1;
				}
				else{
					log_err("compound value is NULL, further simplification might be incorrect");
				}
			}
			else{
				list_add_tail_check(&mem_token_list, &current_token)
			}
		}
		else{
			for (i = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT && fragments[i].type != IRMEMORY_ACCESS_NONE; i++){
				if (fragments[i].type == IRMEMORY_ACCESS_VALUE_WRITE){
					if (fragments[i].offset_dst == 0 && fragments[i].size >= memToken_get_size(fragments[i].token)){
						ir_merge_equivalent_node(ir, node_cursor, fragments[i].token->access);
						list_remove(&mem_token_list, fragments[i].token);
						accessFragment_remove_token(fragments, i);
						result = 1;
					}
					#ifdef VERBOSE
					else{
						log_warn("simplification of memory STOREs of different size: not implemented");
					}
					#endif
				}
			}

			list_add_tail_check(&mem_token_list, &current_token)
		}
	}

	list_clean(&mem_token_list);

	return result;
}

void irMemory_print_aliasing(struct ir* ir){
	struct node* 			node_cursor;
	struct list 			mem_token_list;
	struct memTokenStatic 	current_token;
	struct accessFragment 	fragments[IRMEMORY_ACCESS_MAX_NB_FRAGMENT];

	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	list_init(mem_token_list, sizeof(struct memTokenStatic))

	for (node_cursor = irMemory_get_first(ir); node_cursor != NULL; node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.next){
		if (memTokenStatic_init(&current_token, node_cursor)){
			log_err("unable to init memTokenStatic");
			continue;
		}

		memTokenStatic_search(&current_token, &mem_token_list, ir->range_seed, fragments);
		accessFragment_print(fragments, &(current_token.token));

		list_add_tail_check(&mem_token_list, &current_token)
	}

	list_clean(&mem_token_list);
}






























uint32_t irMemory_simplify_concrete(struct ir* ir){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	struct node* 				node_cursor;
	struct node* 				next_node_cursor;
	struct irOperation* 		operation_cursor;
	void* 						binary_tree_root 	= NULL;
	struct memTokenDynamic 		fetch_token;
	struct memTokenDynamic** 	existing_token;
	struct memTokenDynamic* 	dead_token;
	struct accessFragment 		access_fragment[IRMEMORY_ACCESS_MAX_NB_FRAGMENT];
	uint32_t 					nb_mem_access_fragment;
	uint8_t 					taken_fragment[IRMEMORY_ACCESS_MAX_NB_FRAGMENT]; /* ca ne doit plus être utile */
	uint32_t 					result 				= 0;

	for (node_cursor = irMemory_get_first(ir); node_cursor != NULL; node_cursor = next_node_cursor){
		operation_cursor = ir_node_get_operation(node_cursor);
		next_node_cursor = operation_cursor->operation_type.mem.next;

		if (operation_cursor->operation_type.mem.con_addr == MEMADDRESS_INVALID){
			log_warn("a memory access has an incorrect concrete address, further simplifications might be incorrect");
			continue;
		}

		if (operation_cursor->size > IRMEMORY_ACCESS_MAX_SIZE){
			log_warn_m("memory access size (%u) exceeds max limit (%u), further simplification might be incorrect", operation_cursor->size, IRMEMORY_ACCESS_MAX_SIZE);
			continue;
		}

		/* init token */
		/* search */
		/* simplify */

		fetch_token.token.access 	= NULL;
		fetch_token.address 		= operation_cursor->operation_type.mem.con_addr;

		memset(taken_fragment, 0, sizeof(uint8_t) * IRMEMORY_ACCESS_MAX_NB_FRAGMENT);

		for (i = 0, j = 0, nb_mem_access_fragment = 0; i < operation_cursor->size / 8; ){
			int32_t is_alive = 1;

			existing_token = (struct memTokenDynamic**)tfind((void*)&fetch_token, &binary_tree_root, memTokenDynamic_compare_address);
			if (existing_token != NULL && (is_alive = memTokenDynamic_is_alive(*existing_token, node_cursor)) && (*existing_token)->size > j){
				access_fragment[nb_mem_access_fragment].type 		= /**/;
				access_fragment[nb_mem_access_fragment].token 		= *existing_token->token;
				access_fragment[nb_mem_access_fragment].size 		= min((*existing_token)->size - j, (operation_cursor->size / 8) - i);
				access_fragment[nb_mem_access_fragment].offset_src 	= /*j*/;
				access_fragment[nb_mem_access_fragment].offset_dst 	= /*j*/;

				memset(taken_fragment + i, 1, access_fragment[nb_mem_access_fragment].size);

				i += access_fragment[nb_mem_access_fragment].size;
				fetch_token.address += access_fragment[nb_mem_access_fragment].size + j;
				nb_mem_access_fragment ++;
				j = 0;
			}
			else{
				if (!is_alive){
					dead_token = *existing_token;
					if (tdelete(dead_token, &binary_tree_root, memTokenDynamic_compare_address) == NULL){
						log_err("unable to delete token from the binary tree");
					}
					free(dead_token);
				}
				if (i == 0){
					if (j + 1 == IRMEMORY_ACCESS_MAX_NB_FRAGMENT){
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
					if (ir_memory_concrete_build_compound_value(ir, &value, ir_memory_concrete_get_access_value(ir, access_fragment + j, operation_cursor->size), i, operation_cursor->size)){
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
					if (tdelete(access_fragment[i].token, &binary_tree_root, memTokenDynamic_compare_address) == NULL){
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

	return result;
}
