#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irBuffer.h"
#include "signatureCollection.h"
#include "dijkstra.h"
#include "array.h"
#include "set.h"
#include "multiColumn.h"
#include "base.h"


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

enum bufferCoupleCompare{
	BUFFERCOUPLECOMPARE_SUBSET,
	BUFFERCOUPLECOMPARE_SUPERSET,
	BUFFERCOUPLECOMPARE_NONE
};

static enum bufferCoupleCompare bufferCouple_compare_subset_superset(const struct bufferCouple* couple1, const struct bufferCouple* couple2);

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
				log_warn_m("found an overlapping memory access (" PRINTF_ADDR ", " PRINTF_ADDR ":%u). Make sure you have done a concrete memory simplification", address + (size / 8), operation->operation_type.mem.con_addr, operation->size / 8);
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
				return NULL;
			}
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return result;
}

static void memoryBuffer_search_dependence(struct memoryBuffer* in_buf, struct memoryBuffer* ou_buf, const uint32_t* dst_from, struct graph* graph, struct set* buffer_couple_set){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	uint32_t 					l;
	uint32_t 					m;
	uint32_t 					n;
	uint32_t 					ou_size;
	uint32_t 					in_size;
	struct bufferCouple 		new_couple;
	struct bufferCouple* 		set_couple;
	struct subSet 				buffer_couple_sub_set;
	struct setIterator 			it;

	new_couple.in.buffer = in_buf;
	new_couple.ou.buffer = ou_buf;

	subSet_init(&buffer_couple_sub_set, buffer_couple_set);

	for (i = 0; i < in_buf->nb_access; i ++){
		for (j = 0; j < ou_buf->nb_access; j ++){
			for (k = 0, ou_size = 0; j + k < ou_buf->nb_access && dst_from[i * graph->nb_node + ou_buf->access[j + k].index] != DIJKSTRA_INVALID_DST; k++){
				ou_size += ir_node_get_operation(ou_buf->access[j + k].node)->size;
			}
			if (ou_size < IRBUFFER_MIN_SIZE){
				continue;
			}

			for (l = i + 1, in_size = ir_node_get_operation(in_buf->access[i].node)->size, n = k; l < in_buf->nb_access; l ++, n = m){
				for (m = 0, ou_size = 0; m < n && dst_from[l * graph->nb_node + ou_buf->access[j + m].index] != DIJKSTRA_INVALID_DST; m ++){
					ou_size += ir_node_get_operation(ou_buf->access[j + m].node)->size;
				}

				if (ou_size >= IRBUFFER_MIN_SIZE){
					in_size += ir_node_get_operation(in_buf->access[l].node)->size;

					if (in_size >= IRBUFFER_MIN_SIZE){
						new_couple.in.offset 	= i;
						new_couple.in.length 	= l + 1 - i;
						new_couple.in.size 		= in_size;
						new_couple.ou.offset 	= j;
						new_couple.ou.length 	= m;
						new_couple.ou.size 		= ou_size;

						for (set_couple = subSetIterator_get_first(&buffer_couple_sub_set, &it); set_couple != NULL; set_couple = setIterator_get_next(&it)){
							switch (bufferCouple_compare_subset_superset(&new_couple, set_couple)){
								case BUFFERCOUPLECOMPARE_SUPERSET 	: {
									setIterator_pop(&it);
									break;
								}
								case BUFFERCOUPLECOMPARE_SUBSET 	: {
									goto next;
								}
								case BUFFERCOUPLECOMPARE_NONE 		: {
									break;
								}
							}
						}

						if (subSet_add(&buffer_couple_sub_set, &new_couple) < 0){
							log_err("unable to add element to set");
						}

						next:;
					}
				}
				else{
					break;
				}
			}
		}
	}
}

void memoryBuffer_print(const struct memoryBuffer* mem_buf){
	printf("[" PRINTF_ADDR ", " PRINTF_ADDR "] size=%3u", mem_buf->address_start, mem_buf->address_start + mem_buf->size / 8, mem_buf->size);
}

static void memoryBuffer_delete_array(struct array* buffer_array){
	uint32_t i;

	for (i = 0; i < array_get_length(buffer_array); i++){
		free(*(struct memoryBuffer**)array_get(buffer_array, i));
	}
	array_delete(buffer_array);

}

static inline ADDRESS subMemoryBuffer_get_start_address(const struct subMemoryBuffer* sub_mem_buf){
	struct irOperation* op = ir_node_get_operation(sub_mem_buf->buffer->access[sub_mem_buf->offset].node);
	return op->operation_type.mem.con_addr;
}

static inline ADDRESS subMemoryBuffer_get_stop_address(const struct subMemoryBuffer* sub_mem_buf){
	struct irOperation* op = ir_node_get_operation(sub_mem_buf->buffer->access[sub_mem_buf->offset + sub_mem_buf->length - 1].node);
	return op->operation_type.mem.con_addr + op->size / 8;
}

void subMemoryBuffer_print(const struct subMemoryBuffer* sub_mem_buf){
	printf("[" PRINTF_ADDR ", " PRINTF_ADDR "] size=%3u", subMemoryBuffer_get_start_address(sub_mem_buf), subMemoryBuffer_get_stop_address(sub_mem_buf), sub_mem_buf->size);
}

static void subMemoryBuffer_remove_edge_footprint(const struct subMemoryBuffer* sub_mem_buf, struct ir* ir){
	uint32_t 		i;
	struct edge* 	cur_edge;
	struct edge* 	nex_edge;

	for (i = 0; i < sub_mem_buf->length; i++){
		for (nex_edge = node_get_head_edge_dst(sub_mem_buf->buffer->access[sub_mem_buf->offset + i].node); nex_edge != NULL; nex_edge = edge_get_next_dst(nex_edge)){
			if (ir_edge_get_dependence(nex_edge)->type == IR_DEPENDENCE_TYPE_DIRECT){
				break;
			}
		}
		if (nex_edge == NULL){
			log_err("unable to find memory write value");
			continue;
		}

		for (cur_edge = node_get_head_edge_dst(edge_get_src(nex_edge)); cur_edge != NULL; cur_edge = nex_edge){
			nex_edge = edge_get_next_dst(cur_edge);
			if (ir_edge_get_dependence(cur_edge)->type != IR_DEPENDENCE_TYPE_MACRO){
				ir_remove_dependence(ir, cur_edge);
			}
		}
	}
}

static enum bufferCoupleCompare bufferCouple_compare_subset_superset(const struct bufferCouple* couple1, const struct bufferCouple* couple2){
	if (couple1->in.buffer != couple2->in.buffer || couple1->ou.buffer != couple2->ou.buffer){
		return BUFFERCOUPLECOMPARE_NONE;
	}

	if (couple1->in.offset <= couple2->in.offset && couple1->in.offset + couple1->in.length >= couple2->in.offset + couple2->in.length){
		if (couple1->ou.offset <= couple2->ou.offset && couple1->ou.offset + couple1->ou.length >= couple2->ou.offset + couple2->ou.length){
			return BUFFERCOUPLECOMPARE_SUPERSET;
		}
	}

	if (couple2->in.offset <= couple1->in.offset && couple2->in.offset + couple2->in.length >= couple1->in.offset + couple1->in.length){
		if (couple2->ou.offset <= couple1->ou.offset && couple2->ou.offset + couple2->ou.length >= couple1->ou.offset + couple1->ou.length){
			return BUFFERCOUPLECOMPARE_SUBSET;
		}
	}

	return BUFFERCOUPLECOMPARE_NONE;
}

void bufferCouple_print(const struct bufferCouple* couple){
	subMemoryBuffer_print(&(couple->in));
	fputs(" -> ", stdout);
	subMemoryBuffer_print(&(couple->ou));
}

#define BUFFERSIGNATURE_NB_MAX_INPUT 4

struct bufferSignature{
	struct signatureSymbol 	symbol;
	char 					export_name[SIGNATURE_NAME_MAX_SIZE];
	uint32_t 				nb_in;
	uint32_t 				in_size[BUFFERSIGNATURE_NB_MAX_INPUT];
	uint32_t 				ou_size;
};

static struct bufferSignature aes128 = {
	.symbol 		= {
		.name 		= "aes128",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {128,1408},
	.ou_size 		= 128
};

static struct bufferSignature aes196 = {
	.symbol 		= {
		.name 		= "aes196",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {128,1664},
	.ou_size 		= 128
};

static struct bufferSignature aes256 = {
	.symbol 		= {
		.name 		= "aes256",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {128,1920},
	.ou_size 		= 128
};

static struct bufferSignature aes128_ks = {
	.symbol 		= {
		.name 		= "aes_ks_128",
	},
	.export_name 	= "ks",
	.nb_in 			= 1,
	.in_size 		= {128},
	.ou_size 		= 1280
};

static struct bufferSignature md5_compress = {
	.symbol 		= {
		.name 		= "md5_compress",
	},
	.export_name 	= "compress",
	.nb_in 			= 2,
	.in_size 		= {128,512},
	.ou_size 		= 128
};

static struct bufferSignature sha1_compress = {
	.symbol 		= {
		.name 		= "sha1_compress",
	},
	.export_name 	= "compress",
	.nb_in 			= 2,
	.in_size 		= {160,512},
	.ou_size 		= 160
};

static struct bufferSignature xtea = {
	.symbol 		= {
		.name 		= "xtea_32",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {64,128},
	.ou_size 		= 64
};

static struct bufferSignature* const buffer_signature_buffer[] = {
	&aes128,
	&aes196,
	&aes256,
	&aes128_ks,
	&md5_compress,
	&sha1_compress,
	&xtea,
	NULL
};

static uint32_t are_signatures_registered;

static void bufferSignature_register_buffer(void){
	uint32_t i;

	for (i = 0; buffer_signature_buffer[i] != NULL; i++){
		signatureSymbol_register(&(buffer_signature_buffer[i]->symbol), buffer_signature_buffer[i]->export_name, NULL);
	}

	are_signatures_registered = 1;
}

static int32_t bufferSignature_push(struct bufferSignature* buffer_signature, struct subMemoryBuffer** in, struct subMemoryBuffer** ou, uint32_t nb_in, struct ir* ir){
	struct node* 	symbol;
	uint32_t 		i;
	uint32_t 		j;
	struct edge* 	edge_cursor;

	if ((symbol = ir_add_symbol(ir, &(buffer_signature->symbol))) == NULL){
		log_err("unable to add symbol to IR");
		return -1;
	}

	for (i = 0; i < nb_in; i++){
		for (j = 0; j < in[i]->length; j++){
			if (ir_add_macro_dependence(ir, in[i]->buffer->access[in[i]->offset + j].node, symbol, IR_DEPENDENCE_MACRO_DESC_SET_INPUT(j + 1, i + 1)) == NULL){
				log_err("unable to add macro dependence to IR");
			}
		}
	}

	for (i = 0; i < ou[0]->length; i++){
		for (edge_cursor = node_get_head_edge_dst(ou[0]->buffer->access[ou[0]->offset + i].node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
				break;
			}
		}
		if (edge_cursor == NULL){
			log_err("unable to find memory write value");
			continue;
		}

		if (ir_add_macro_dependence(ir, symbol, edge_get_src(edge_cursor), IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(i + 1, 1)) == NULL){
			log_err("unable to add macro dependence to IR");
		}
	}

	return 0;
}

static uint32_t bufferSignature_recursive_match(struct bufferSignature* buffer_signature, struct set* couple_set, struct subMemoryBuffer** in, struct subMemoryBuffer** ou, uint32_t nb_in, struct ir* ir, struct subMemoryBuffer** remove_footprint, struct array* prim_para_array){
	uint32_t 					i;
	struct setIterator 			it;
	uint32_t 					result;
	struct bufferCouple* 		couple;
	struct primitiveParameter* 	match;

	for (couple = setIterator_get_first(couple_set, &it), result = 0; couple != NULL; couple = setIterator_get_next(&it)){
		if (!nb_in){
			if (couple->in.size != buffer_signature->in_size[0] || couple->ou.size != buffer_signature->ou_size){
				continue;
			}

			in[0] = &couple->in;
			ou[0] = &couple->ou;
		}
		else{
			if (couple->in.size != buffer_signature->in_size[nb_in] || memcmp(&couple->ou, ou[0], sizeof(struct subMemoryBuffer))){
				continue;
			}

			in[nb_in] = &couple->in;
		}

		if (nb_in + 1 < buffer_signature->nb_in){
			result += bufferSignature_recursive_match(buffer_signature, couple_set, in, ou, nb_in + 1, ir, remove_footprint, prim_para_array);
		}
		else{
			result ++;

			if (bufferSignature_push(buffer_signature, in, ou, buffer_signature->nb_in, ir)){
				log_err("unable to push bufferSignature");
			}
			else{
				for (i = 0; remove_footprint[i] != NULL; i++){
					if (!memcmp(remove_footprint[i], ou[0], sizeof(struct subMemoryBuffer))){
						break;
					}
				}
				if (remove_footprint[i] == NULL){
					remove_footprint[i] = ou[0];
				}
			}

			if ((match = array_inc(prim_para_array)) != NULL){
				match->nb_in 			= buffer_signature->nb_in;
				match->nb_ou 			= 1;
				match->mem_para_buffer 	= (struct memParameter*)(match + 1);

				for (i = 0; i < buffer_signature->nb_in; i++){
					match->mem_para_buffer[i].addr = subMemoryBuffer_get_start_address(in[i]);
					match->mem_para_buffer[i].size = buffer_signature->in_size[i] / 8;
				}

				match->mem_para_buffer[i].addr = subMemoryBuffer_get_start_address(ou[0]);
				match->mem_para_buffer[i].size = buffer_signature->ou_size / 8;
			}
			else{
				log_err("unable to increment array size");
			}
		}
	}

	return result;
}

static struct primitiveParameter* bufferSignature_search_buffer(struct set* couple_set, struct ir* ir, uint32_t* nb_prim_para){
	uint32_t 					i;
	uint32_t 					j;
	struct subMemoryBuffer** 	remove_footprint;
	uint32_t 					nb_match;
	struct multiColumnPrinter* 	printer 			= NULL;
	struct subMemoryBuffer* 	in[BUFFERSIGNATURE_NB_MAX_INPUT];
	struct subMemoryBuffer* 	ou[1];
	struct array* 				prim_para_array 	= NULL;
	struct primitiveParameter* 	prim_para_buffer 	= NULL;
	struct primitiveParameter* 	prim_para;
	struct memParameter* 		mem_para_buffer;

	if (set_get_length(couple_set) == 0){
		return NULL;
	}

	if ((remove_footprint = (struct subMemoryBuffer**)calloc(set_get_length(couple_set), sizeof(struct subMemoryBuffer*))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	if ((prim_para_array = array_create(sizeof(struct memParameter) * (BUFFERSIGNATURE_NB_MAX_INPUT + 1) + sizeof(struct primitiveParameter))) == NULL){
		log_err("unable to create array");
		goto exit;
	}

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer == NULL){
		log_err("unable to create multiColumnPrinter");
		goto exit;
	}

	multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_INT32);
	multiColumnPrinter_set_column_size(printer, 0, SIGNATURE_NAME_MAX_SIZE);
	multiColumnPrinter_set_title(printer, 0, "NAME");
	multiColumnPrinter_set_title(printer, 1, "FOUND");
	multiColumnPrinter_set_title(printer, 2, "-");

	multiColumnPrinter_print_header(printer);

	for (i = 0; buffer_signature_buffer[i] != NULL; i++){
		nb_match = bufferSignature_recursive_match(buffer_signature_buffer[i], couple_set, in, ou, 0, ir, remove_footprint, prim_para_array);

		if (nb_match){
			multiColumnPrinter_print(printer, buffer_signature_buffer[i]->symbol.name, nb_match, "0.0", NULL);
		}
	}

	for (i = 0; i < set_get_length(couple_set) && remove_footprint[i] != NULL; i++){
		subMemoryBuffer_remove_edge_footprint(remove_footprint[i], ir);
	}

	if (array_get_length(prim_para_array)){
		if ((prim_para_buffer = (struct primitiveParameter*)malloc((sizeof(struct memParameter) * (BUFFERSIGNATURE_NB_MAX_INPUT + 1) + sizeof(struct primitiveParameter)) * array_get_length(prim_para_array))) == NULL){
			log_err("unable to allocate memory");
			goto exit;
		}

		*nb_prim_para = array_get_length(prim_para_array);

		mem_para_buffer = (struct memParameter*)(prim_para_buffer + array_get_length(prim_para_array));

		for (i = 0; i < array_get_length(prim_para_array); i++){
			prim_para = (struct primitiveParameter*)array_get(prim_para_array, i);

			prim_para_buffer[i].nb_in 			= prim_para->nb_in;
			prim_para_buffer[i].nb_ou 			= prim_para->nb_ou;
			prim_para_buffer[i].mem_para_buffer = mem_para_buffer;

			prim_para->mem_para_buffer = (struct memParameter*)(prim_para + 1);

			for (j = 0; j < prim_para_buffer[i].nb_in; j++){
				mem_para_buffer[j].addr = prim_para->mem_para_buffer[j].addr;
				mem_para_buffer[j].size = prim_para->mem_para_buffer[j].size;
			}

			mem_para_buffer = mem_para_buffer + j;

			for (j = 0; j < prim_para_buffer[i].nb_ou; j++){
				mem_para_buffer[j].addr = prim_para->mem_para_buffer[prim_para_buffer[i].nb_in + j].addr;
				mem_para_buffer[j].size = prim_para->mem_para_buffer[prim_para_buffer[i].nb_in + j].size;
			}

			mem_para_buffer = mem_para_buffer + j;
		}
	}

	exit:

	free(remove_footprint);
	if (prim_para_array != NULL){
		array_delete(prim_para_array);
	}
	if (printer != NULL){
		multiColumnPrinter_delete(printer);
	}

	return prim_para_buffer;
}

void bufferSignature_print_buffer(void){
	#define STR_PARA_SIZES 		32
	uint32_t 					i;
	uint32_t 					j;
	int32_t 					off;
	struct multiColumnPrinter* 	printer;
	char 						str_in_sizes[STR_PARA_SIZES];
	char 						str_ou_sizes[STR_PARA_SIZES];

	if (!are_signatures_registered){
		bufferSignature_register_buffer();
	}

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer == NULL){
		log_err("unable to create multiColumnPrinter");
		return;
	}

	multiColumnPrinter_set_title(printer, 0, "NAME");
	multiColumnPrinter_set_title(printer, 1, "ID");
	multiColumnPrinter_set_title(printer, 2, "IN SIZES");
	multiColumnPrinter_set_title(printer, 3, "OU SIZES");

	multiColumnPrinter_set_column_size(printer, 0, SIGNATURE_NAME_MAX_SIZE);
	multiColumnPrinter_set_column_size(printer, 1, 3);
	multiColumnPrinter_set_column_size(printer, 2, STR_PARA_SIZES);
	multiColumnPrinter_set_column_size(printer, 3, STR_PARA_SIZES);

	multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_UINT32);

	multiColumnPrinter_print_header(printer);

	for (i = 0; buffer_signature_buffer[i] != NULL; i++){
		for (j = 0, off = 0; j < buffer_signature_buffer[i]->nb_in; j++){
			off += snprintf(str_in_sizes + off, STR_PARA_SIZES - off, "%u ", buffer_signature_buffer[i]->in_size[j]);
			if (off >= STR_PARA_SIZES){
				break;
			}
		}
		snprintf(str_ou_sizes, STR_PARA_SIZES, "%u", buffer_signature_buffer[i]->ou_size);

		multiColumnPrinter_print(printer, buffer_signature_buffer[i]->symbol.name, buffer_signature_buffer[i]->symbol.id, str_in_sizes, str_ou_sizes, NULL);
	}

	multiColumnPrinter_delete(printer);
}

struct primitiveParameter* ir_search_buffer_signature(struct ir* ir, uint32_t* nb_prim_para){
	uint32_t 					i;
	uint32_t 					j;
	struct node* 				node_cursor;
	struct irOperation* 		operation;
	struct array* 				read_mem_access_array 	= NULL;
	struct array* 				write_mem_access_array 	= NULL;
	struct array* 				buf_in_array 			= NULL;
	struct array* 				buf_ou_array 			= NULL;
	struct memAccess 			mem_access;
	struct set* 				couple_set 				= NULL;
	struct primitiveParameter* 	prim_para_buffer 	= NULL;

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
	log_info_m("%u input buffer(s):", array_get_length(buf_in_array));
	for (i = 0; i < array_get_length(buf_in_array); i++){
		putchar('\t'); memoryBuffer_print(*(struct memoryBuffer**)array_get(buf_in_array, i)); putchar('\n');
	}
	log_info_m("%u output buffer(s):", array_get_length(buf_ou_array));
	for (i = 0; i < array_get_length(buf_ou_array); i++){
		putchar('\t'); memoryBuffer_print(*(struct memoryBuffer**)array_get(buf_ou_array, i)); putchar('\n');
	}
	#endif

	if ((couple_set = set_create(sizeof(struct bufferCouple), 16)) == NULL){
		log_err("unable to create set");
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
				memoryBuffer_search_dependence(in_ptr, ou_ptr, dst_from, &(ir->graph), couple_set);
			}
			free(dst_from);
		}
		else{
			log_err("unable to compute distance from");
		}
	}

	if (set_get_length(couple_set) == 0){
		goto exit;
	}

	#ifdef VERBOSE
	{
		struct setIterator 		it;
		struct bufferCouple* 	couple;

		log_info("dependencies:");

		for (couple = setIterator_get_first(couple_set, &it); couple != NULL; couple = setIterator_get_next(&it)){
			putchar('\t'); bufferCouple_print(couple); putchar('\n');
		}
	}
	#endif

	prim_para_buffer = bufferSignature_search_buffer(couple_set, ir, nb_prim_para);

	exit:
	if (couple_set != NULL){
		set_delete(couple_set);
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

	return prim_para_buffer;
}
