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
#define IRMEMORY_ACCESS_MAX_NB_FRAGMENT 	(IRMEMORY_ACCESS_MAX_SIZE / 8)

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

static int32_t addrFingerprint_init(struct node* node, struct addrFingerprint* addr_fgp, uint32_t recursion_level, uint32_t parent_label){
	struct irOperation* operation;
	struct edge* 		operand_cursor;
	uint32_t 			nb_dependence;
	int32_t 			result 				= 0;

	if (recursion_level == 0){
		addr_fgp->nb_dependence = 0;
		addr_fgp->flag 			= 0;
	}
	else if (recursion_level == FINGERPRINT_MAX_RECURSION_LEVEL){
		addr_fgp->flag |= FINGERPRINT_FLAG_INCOMPLETE;
		return 0;
	}

	if (addr_fgp->nb_dependence < ADDRESS_NB_MAX_DEPENDENCE){
		addr_fgp->dependence[addr_fgp->nb_dependence].node = node;
		addr_fgp->dependence[addr_fgp->nb_dependence].flag = parent_label & 0x0fffffff;
		addr_fgp->dependence[addr_fgp->nb_dependence].label = addr_fgp->nb_dependence + 1;

		nb_dependence = ++ addr_fgp->nb_dependence;
	}
	else{
		log_err("the max number of dependence has been reached");
		return -1;
	}

	operation = ir_node_get_operation(node);

	switch(operation->type){
		case IR_OPERATION_TYPE_INST 	: {
			if (operation->operation_type.inst.opcode == IR_ADD){
				for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
					result |= addrFingerprint_init(edge_get_src(operand_cursor), addr_fgp, recursion_level + 1, nb_dependence);
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

	return result;
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

struct memToken{
	struct node* access;
};

#define memToken_is_read(token) (ir_node_get_operation((token)->access)->type == IR_OPERATION_TYPE_IN_MEM)
#define memToken_is_write(token) (ir_node_get_operation((token)->access)->type == IR_OPERATION_TYPE_OUT_MEM)
#define memToken_get_size(token) (ir_node_get_operation((token)->access)->size / 8)
#define memToken_get_conAddr(token) (ir_node_get_operation((token)->access)->operation_type.mem.con_addr)

struct memTokenDynamic{
	struct memToken token; 					/* must be first */
};

#define memTokenDynamic_is_read(dyn_token) memToken_is_read(&((dyn_token)->token))
#define memTokenDynamic_is_write(dyn_token) memToken_is_write(&((dyn_token)->token))
#define memTokenDynamic_get_size(dyn_token) memToken_get_size(&((dyn_token)->token))
#define memTokenDynamic_get_conAddr(dyn_token) memToken_get_conAddr(&((dyn_token)->token))
#define memTokenDynamic_get_token(dyn_token) ((struct memToken*)(dyn_token))

struct memTokenStatic{
	struct memToken 		token; 			/* must be first */
	struct addrFingerprint 	addr_fgp;
};

#define memTokenStatic_is_read(static_token) memToken_is_read(&((static_token)->token))
#define memTokenStatic_is_write(static_token) memToken_is_write(&((static_token)->token))
#define memTokenStatic_get_size(static_token) memToken_get_size(&((static_token)->token))
#define memTokenStatic_get_conAddr(static_token) memToken_get_conAddr(&((static_token)->token))
#define memTokenStatic_get_token(static_token) ((struct memToken*)(static_token))

#define memToken_getDynamicToken(token) ((struct memTokenDynamic*)(token))
#define memToken_getStaticToken(token) ((struct memTokenStatic*)(token))

enum accessFragmentType{
	IRMEMORY_ACCESS_NONE 		= 0x00000000,
	IRMEMORY_ACCESS_ALIAS 		= 0x00000010,
	IRMEMORY_ACCESS_ALIAS_READ 	= 0x00000011,
	IRMEMORY_ACCESS_ALIAS_WRITE = 0x00000012,
	IRMEMORY_ACCESS_ALIAS_ALL 	= 0x00000013,
	IRMEMORY_ACCESS_VALUE 		= 0x00000020,
	IRMEMORY_ACCESS_VALUE_READ 	= 0x00000021,
	IRMEMORY_ACCESS_VALUE_WRITE = 0x00000022,
	IRMEMORY_ACCESS_VALUE_ALL 	= 0x00000023,
	IRMEMORY_ACCESS_NEW 		= 0x00000040
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

static void accessFragment_fill(struct accessFragment* access_frag_buffer, struct memToken* token){
	uint32_t i;
	uint32_t size;

	for (i = 0, size = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT; i++){
		if (access_frag_buffer[i].type == IRMEMORY_ACCESS_NONE){
			if (size < memToken_get_size(token)){
				access_frag_buffer[i].type 			= IRMEMORY_ACCESS_NEW;
				access_frag_buffer[i].token 		= token;
				access_frag_buffer[i].size 			= memToken_get_size(token) - size;
				access_frag_buffer[i].offset_src 	= size;
				access_frag_buffer[i].offset_dst 	= size;
			}

			break;
		}
		else if (size < access_frag_buffer[i].offset_src){
			memmove(access_frag_buffer + i + 1, access_frag_buffer + i, sizeof(struct accessFragment) * (IRMEMORY_ACCESS_MAX_NB_FRAGMENT - i - 1));

			access_frag_buffer[i].type 			= IRMEMORY_ACCESS_NEW;
			access_frag_buffer[i].token 		= token;
			access_frag_buffer[i].size 			= access_frag_buffer[i + 1].offset_src - size;
			access_frag_buffer[i].offset_src 	= size;
			access_frag_buffer[i].offset_dst 	= size;

		}

		size = access_frag_buffer[i].offset_src + access_frag_buffer[i].size;
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

	if (access_fragment->size * 8 != ir_node_get_operation(raw_value)->size){
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

static struct node* accessFragment_create(struct accessFragment* access_fragment, struct ir* ir, struct node* insert_location){
	struct node* addr;
	struct node* access;

	if (access_fragment->size != memToken_get_size(access_fragment->token)){
		if ((addr = irMemory_get_address_node(access_fragment->token->access)) == NULL){
			log_err("unable to get memory address");
			return NULL;
		}

		if (access_fragment->offset_dst){
			struct node* off_val;
			struct node* add;

			off_val = ir_add_immediate(ir, 32, access_fragment->offset_dst);
			add = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_ADD, IR_OPERATION_DST_UNKOWN);

			if (off_val != NULL && add != NULL){
				ir_add_dependence_check(ir, off_val, add, IR_DEPENDENCE_TYPE_DIRECT)
				ir_add_dependence_check(ir, addr, add, IR_DEPENDENCE_TYPE_DIRECT)
				addr = add;
			}
			else{
				log_err("unable to add nodes to IR");
				return NULL;
			}
		}

		if ((access = ir_add_in_mem_(ir, ir_node_get_operation(access_fragment->token->access)->index, access_fragment->size * 8, addr, insert_location, memToken_get_conAddr(access_fragment->token) + access_fragment->offset_dst)) == NULL){
			log_err("unable to add memory access to IR");
		}

		return access;
	}
	else{
		return access_fragment->token->access;
	}
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

static int32_t memTokenStatic_init(struct memTokenStatic* mem_token_static, struct node* node){
	struct node* address;

	if (ir_node_get_operation(node)->size > IRMEMORY_ACCESS_MAX_SIZE){
		log_warn_m("memory access size (%u) exceeds max limit (%u)", ir_node_get_operation(node)->size, IRMEMORY_ACCESS_MAX_SIZE);
		return -1;
	}

	mem_token_static->token.access = node;

	if ((address = irMemory_get_address_node(node)) == NULL){
		log_err("unable to get memory address");
		return -1;
	}

	return addrFingerprint_init(address, &(mem_token_static->addr_fgp), 0, 0);
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
			if (token2->addr_fgp.dependence[j].flag & FINGERPRINT_FLAG_DISABLE){
				continue;				
			}
			if (token1->addr_fgp.dependence[i].node == token2->addr_fgp.dependence[j].node){
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
					log_err_m("incoherence between fingerprints and concrete addresses: " PRINTF_ADDR " - " PRINTF_ADDR, memTokenStatic_get_conAddr(token1), memTokenStatic_get_conAddr(token2));
				}
			}
			#endif

			if (variableRange_get_cst(&range2) - variableRange_get_cst(&range1) < memTokenStatic_get_size(token1)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_VALUE;
				access_frag_buffer[0].token 		= memTokenStatic_get_token(token2);
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
					log_err_m("incoherence between fingerprints and concrete addresses: " PRINTF_ADDR " - " PRINTF_ADDR, memTokenStatic_get_conAddr(token1), memTokenStatic_get_conAddr(token2));
				}
			}
			#endif

			if (variableRange_get_cst(&range1) - variableRange_get_cst(&range2) < memTokenStatic_get_size(token2)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_VALUE;
				access_frag_buffer[0].token 		= memTokenStatic_get_token(token2);
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
	#if IRMEMORY_ALIAS_HEURISTIC_CONCRETE_MAY == 1 /* fragment are not handled for aliasing */
	if (memTokenStatic_get_conAddr(token1) != MEMADDRESS_INVALID && memTokenStatic_get_conAddr(token2) != MEMADDRESS_INVALID){
		if (memTokenStatic_get_conAddr(token1) < memTokenStatic_get_conAddr(token2)){
			if (memTokenStatic_get_conAddr(token2) - memTokenStatic_get_conAddr(token1) < memTokenStatic_get_size(token1)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_ALIAS;
				access_frag_buffer[0].token 		= memTokenStatic_get_token(token2);
				access_frag_buffer[0].size 			= memTokenStatic_get_size(token1);
				access_frag_buffer[0].offset_src 	= 0;
				access_frag_buffer[0].offset_dst 	= 0;

				return 1;
			}
		}
		else{
			if (memTokenStatic_get_conAddr(token1) - memTokenStatic_get_conAddr(token2) < memTokenStatic_get_size(token2)){
				access_frag_buffer[0].type 			= IRMEMORY_ACCESS_ALIAS;
				access_frag_buffer[0].token 		= memTokenStatic_get_token(token2);
				access_frag_buffer[0].size 			= memTokenStatic_get_size(token1);
				access_frag_buffer[0].offset_src 	= 0;
				access_frag_buffer[0].offset_dst 	= 0;

				return 1;
			}
		}
	}
	#endif
	else if (variableRange_is_range_intersect(&range1, &range2)){ /* fragment are not handled for aliasing */
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		if (!(token1->addr_fgp.flag & FINGERPRINT_FLAG_INCOMPLETE || token2->addr_fgp.flag & FINGERPRINT_FLAG_INCOMPLETE) && (token1->addr_fgp.flag & FINGERPRINT_FLAG_ESP) != (token2->addr_fgp.flag & FINGERPRINT_FLAG_ESP)){
			return 0;
		}
		#endif

		access_frag_buffer[0].type 			= IRMEMORY_ACCESS_ALIAS;
		access_frag_buffer[0].token 		= memTokenStatic_get_token(token2);
		access_frag_buffer[0].size 			= memTokenStatic_get_size(token1);
		access_frag_buffer[0].offset_src 	= 0;
		access_frag_buffer[0].offset_dst 	= 0;

		return 1;
	}

	return 0;
}

static void memTokenStatic_search(struct memTokenStatic* mem_token_static, struct list* token_list, uint32_t ir_range_seed, struct accessFragment* access_frag_buffer){
	uint32_t 				i;
	struct listIterator 	it;
	struct memTokenStatic* 	mem_token_static_cursor;
	struct accessFragment* 	local_access_frag_buffer;
	uint32_t 				nb_frag;
	struct node* 			node_cursor;

	local_access_frag_buffer = (struct accessFragment*)alloca(sizeof(struct accessFragment) * memTokenStatic_get_size(mem_token_static));

	memset(access_frag_buffer, 0, sizeof(struct accessFragment) * IRMEMORY_ACCESS_MAX_NB_FRAGMENT);

	for (listIterator_init(&it, token_list), node_cursor = ir_node_get_operation(mem_token_static->token.access)->operation_type.mem.prev; listIterator_get_prev(&it) != NULL; ){
		mem_token_static_cursor = listIterator_get_data(it);
		if (mem_token_static_cursor->token.access != node_cursor){ /* suppression of memory STORE can lead to the suppression of a memory LOAD and to an incorrect list of token */
			listIterator_pop_next(&it);
			continue;
		}

		nb_frag = memTokenStatic_compare(mem_token_static, mem_token_static_cursor, ir_range_seed, local_access_frag_buffer);

		for (i = 0; i < nb_frag; i++){
			local_access_frag_buffer[i].type |= 0x00000001 << ((memTokenStatic_is_write(mem_token_static_cursor)) ? 1 : 0);
			if ((memTokenStatic_is_read(mem_token_static) && local_access_frag_buffer[i].type == IRMEMORY_ACCESS_ALIAS_READ) || (memTokenStatic_is_write(mem_token_static) && local_access_frag_buffer[i].type == IRMEMORY_ACCESS_ALIAS_WRITE)){
				continue;
			}

			accessFragment_merge(access_frag_buffer, local_access_frag_buffer + i);
		}

		node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.prev;

		if (accessFragment_is_complete(access_frag_buffer, memTokenStatic_get_size(mem_token_static))){
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
			log_err("unable to init memTokenStatic, further simplification might be incorrect -> STOP");
			break;
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
					if (accessFragment_build_compound(fragments + j, ir, &value, accessFragment_get(fragments + j, ir), memTokenStatic_get_size(&current_token) * 8)){
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

static int32_t memTokenDynamic_init(struct memTokenDynamic* mem_token_dyn, struct node* node){
	if (ir_node_get_operation(node)->operation_type.mem.con_addr == MEMADDRESS_INVALID){
		log_warn("a memory access has an incorrect concrete address, further simplifications might be incorrect");
		return -1;
	}

	if (ir_node_get_operation(node)->size > IRMEMORY_ACCESS_MAX_SIZE){
		log_warn_m("memory access size (%u) exceeds max limit (%u)", ir_node_get_operation(node)->size, IRMEMORY_ACCESS_MAX_SIZE);
		return -1;
	}

	mem_token_dyn->token.access = node;

	return 0;
}

static uint32_t memTokenDynamic_compare(struct memTokenDynamic* token1, struct memTokenDynamic* token2, struct accessFragment* access_frag_buffer){
	if (memTokenDynamic_get_conAddr(token1) < memTokenDynamic_get_conAddr(token2)){
		if (memTokenDynamic_get_conAddr(token2) - memTokenDynamic_get_conAddr(token1) < memTokenDynamic_get_size(token1)){
			access_frag_buffer[0].type 			= memTokenDynamic_is_write(token2) ? IRMEMORY_ACCESS_VALUE_WRITE : IRMEMORY_ACCESS_VALUE_READ;
			access_frag_buffer[0].token 		= memTokenDynamic_get_token(token2);
			access_frag_buffer[0].size 			= (uint16_t)(min(memTokenDynamic_get_conAddr(token1) + memTokenDynamic_get_size(token1) - memTokenDynamic_get_conAddr(token2), memTokenDynamic_get_size(token2)));
			access_frag_buffer[0].offset_src 	= (uint8_t)(memTokenDynamic_get_conAddr(token2) - memTokenDynamic_get_conAddr(token1));
			access_frag_buffer[0].offset_dst 	= 0;

			return 1;
		}
	}
	else{
		if (memTokenDynamic_get_conAddr(token1) - memTokenDynamic_get_conAddr(token2) < memTokenDynamic_get_size(token2)){
			access_frag_buffer[0].type 			= memTokenDynamic_is_write(token2) ? IRMEMORY_ACCESS_VALUE_WRITE : IRMEMORY_ACCESS_VALUE_READ;
			access_frag_buffer[0].token 		= memTokenStatic_get_token(token2);
			access_frag_buffer[0].size 			= (uint16_t)(min(memTokenDynamic_get_conAddr(token2) + memTokenDynamic_get_size(token2) - memTokenDynamic_get_conAddr(token1), memTokenDynamic_get_size(token1)));
			access_frag_buffer[0].offset_src 	= 0;
			access_frag_buffer[0].offset_dst 	= (uint8_t)(memTokenDynamic_get_conAddr(token1) - memTokenDynamic_get_conAddr(token2));

			return 1;
		}
	}

	return 0;
}

static void memTokenDynamic_search(struct memTokenDynamic* mem_token_dyn, struct list* token_list, struct accessFragment* access_frag_buffer){
	struct listIterator 	it;
	struct memTokenDynamic* mem_token_dyn_cursor;
	struct accessFragment 	local_access_frag_buffer[1];
	struct node* 			node_cursor;

	memset(access_frag_buffer, 0, sizeof(struct accessFragment) * IRMEMORY_ACCESS_MAX_NB_FRAGMENT);

	for (listIterator_init(&it, token_list), node_cursor = ir_node_get_operation(mem_token_dyn->token.access)->operation_type.mem.prev; listIterator_get_prev(&it) != NULL; ){
		mem_token_dyn_cursor = listIterator_get_data(it);
		if (mem_token_dyn_cursor->token.access != node_cursor){ /* suppression of memory STORE can lead to the suppression of a memory LOAD and to an incorrect list of token */
			listIterator_pop_next(&it);
			continue;
		}

		if (memTokenDynamic_compare(mem_token_dyn, mem_token_dyn_cursor, local_access_frag_buffer)){
			accessFragment_merge(access_frag_buffer, local_access_frag_buffer);
		}

		node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.prev;

		if (accessFragment_is_complete(access_frag_buffer, memTokenDynamic_get_size(mem_token_dyn))){
			break;
		}
	}
}

int32_t irMemory_simplify_concrete(struct ir* ir){
	uint32_t 				i;
	int32_t 				result;
	struct node* 			node_cursor;
	struct node* 			next_node_cursor;
	struct list 			mem_token_list;
	struct memTokenDynamic 	current_token;
	struct accessFragment 	fragments[IRMEMORY_ACCESS_MAX_NB_FRAGMENT];

	list_init(mem_token_list, sizeof(struct memTokenDynamic))

	for (node_cursor = irMemory_get_first(ir), result = 0; node_cursor != NULL; node_cursor = next_node_cursor){
		next_node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.next;

		if (memTokenDynamic_init(&current_token, node_cursor)){
			log_err("unable to init memTokenDynamic, further simplification might be incorrect");
			continue;
		}

		memTokenDynamic_search(&current_token, &mem_token_list, fragments);

		if (memTokenDynamic_is_read(&current_token)){
			struct node* value;
			struct node* insert_location;

			accessFragment_fill(fragments, memTokenDynamic_get_token(&current_token));

			for (i = 0, value = NULL, insert_location = node_cursor; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT && fragments[i].type != IRMEMORY_ACCESS_NONE; i++){
				if (fragments[i].type == IRMEMORY_ACCESS_NEW){
					struct node* 			new_mem_access;
					struct memTokenDynamic 	new_token;
					struct memTokenDynamic* new_token_ptr;

					if ((new_mem_access = accessFragment_create(fragments + i, ir, insert_location)) == NULL){
						log_err("unable to create accessFragment");
						continue;
					}

					insert_location = new_mem_access;

					if (memTokenDynamic_init(&new_token, new_mem_access)){
						log_err("unable to init memTokenDynamic, further simplification might be incorrect");
						continue;
					}

					if ((new_token_ptr = list_add_tail(&mem_token_list, &new_token)) == NULL){
						log_err("unable to add element to list");
						continue;
					}

					fragments[i].type 		= IRMEMORY_ACCESS_VALUE_READ;
					fragments[i].token 		= memTokenDynamic_get_token(new_token_ptr);
					fragments[i].offset_dst = 0;
				}

				if (accessFragment_build_compound(fragments + i, ir, &value, accessFragment_get(fragments + i, ir), memTokenDynamic_get_size(&current_token) * 8)){
					log_err("unable to build compound value");
				}
			}

			if (value == NULL){
				log_err("compound value is NULL, further simplification might be incorrect");
			}
			else if (value != node_cursor){
				ir_merge_equivalent_node(ir, value, node_cursor);
				result = 1;
			}
		}
		else{
			for (i = 0; i < IRMEMORY_ACCESS_MAX_NB_FRAGMENT && fragments[i].type != IRMEMORY_ACCESS_NONE; i++){
				if (fragments[i].type == IRMEMORY_ACCESS_VALUE_WRITE){
					if (fragments[i].offset_dst == 0 && fragments[i].size >= memToken_get_size(fragments[i].token)){
						ir_merge_equivalent_node(ir, node_cursor, fragments[i].token->access);
						list_remove(&mem_token_list, fragments[i].token);
						result = 1;
					}
				}
			}

			list_add_tail_check(&mem_token_list, &current_token)
		}
	}

	list_clean(&mem_token_list);

	return result;
}

struct accessEntry{
	uint64_t 		offset;
	uint32_t 		size;
	struct edge* 	access;
};

static int32_t accessEntry_compare_offset_and_type(void* element1, void* element2){
	struct accessEntry* entry1 = (struct accessEntry*)element1;
	struct accessEntry* entry2 = (struct accessEntry*)element2;
	struct irOperation* mem_access1;
	struct irOperation* mem_access2;

	mem_access1 = ir_node_get_operation(edge_get_dst(entry1->access));
	mem_access2 = ir_node_get_operation(edge_get_dst(entry2->access));

	if (mem_access1->type < mem_access2->type){
		return -1;
	}
	else if (mem_access1->type > mem_access2->type){
		return 1;
	}
	else{
		if (entry1->offset < entry2->offset){
			return -1;
		}
		else if (entry1->offset > entry2->offset){
			return 1;
		}
		else{
			return 0;
		}
	}
}

static int32_t accessEntry_compare_order(const void* element1, const void* element2){
	struct accessEntry* entry1 = *(struct accessEntry**)element1;
	struct accessEntry* entry2 = *(struct accessEntry**)element2;
	struct irOperation* mem_access1;
	struct irOperation* mem_access2;

	mem_access1 = ir_node_get_operation(edge_get_dst(entry1->access));
	mem_access2 = ir_node_get_operation(edge_get_dst(entry2->access));

	if (mem_access1->operation_type.mem.order < mem_access2->operation_type.mem.order){
		return -1;
	}
	else if (mem_access1->operation_type.mem.order > mem_access2->operation_type.mem.order){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t accessEntry_compare_offset(const void* element1, const void* element2){
	struct accessEntry* entry1 = *(struct accessEntry**)element1;
	struct accessEntry* entry2 = *(struct accessEntry**)element2;

	if (entry1->offset < entry2->offset){
		return -1;
	}
	else if (entry1->offset > entry2->offset){
		return 1;
	}
	else{
		return 0;
	}
}

#define IRVARIABLESIZE_GROUP_MAX_SIZE 4

struct accessGroup{
	uint32_t 			nb_entry;
	struct node* 		sched_node;
	struct node* 		addr_node;
	ADDRESS 			con_addr;
	struct accessEntry* entries[IRVARIABLESIZE_GROUP_MAX_SIZE];
};

#define accessGroup_is_read(group)  (ir_node_get_operation(edge_get_dst((group)->entries[0]->access))->type == IR_OPERATION_TYPE_IN_MEM)
#define accessGroup_is_write(group) (ir_node_get_operation(edge_get_dst((group)->entries[0]->access))->type == IR_OPERATION_TYPE_OUT_MEM)

#ifdef IR_FULL_CHECK
uint32_t accessGroup_check(const struct accessGroup* group){
	uint32_t 			i;
	struct irOperation* operation;
	uint32_t 			result = 0;
	uint32_t 			offset;

	for (i = 0, offset = 0; i < group->nb_entry; offset += group->entries[i++]->size){
		operation = ir_node_get_operation(edge_get_dst(group->entries[i]->access));

		if (operation->type != IR_OPERATION_TYPE_IN_MEM && operation->type != IR_OPERATION_TYPE_OUT_MEM){
			log_err("incorrect operation type");
			result = 1;
		}

		if (group->con_addr != MEMADDRESS_INVALID && operation->operation_type.mem.con_addr != MEMADDRESS_INVALID){
			if (group->con_addr + offset != operation->operation_type.mem.con_addr){
				log_err_m("found group with concrete address incoherence, expected " PRINTF_ADDR " but get " PRINTF_ADDR, group->con_addr + offset, operation->operation_type.mem.con_addr);
				result = 1;
			}
		}
	}

	return result;
}
#endif

void accessGroup_print(const struct accessGroup* group, FILE* file){
	uint32_t i;

	fputc('{', file);
	for (i = 0; i < group->nb_entry; i++){
		if (ir_node_get_operation(edge_get_dst(group->entries[i]->access))->type == IR_OPERATION_TYPE_IN_MEM){
			fputc('R', file);
		}
		else{
			fputc('W', file);
		}
		fprintf(file, ":0x%llx:%u:%p", group->entries[i]->offset, group->entries[i]->size, (void*)group->entries[i]);
		if (i != group->nb_entry - 1){
			fputc(' ', file);
		}
	}
	fputc('}', file);
}

static uint32_t accessGroup_check_aliasing(struct accessGroup* group, struct ir* ir){
	uint32_t 				i;
	struct memTokenStatic 	token;
	struct node* 			node_cursor;
	struct memTokenStatic 	token_cursor;
	struct accessFragment 	fragments[IRMEMORY_ACCESS_MAX_NB_FRAGMENT];

	qsort(group->entries, group->nb_entry, sizeof(struct accessEntry*), accessEntry_compare_order);

	if (accessGroup_is_read(group)){
		group->sched_node = edge_get_dst(group->entries[0]->access);
		for (i = 1; i < group->nb_entry; i++){
			if (memTokenStatic_init(&token, edge_get_dst(group->entries[i]->access))){
				log_err("unable to init memTokenStatic");
				return 0;
			}

			for (node_cursor = ir_node_get_operation(edge_get_dst(group->entries[i]->access))->operation_type.mem.prev; node_cursor != NULL; node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.prev){
				if (node_cursor == group->sched_node){
					break;
				}

				if (ir_node_get_operation(node_cursor)->type == IR_OPERATION_TYPE_OUT_MEM){
					if (memTokenStatic_init(&token_cursor, node_cursor)){
						log_err("unable to init memTokenStatic");
						return 0;
					}

					if (memTokenStatic_compare(&token_cursor, &token, ir->range_seed, fragments)){
						return 0;
					}
				}
			}
			#ifdef EXTRA_CHECK
			if (node_cursor == NULL){
				log_err("reached beginning of memory access list but first access was never reached");
			}
			#endif
		}
	}
	else{
		group->sched_node = edge_get_dst(group->entries[group->nb_entry - 1]->access);
		for (i = 0; i + 1 < group->nb_entry; i++){
			if (memTokenStatic_init(&token, edge_get_dst(group->entries[i]->access))){
				log_err("unable to init memTokenStatic");
				return 0;
			}

			for (node_cursor = ir_node_get_operation(edge_get_dst(group->entries[i]->access))->operation_type.mem.next; node_cursor != NULL; node_cursor = ir_node_get_operation(node_cursor)->operation_type.mem.next){
				if (node_cursor == group->sched_node){
					break;
				}

				if (memTokenStatic_init(&token_cursor, node_cursor)){
					log_err("unable to init memTokenStatic");
					return 0;
				}

				if (memTokenStatic_compare(&token_cursor, &token, ir->range_seed, fragments)){
					return 0;
				}
			}
			#ifdef EXTRA_CHECK
			if (node_cursor == NULL){
				log_err("reached end of memory access list but last access was never reached");
			}
			#endif
		}
	}

	return 1;
}

static enum irOpcode accessGroup_choose_part_opcode(uint32_t offset, uint32_t size){
	if (!offset){
		if (size == 1){
			return IR_PART1_8;
		}
		else if (size == 2){
			return IR_PART1_16;
		}
	}
	else if (size == 1){
		return IR_PART2_8;
	}

	log_err_m("this case is not handled yet (offset=%u, size=%u)", offset, size);
	return IR_INVALID;
}

static int32_t accessGroup_commit(struct accessGroup* group, struct ir* ir){
	uint32_t i;
	uint32_t offset;

	qsort(group->entries, group->nb_entry, sizeof(struct accessEntry*), accessEntry_compare_offset);

	if (accessGroup_is_read(group)){
		enum irOpcode 	opcode;
		struct node* 	attach_node[IRVARIABLESIZE_GROUP_MAX_SIZE];
		struct node* 	part_node;
		uint32_t 		nb_attach_node;
		struct node* 	imm_node;
		struct node* 	shl_node = NULL;

		for (i = 0, offset = 0, nb_attach_node = 0; i < group->nb_entry; offset += group->entries[i++]->size){
			if ((opcode = accessGroup_choose_part_opcode(offset & 0x00000001, group->entries[i]->size)) == IR_INVALID){
				log_err("unable to choose part opcode");
				continue;
			}

			if ((part_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, group->entries[i]->size * 8, opcode, IR_OPERATION_DST_UNKOWN)) == NULL){
				log_err("unable to add instruction to IR");
				continue;
			}

			graph_transfert_src_edge(&(ir->graph), part_node, edge_get_dst(group->entries[i]->access));

			if (offset >= 2){
				if (shl_node == NULL){
					shl_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_SHR, IR_OPERATION_DST_UNKOWN);
					if (shl_node == NULL){
						log_err("unable to add instruction to IR");
						return 0;
					}

					imm_node = ir_add_immediate(ir, 32, 16);
					if (imm_node == NULL){
						log_err("unable to add immediate to IR");
					}
					else{
						ir_add_dependence_check(ir, imm_node, shl_node, IR_DEPENDENCE_TYPE_SHIFT_DISP)
					}

					attach_node[nb_attach_node ++] = shl_node;
				}

				ir_add_dependence_check(ir, shl_node, part_node, IR_DEPENDENCE_TYPE_DIRECT)
			}
			else{
				attach_node[nb_attach_node ++] = part_node;
			}
		}

		for (i = 0; i < nb_attach_node; i++){
			ir_add_dependence_check(ir, group->sched_node, attach_node[i], IR_DEPENDENCE_TYPE_DIRECT)
		}
	}
	else{
		struct node* shl_node;
		struct node* exp_node;
		struct node* or_node;
		struct node* imm_node;
		struct edge* edge_cursor;

		if ((or_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_OR, IR_OPERATION_DST_UNKOWN)) == NULL){
			log_err("unable to add instruction to IR");
			return 0;
		}

		for (i = 0, offset = 0; i < group->nb_entry; offset += group->entries[i++]->size){
			for (edge_cursor = node_get_head_edge_dst(edge_get_dst(group->entries[i]->access)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
					break;
				}
			}
			if (edge_cursor == NULL){
				log_err("unable to locate direct operand");
				continue;
			}

			exp_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
			if (exp_node == NULL){
				log_err("unable to add instruction to IR");
			}
			else{
				ir_add_dependence_check(ir, edge_get_src(edge_cursor), exp_node, IR_DEPENDENCE_TYPE_DIRECT)
				if (offset){
					shl_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_SHL, IR_OPERATION_DST_UNKOWN);
					if (shl_node == NULL){
						log_err("unable to add instruction to IR");
					}
					else{
						imm_node = ir_add_immediate(ir, 32, offset * 8);
						if (imm_node == NULL){
							log_err("unable to add immediate to IR");
						}
						else{
							ir_add_dependence_check(ir, imm_node, shl_node, IR_DEPENDENCE_TYPE_SHIFT_DISP)
						}
						ir_add_dependence_check(ir, exp_node, shl_node, IR_DEPENDENCE_TYPE_DIRECT)
						exp_node = shl_node;
					}
				}

				ir_add_dependence_check(ir, exp_node, or_node, IR_DEPENDENCE_TYPE_DIRECT)
				ir_remove_dependence(ir, edge_cursor);
			}
		}

		ir_add_dependence_check(ir, or_node, group->sched_node, IR_DEPENDENCE_TYPE_DIRECT)
	}

	ir_node_get_operation(group->sched_node)->operation_type.mem.con_addr = group->con_addr;
	ir_node_get_operation(group->sched_node)->size = 32;

	ir_add_dependence_check(ir, group->addr_node, group->sched_node, IR_DEPENDENCE_TYPE_ADDRESS)

	for (i = 0; i < group->nb_entry; i++){
		if (edge_get_dst(group->entries[i]->access) != group->sched_node){
			ir_remove_node(ir, edge_get_dst(group->entries[i]->access));
		}
		else{
			ir_remove_dependence(ir, group->entries[i]->access);
		}
	}

	return 1;
}

struct accessTable{
	struct node* 	var;
	struct array* 	group_array;
	struct ir* 		ir;
	struct array 	access_array;
};

void accessTable_print(const struct accessTable* table, uint32_t* mapping, FILE* file){
	uint32_t 			i;
	struct accessEntry* entry;

	fputs("{", file);
	for (i = 0; i < array_get_length(&(table->access_array)); i++){
		if (mapping != NULL){
			entry = (struct accessEntry*)array_get(&(table->access_array), mapping[i]);
		}
		else{
			entry = (struct accessEntry*)array_get(&(table->access_array), i);
		}

		if (i == array_get_length(&(table->access_array)) - 1){
			fprintf(file, "%c:0x%llx:%u", (ir_node_get_operation(edge_get_dst(entry->access))->type == IR_OPERATION_TYPE_IN_MEM) ? 'R' : 'W', entry->offset, entry->size);
		}
		else{
			fprintf(file, "%c:0x%llx:%u ", (ir_node_get_operation(edge_get_dst(entry->access))->type == IR_OPERATION_TYPE_IN_MEM) ? 'R' : 'W', entry->offset, entry->size);
		}
	}
	fputs("}", file);
}

static uint32_t accessTable_generate_recursive_group(struct accessTable* table, const uint32_t* mapping, struct accessGroup* group, uint32_t offset_start, uint32_t nb_entry, uint32_t size){
	uint32_t i;

	for (i = offset_start; i < array_get_length(&(table->access_array)); i++){
		group->entries[nb_entry] = (struct accessEntry*)array_get(&(table->access_array), mapping[i]);
		if (ir_node_get_operation(edge_get_dst(group->entries[0]->access))->type != ir_node_get_operation(edge_get_dst(group->entries[nb_entry]->access))->type){
			continue;
		}
		if (group->entries[nb_entry - 1]->offset + group->entries[nb_entry - 1]->size != group->entries[nb_entry]->offset){
			continue;
		}

		if (group->entries[nb_entry]->size + size > 4){
			continue;
		}
		else if (group->entries[nb_entry]->size + size == 4){
			group->nb_entry = nb_entry + 1;

			#ifdef IR_FULL_CHECK
			if (accessGroup_check(group)){
				continue;
			}
			#endif

			if (accessGroup_check_aliasing(group, table->ir)){
				if (array_add(table->group_array, group) < 0){
					log_err("unable to add element to array");
				}
				else{
					return 1;
				}
			}
		}
		else if (nb_entry != IRVARIABLESIZE_GROUP_MAX_SIZE - 1){
			if (accessTable_generate_recursive_group(table, mapping, group, i + 1, nb_entry + 1, size + group->entries[nb_entry]->size)){
				return 1;
			}
		}
	}

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void accessTable_regroup(const void* data, const VISIT which, int32_t depth){
	struct accessTable* table = *(struct accessTable**)data;
	uint32_t* 			mapping;
	uint32_t 			i;
	struct accessGroup 	group;

	if (which != leaf && which != preorder){
		return;
	}

	if (array_get_length(&(table->access_array)) < 2){
		return;
	}

	mapping = array_create_mapping(&(table->access_array), accessEntry_compare_offset_and_type);
	if (mapping == NULL){
		log_err("unable to create mapping");
		return;
	}

	for (i = 0; i < array_get_length(&(table->access_array)); i++){
		group.entries[0] = (struct accessEntry*)array_get(&(table->access_array), mapping[i]);
		if (group.entries[0]->offset & 0x0000000000000003){
			continue;
		}

		#ifdef IR_FULL_CHECK
		{
			struct irOperation* op_mem_access = ir_node_get_operation(edge_get_dst(group.entries[0]->access));

			if (op_mem_access->operation_type.mem.con_addr != MEMADDRESS_INVALID && op_mem_access->operation_type.mem.con_addr & 0x0000000000000003){
				log_warn("memory access seems to be aligned but concrete address is not");
			}
		}
		#endif

		group.addr_node = edge_get_src(group.entries[0]->access);
		group.con_addr = ir_node_get_operation(edge_get_dst(group.entries[0]->access))->operation_type.mem.con_addr;
		if (accessTable_generate_recursive_group(table, mapping, &group, i + 1, 1, group.entries[0]->size)){
			break;
		}
	}

	free(mapping);
}

static int32_t accessTable_compare_var(const void* arg1, const void* arg2){
	const struct accessTable* table1 = (const struct accessTable*)arg1;
	const struct accessTable* table2 = (const struct accessTable*)arg2;

	if (table1->var < table2->var){
		return -1;
	}
	else if (table1->var > table2->var){
		return 1;
	}
	else{
		return 0;
	}
}

static void accessTable_delete(struct accessTable* table){
	array_clean(&(table->access_array));
	free(table);
}

int32_t irMemory_coalesce(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct edge*		edge_cursor;
	struct node*		var;
	struct node* 		imm;
	void* 				tree_root 			= NULL;
	struct accessTable* new_table 			= NULL;
	void* 				ptr;
	struct accessTable* table_ptr;
	struct accessEntry 	new_entry;
	struct array 		group_array;
	int32_t 			result 				= 0;

	if (array_init(&group_array, sizeof(struct accessGroup))){
		log_err("unable to init array");
		return 0;
	}

	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_dst != 2 || node_cursor->nb_edge_src == 0){
			continue;
		}

		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST || operation_cursor->operation_type.inst.opcode != IR_ADD){
			continue;
		}

		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
				break;
			}
		}
		if (edge_cursor == NULL){
			continue;
		}

		var = NULL;
		imm = NULL;

		edge_cursor = node_get_head_edge_dst(node_cursor);
		switch (ir_node_get_operation(edge_get_src(edge_cursor))->type){
			case IR_OPERATION_TYPE_IN_REG 	:
			case IR_OPERATION_TYPE_IN_MEM 	:
			case IR_OPERATION_TYPE_INST 	: {
				var = edge_get_src(edge_cursor);
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				imm = edge_get_src(edge_cursor);
				break;
			}
			default 						: {
				continue;
			}
		}

		edge_cursor = edge_get_next_dst(edge_cursor);
		switch (ir_node_get_operation(edge_get_src(edge_cursor))->type){
			case IR_OPERATION_TYPE_IN_REG 	:
			case IR_OPERATION_TYPE_IN_MEM 	:
			case IR_OPERATION_TYPE_INST 	: {
				if (var == NULL){
					var = edge_get_src(edge_cursor);
					break;
				}
				else{
					continue;
				}
			}
			case IR_OPERATION_TYPE_IMM 		: {
				if (imm == NULL){
					imm = edge_get_src(edge_cursor);
					break;
				}
				else{
					continue;
				}
			}
			default 						: {
				continue;
			}
		}

		if (new_table == NULL){
			new_table = (struct accessTable*)malloc(sizeof(struct accessTable));
			if (new_table == NULL){
				log_err("unable to allocate memory\n");
				continue;
			}
			else{
				if (array_init(&(new_table->access_array), sizeof(struct accessEntry))){
					log_err("unable to init array");
					free(new_table);
					new_table = NULL;
					continue;
				}
			}
		}

		new_table->var 			= var;
		new_table->group_array 	= &group_array;
		new_table->ir 			= ir;

		if ((ptr = tsearch(new_table, &tree_root, accessTable_compare_var)) == NULL){
			log_err("unable to allocate memory in tsearch");
			continue;
		}

		table_ptr = *(struct accessTable**)ptr;
		if (table_ptr == new_table){
			for (edge_cursor = node_get_head_edge_src(var); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){

					new_entry.offset 	= 0;
					new_entry.size 		= ir_node_get_operation(edge_get_dst(edge_cursor))->size / 8;
					new_entry.access 	= edge_cursor;

					if (array_add(&(table_ptr->access_array), &new_entry) < 0){
						log_err("unable to add element to array\n");
					}
				}
			}
			new_table = NULL;
		}

		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){

				new_entry.offset 	= ir_imm_operation_get_unsigned_value(ir_node_get_operation(imm));
				new_entry.size 		= ir_node_get_operation(edge_get_dst(edge_cursor))->size / 8;
				new_entry.access 	= edge_cursor;

				if (array_add(&(table_ptr->access_array), &new_entry) < 0){
					log_err("unable to add element to array\n");
				}
			}
		}
	}

	if (new_table != NULL){
		accessTable_delete(new_table);
	}

	twalk(tree_root, accessTable_regroup);

	if (array_get_length(&group_array) > 0){
		uint32_t 			i;
		struct accessGroup* group;

		for (i = 0; i < array_get_length(&group_array); i++){
			group = (struct accessGroup*)array_get(&group_array, i);
			result |= accessGroup_commit(group, ir);
		}
	}

	array_clean(&group_array);

	tdestroy(tree_root, (void(*)(void*))accessTable_delete);

	return result;
}
