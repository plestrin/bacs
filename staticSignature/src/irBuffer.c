#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irBuffer.h"
#include "dijkstra.h"
#include "base.h"
#include "array.h"

#define IRBUFFER_MIN_SIZE 64

struct memAccess{
	struct node* 	node;
	uint32_t 		index;
};

static int32_t compare_memAccess_address_then_order(void* arg1, void* arg2){
	struct memAccess* 		mem_access1 = (struct memAccess*)arg1;
	struct memAccess* 		mem_access2 = (struct memAccess*)arg2;
	struct irOperation* 	op1 = ir_node_get_operation(mem_access1->node);
	struct irOperation* 	op2 = ir_node_get_operation(mem_access2->node);

	if (op1->operation_type.mem.con_addr < op2->operation_type.mem.con_addr){
		return -1;
	}
	else if (op1->operation_type.mem.con_addr > op2->operation_type.mem.con_addr){
		return 1;
	}
	else if (op1->operation_type.mem.order > op2->operation_type.mem.order){
		return -1;
	}
	else if (op1->operation_type.mem.order < op2->operation_type.mem.order){
		return 1;
	}
	else{
		return 0;
	}
}

struct memoryBuffer{
	ADDRESS 			address_start;
	uint32_t 			size;
	uint32_t 			nb_access;
	struct memAccess* 	access;
};

#define memoryBuffer_get_size(nb_access) (sizeof(struct memoryBuffer) + sizeof(struct memAccess) * (nb_access))

struct subMemoryBuffer{
	struct memoryBuffer* 	buffer;
	uint32_t 				offset;
	uint32_t 				length;
	uint32_t 				size;
};

struct bufferCouple{
	struct subMemoryBuffer in;
	struct subMemoryBuffer ou;
};

static inline struct memoryBuffer* memoryBuffer_create(uint32_t nb_access){
	struct memoryBuffer* mem_buf;

	if ((mem_buf = (struct memoryBuffer*)malloc(memoryBuffer_get_size(nb_access))) != NULL){
		mem_buf->nb_access = nb_access;
		mem_buf->access = (struct memAccess*)(mem_buf + 1);
	}
	else{
		log_err("unable to allocate memory");
	}

	return mem_buf;
}

static struct array* memoryBuffer_create_array(struct array* mem_access_array, uint32_t is_write){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	struct array* 			buffer_array;
	uint32_t* 				mapping 		= NULL;
	ADDRESS 				address;
	uint32_t 				size;
	struct irOperation* 	operation;
	struct memoryBuffer* 	mem_buf;
	uint32_t 				nb_mem_access;

	buffer_array = array_create(sizeof(struct memoryBuffer*));
	if (buffer_array == NULL){
		log_err("unable to create array");
		return NULL;
	}

	mapping = array_create_mapping(mem_access_array, compare_memAccess_address_then_order);
	if (mapping == NULL){
		log_err("unable to create mapping");
		return buffer_array;
	}

	if (is_write){
		operation = ir_node_get_operation(((struct memAccess*)array_get(mem_access_array, mapping[0]))->node);
		address = operation->operation_type.mem.con_addr;
		for (i = 1, j = 1; i < array_get_length(mem_access_array); i++){
			operation = ir_node_get_operation(((struct memAccess*)array_get(mem_access_array, mapping[i]))->node);
			if (operation->operation_type.mem.con_addr != address){
				if (i != j){
					mapping[j] = mapping[i];
				}
				j++;
			}
			address = operation->operation_type.mem.con_addr;
		}
		nb_mem_access = j;
	}
	else{
		nb_mem_access = array_get_length(mem_access_array);
	}

	for (i = 0; i < nb_mem_access; i = j){
		operation = ir_node_get_operation(((struct memAccess*)array_get(mem_access_array, mapping[i]))->node);
		address = operation->operation_type.mem.con_addr;
		size = operation->size;
		for (j = i + 1; j < nb_mem_access; j++){
			operation = ir_node_get_operation(((struct memAccess*)array_get(mem_access_array, mapping[j]))->node);
			if (operation->operation_type.mem.con_addr < address + (size / 8)){
				log_warn_m("found and overlapping memory access (" PRINTF_ADDR ", " PRINTF_ADDR "). Make sure you have done a concrete memory simplification", address + (size / 8), operation->operation_type.mem.con_addr);
			}
			else if (operation->operation_type.mem.con_addr > address + (size / 8)){
				break;
			}
			else{
				size += operation->size;
			}
		}

		if (size >= IRBUFFER_MIN_SIZE){
			if ((mem_buf = memoryBuffer_create(j - i)) != NULL){
				mem_buf->address_start 	= address;
				mem_buf->size 			= size;
				for (k = i; k < j; k++){
					memcpy(mem_buf->access + k - i, array_get(mem_access_array, mapping[k]), sizeof(struct memAccess));
				}

				if (array_add(buffer_array, &mem_buf) < 0){
					log_err("unable to add element to array");
					free(mem_buf);
				}
			}
			else{
				log_err("unable to create memory buffer");
			}
		}
	}

	free(mapping);

	return buffer_array;
}

static uint32_t* memoryBuffer_get_dst_from(const struct memoryBuffer* mem_buf, struct graph* graph){
	uint32_t* 	result;
	uint32_t 	i;

	result = (uint32_t*)malloc(mem_buf->nb_access * graph->nb_node * sizeof(uint32_t));
	if (result != NULL){
		for (i = 0; i < mem_buf->nb_access; i++){
			if (dijkstra_dst_from(graph, mem_buf->access[i].node, result + graph->nb_node * i)){
				log_err("unable to compute distance from");
				free(result);
				result = NULL;
				break;
			}
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return result;
}

static void memoryBuffer_search_dependence(struct memoryBuffer* in_buf, struct memoryBuffer* ou_buf, const uint32_t* dst_from, struct graph* graph, struct array* buffer_couple_array){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	uint32_t 			l;
	uint32_t 			m;
	uint32_t 			n;
	uint32_t 			ou_size;
	uint32_t 			in_size;
	struct bufferCouple couple;

	for (i = 0; i < in_buf->nb_access; i++){
		for (j = 0; j < ou_buf->nb_access; j += k + 1){
			for (k = 0, ou_size = 0; j + k < ou_buf->nb_access && dst_from[i * graph->nb_node + ou_buf->access[j + k].index] != DIJKSTRA_INVALID_DST; k++){
				ou_size += ir_node_get_operation(ou_buf->access[j + k].node)->size;
			}
			if (ou_size < IRBUFFER_MIN_SIZE){
				continue;
			}

			for (l = i + 1, in_size = ir_node_get_operation(in_buf->access[i].node)->size, n = k; l < in_buf->nb_access; l++){
				for (m = 0, ou_size = 0; m < n && dst_from[l * graph->nb_node + ou_buf->access[j + m].index] != DIJKSTRA_INVALID_DST; m++){
					ou_size += ir_node_get_operation(ou_buf->access[j + m].node)->size;
				}
				if (ou_size >= IRBUFFER_MIN_SIZE){
					in_size += ir_node_get_operation(in_buf->access[l].node)->size;
					n = min(n, m);
				}
				else{
					break;
				}
			}
			if (in_size >= IRBUFFER_MIN_SIZE){
				couple.in.buffer 	= in_buf;
				couple.in.offset 	= i;
				couple.in.length 	= l -i;
				couple.in.size 		= in_size;
				couple.ou.buffer 	= ou_buf;
				couple.ou.offset 	= j;
				couple.ou.length 	= n;
				couple.ou.size 		= ou_size;
				if (array_add(buffer_couple_array, &couple) < 0){
					log_err("unable to add element to array");
				}
			}
		}
	}
}

static void memoryBuffer_delete_array(struct array* buffer_array){
	uint32_t i;

	for (i = 0; i < array_get_length(buffer_array); i++){
		free(*(struct memoryBuffer**)array_get(buffer_array, i));
	}
	array_delete(buffer_array);

}

void subMemoryBuffer_print(struct subMemoryBuffer* sub_mem_buf){
	struct irOperation* op1 = ir_node_get_operation(sub_mem_buf->buffer->access[sub_mem_buf->offset].node);
	struct irOperation* op2 = ir_node_get_operation(sub_mem_buf->buffer->access[sub_mem_buf->offset + sub_mem_buf->offset - 1].node);

	printf("[" PRINTF_ADDR ", " PRINTF_ADDR "] size=%3u", op1->operation_type.mem.con_addr, op2->operation_type.mem.con_addr, sub_mem_buf->size);
}

static int32_t bufferCouple_compare_redundant(void* arg1, void* arg2){
	struct bufferCouple* 	c1 = (struct bufferCouple*)arg1;
	struct bufferCouple* 	c2 = (struct bufferCouple*)arg2;
	int32_t 				result;

	result = memcmp(&(c1->ou), &(c2->ou), sizeof(struct subMemoryBuffer));
	if (result != 0){
		return result;
	}
	else{
		if (c1->in.buffer < c2->in.buffer){
			return -1;
		}
		else if (c1->in.buffer > c2->in.buffer){
			return 1;
		}
		else if (c1->in.offset + c1->in.length < c2->in.offset + c2->in.length){
			return -1;
		}
		else if (c1->in.offset + c1->in.length > c2->in.offset + c2->in.length){
			return 1;
		}
		else if (c1->in.offset < c2->in.offset){
			return -1;
		}
		else if (c1->in.offset > c2->in.offset){
			return 1;
		}
		else{
			return 0;
		}
	}
}

void bufferCouple_print(struct bufferCouple* couple){
	subMemoryBuffer_print(&(couple->in));
	fputs(" -> ", stdout);
	subMemoryBuffer_print(&(couple->ou));
}

/* Warning: from here it is an awful hack */

#include "codeSignature.h"
#include "irNormalize.h"

static struct codeSignature sha1_compress = {
	.signature 			= {
		.id 			= 42, /*lol*/
		.name 			= "sha1_compress",
		.symbol 		= "compress",
		.graph 			= {0},
		.sub_graph_handle = NULL,
		.symbol_table 	= NULL,
		.result_index 	= 0,
		.state 			= 0,
	},
	.nb_parameter_in 	= 1,
	.nb_parameter_out 	= 1,
	.nb_frag_tot_in 	= 16,
	.nb_frag_tot_out 	= 5
};

static uint32_t sha_is_init = 0;

static void push_sha1_signature(struct bufferCouple* couple, struct ir* ir){
	struct node* 	symbol;
	uint32_t 		i;
	struct edge* 	new_edge;
	struct edge* 	cur_edge;
	struct edge* 	nex_edge;
	struct node* 	parameter;

	if (!sha_is_init){
		struct codeSignatureNode node;


		node.type = CODESIGNATURE_NODE_TYPE_OPCODE;
		node.node_type.opcode = IR_INVALID;

		graph_init(&(sha1_compress.signature.graph), sizeof(struct codeSignatureNode), sizeof(struct codeSignatureEdge));

		node.input_number 		= 1;
		node.input_frag_order 	= 1;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 2;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 3;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 4;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 5;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 6;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 7;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 8;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 9;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 10;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 11;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 12;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 13;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 14;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 15;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 1;
		node.input_frag_order 	= 16;
		node.output_number 		= 0;
		node.output_frag_order 	= 0;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 0;
		node.input_frag_order 	= 0;
		node.output_number 		= 1;
		node.output_frag_order 	= 1;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 0;
		node.input_frag_order 	= 0;
		node.output_number 		= 1;
		node.output_frag_order 	= 2;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 0;
		node.input_frag_order 	= 0;
		node.output_number 		= 1;
		node.output_frag_order 	= 3;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 0;
		node.input_frag_order 	= 0;
		node.output_number 		= 1;
		node.output_frag_order 	= 4;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		node.input_number 		= 0;
		node.input_frag_order 	= 0;
		node.output_number 		= 1;
		node.output_frag_order 	= 5;

		graph_add_node(&(sha1_compress.signature.graph), &node);

		sha1_compress.signature.sub_graph_handle = graphIso_create_sub_graph_handle(&(sha1_compress.signature.graph), codeSignatureNode_get_label, codeSignatureEdge_get_label);
		sha_is_init = 1;
	}

	symbol = ir_add_symbol(ir, &sha1_compress, NULL, 0);
	if (symbol != NULL){
		for (i = 0; i < couple->in.length; i++){
			parameter = couple->in.buffer->access[couple->in.offset + i].node;
			ir_add_macro_dependence(ir, parameter, symbol, IR_DEPENDENCE_MACRO_DESC_SET_INPUT(i + 1, 1));
			/*for (cur_edge = node_get_head_edge_src(parameter); cur_edge != NULL; cur_edge = nex_edge){
				nex_edge = edge_get_next_src(cur_edge);
				if (cur_edge != new_edge){
					ir_remove_dependence(ir, cur_edge);
				}
			}*/
		}
		for (i = 0; i < couple->ou.length; i++){
			for (nex_edge = node_get_head_edge_dst(couple->ou.buffer->access[couple->ou.offset + i].node); nex_edge != NULL; nex_edge = edge_get_next_dst(nex_edge)){
				if (ir_edge_get_dependence(nex_edge)->type == IR_DEPENDENCE_TYPE_DIRECT){
					break;
				}
			}
			if (nex_edge == NULL){
				log_err("unable to find memory write value");
				continue;
			}

			parameter = edge_get_src(nex_edge);
			new_edge = ir_add_macro_dependence(ir, symbol, parameter, IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(i + 1, 1));
			for (cur_edge = node_get_head_edge_dst(parameter); cur_edge != NULL; cur_edge = nex_edge){
				nex_edge = edge_get_next_dst(cur_edge);
				if (cur_edge != new_edge){
					ir_remove_dependence(ir, cur_edge);
				}
			}
		}
	}
	else{
		log_err("unable to add symbol to IR");
	}
}

/* end of the awful section */

void ir_search_buffer(struct ir* ir){
	uint32_t 				i;
	uint32_t 				j;
	struct node* 			node_cursor;
	struct irOperation* 	operation;
	struct array* 			read_mem_access_array 	= NULL;
	struct array* 			write_mem_access_array 	= NULL;
	struct array* 			buf_in_array 			= NULL;
	struct array* 			buf_ou_array 			= NULL;
	struct memAccess 		mem_access;
	struct array* 			couple_array 			= NULL;
	uint32_t* 				filter_couple 			= NULL;
	/*uint32_t 				nb_couple;*/

	read_mem_access_array = array_create(sizeof(struct memAccess));
	write_mem_access_array = array_create(sizeof(struct memAccess));

	if (read_mem_access_array == NULL || write_mem_access_array == NULL){
		log_err("unable to create array");
		goto exit;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		operation = ir_node_get_operation(node_cursor);

		mem_access.node = node_cursor;
		mem_access.index = i;

		switch(operation->type){
			case IR_OPERATION_TYPE_IN_MEM 	: {
				if (operation->operation_type.mem.con_addr == MEMADDRESS_INVALID){
					log_warn("concrete memory address is unspecified, final results might be incorrect");
				}
				else if (array_add(read_mem_access_array, &mem_access) < 0){
					log_err("unable to add element to array");
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM 	: {
				if (operation->operation_type.mem.con_addr == MEMADDRESS_INVALID){
					log_warn("concrete memory address is unspecified, final results might be incorrect");
				}
				else if (array_add(write_mem_access_array, &mem_access) < 0){
					log_err("unable to add element to array");
				}
				break;
			}
			default 						: {
				break;
			}
		}
	}

	if (array_get_length(read_mem_access_array) == 0 || array_get_length(write_mem_access_array) == 0){
		goto exit;
	}

	buf_in_array = memoryBuffer_create_array(read_mem_access_array, 0);
	buf_ou_array = memoryBuffer_create_array(write_mem_access_array, 1);

	if (buf_in_array == NULL || buf_ou_array == NULL){
		log_err("unable to create memoryBuffer array");
		goto exit;
	}

	array_delete(read_mem_access_array);
	read_mem_access_array = NULL;
	array_delete(write_mem_access_array);
	write_mem_access_array = NULL;

	#ifdef VERBOSE
	log_info_m("%u input buffer(s) and %u output buffer(s)", array_get_length(buf_in_array), array_get_length(buf_ou_array));
	#endif

	if ((couple_array = array_create(sizeof(struct bufferCouple))) == NULL){
		log_err("unable to create array");
		goto exit;
	}

	for (i = 0; i < array_get_length(buf_in_array); i++){
		uint32_t* 				dst_from;
		struct memoryBuffer* 	in_ptr;
		struct memoryBuffer* 	ou_ptr;

		in_ptr = *(struct memoryBuffer**)array_get(buf_in_array, i);
		if ((dst_from = memoryBuffer_get_dst_from(in_ptr, &(ir->graph))) != NULL){
			for (j = 0; j < array_get_length(buf_ou_array); j++){
				ou_ptr = *(struct memoryBuffer**)array_get(buf_ou_array, j);
				memoryBuffer_search_dependence(in_ptr, ou_ptr, dst_from, &(ir->graph), couple_array);
			}
			free(dst_from);
		}
		else{
			log_err("unable to compute distance from");
		}
	}

	if (array_get_length(couple_array) == 0){
		goto exit;
	}

	if ((filter_couple = array_create_mapping(couple_array, bufferCouple_compare_redundant)) == NULL){
		log_err("unable to create mapping");
		goto exit;
	}

#if 0
	for (i = 1, j = 1; i < array_get_length(couple_array); i++){
		struct bufferCouple* c1;
		struct bufferCouple* c2;

		c1 = (struct bufferCouple*)array_get(couple_array, filter_couple[i - 1]);
		c2 = (struct bufferCouple*)array_get(couple_array, filter_couple[i]);
		
		if (memcmp(&(c1->ou), &(c2->ou), sizeof(struct subMemoryBuffer)) != 0 || c1->in.buffer != c2->ou.buffer || c1->in.offset + c1->in.length != c2->in.offset + c2->in.length){
			if (i != j){
				filter_couple[]
			}

		}
	}
#endif

	/* Warning: from here it is an awful hack */
	{
		#define HACK_IN_SIZE 512
		#define HACK_OU_SIZE 160
		#define HACK_IN_NB_FRAG 16
		#define HACK_OU_NB_FRAG 5
		#define HACK_COUPLE_ID 0

		uint32_t 				nb_good_couple;
		struct bufferCouple* 	couple;

		for (i = 0, nb_good_couple = 0; i < array_get_length(couple_array); i++){
			couple = (struct bufferCouple*)array_get(couple_array, i);
			if (couple->in.size == HACK_IN_SIZE && couple->in.length == HACK_IN_NB_FRAG && couple->ou.size == HACK_OU_SIZE && couple->ou.length == HACK_OU_NB_FRAG){
				if (nb_good_couple == HACK_COUPLE_ID){
					log_debug("found good couple");
					push_sha1_signature(couple, ir);
					log_debug("signature have been pushed");
					ir_normalize_remove_dead_code(ir, NULL); /* is it necessary ?? I don't think so */
				}
				else{
					log_debug("... but other good couple exist!!");
				}
				nb_good_couple ++;
			}
		}
	}
	/* end of the awful section */


#if 0
	for (i = 0; i < nb_couple; i++){
		bufferCouple_print((struct bufferCouple*)array_get(couple_array, filter_couple[i])); putchar('\n');
	}
#endif

	exit:
	if (filter_couple != NULL){
		free(filter_couple);
	}
	if (couple_array != NULL){
		array_delete(couple_array);
	}
	if (buf_in_array != NULL){
		memoryBuffer_delete_array(buf_in_array);
	}
	if (buf_ou_array != NULL){
		memoryBuffer_delete_array(buf_ou_array);
	}
	if (read_mem_access_array != NULL){
		array_delete(read_mem_access_array);
	}
	if (write_mem_access_array != NULL){
		array_delete(write_mem_access_array);
	}
}