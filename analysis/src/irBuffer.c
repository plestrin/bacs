#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "irBuffer.h"
#include "signatureCollection.h"
#include "array.h"
#include "set.h"
#include "multiColumn.h"
#include "base.h"

#define TAINTED 0xff

static int32_t taint(struct graph* graph, struct node* node, uint8_t* taint_buffer){
	struct taintInternal{
		uint8_t 				taint;
		struct node*			node;
		struct taintInternal* 	next;
	};

	#define overlap_get_mask(size) (~(0xffffffff << (size)))

	struct taintInternal* 	internals;
	struct taintInternal* 	curr_orbital = NULL;
	struct taintInternal* 	next_orbital;
	uint32_t 				i;
	struct node* 			node_cursor;
	struct edge*			edge_cursor;
	struct taintInternal* 	internal_cursor;
	struct irOperation* 	operation;
	uint32_t 				mask;
	ADDRESS 				addr;
	uint32_t 				size;

	if ((internals = (struct taintInternal*)malloc(sizeof(struct taintInternal) * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].taint 	= 0;
		internals[i].node 	= node_cursor;
		internals[i].next 	= NULL;
	}

	curr_orbital = (struct taintInternal*)node->ptr;
	curr_orbital->taint = TAINTED;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_IN_MEM){
		mask = overlap_get_mask(operation->size / 8);
		addr = operation->operation_type.mem.con_addr;
		size = operation->size / 8;

		/* BACKWARD */
		for (node_cursor = operation->operation_type.mem.prev; node_cursor != NULL && mask; node_cursor = operation->operation_type.mem.prev){
			operation = ir_node_get_operation(node_cursor);

			if (operation->operation_type.mem.con_addr <= addr && operation->operation_type.mem.con_addr + operation->size / 8 > addr){
				if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
					mask &= 0xffffffff << (operation->operation_type.mem.con_addr + operation->size / 8 - addr);
				}
				else if(mask & ~(0xffffffff << (operation->operation_type.mem.con_addr + operation->size / 8 - addr))){
					internal_cursor = (struct taintInternal*)node_cursor->ptr;

					internal_cursor->taint = TAINTED;
					internal_cursor->next = curr_orbital;
					curr_orbital = internal_cursor;
				}
			}
			else if (addr <= operation->operation_type.mem.con_addr && addr + size > operation->operation_type.mem.con_addr){
				if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
					mask &= overlap_get_mask(size) >> (addr + size - operation->operation_type.mem.con_addr);
				}
				else if (mask & ~(overlap_get_mask(size) >> (addr + size - operation->operation_type.mem.con_addr))){
					internal_cursor = (struct taintInternal*)node_cursor->ptr;

					internal_cursor->taint = TAINTED;
					internal_cursor->next = curr_orbital;
					curr_orbital = internal_cursor;
				}
			}
		}

		/* FORWARD */
		mask = overlap_get_mask(size);
		for (node_cursor = ir_node_get_operation(node)->operation_type.mem.next; node_cursor != NULL && mask; node_cursor = operation->operation_type.mem.next){
			operation = ir_node_get_operation(node_cursor);

			if (operation->operation_type.mem.con_addr <= addr && operation->operation_type.mem.con_addr + operation->size / 8 > addr){
				if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
					mask &= 0xffffffff << (operation->operation_type.mem.con_addr + operation->size / 8 - addr);
				}
				else if(mask & ~(0xffffffff << (operation->operation_type.mem.con_addr + operation->size / 8 - addr))){
					internal_cursor = (struct taintInternal*)node_cursor->ptr;

					internal_cursor->taint = TAINTED;
					internal_cursor->next = curr_orbital;
					curr_orbital = internal_cursor;
				}
			}
			else if (addr <= operation->operation_type.mem.con_addr && addr + size > operation->operation_type.mem.con_addr){
				if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
					mask &= overlap_get_mask(size) >> (addr + size - operation->operation_type.mem.con_addr);
				}
				else if (mask & ~(overlap_get_mask(size) >> (addr + size - operation->operation_type.mem.con_addr))){
					internal_cursor = (struct taintInternal*)node_cursor->ptr;

					internal_cursor->taint = TAINTED;
					internal_cursor->next = curr_orbital;
					curr_orbital = internal_cursor;
				}
			}
		}
	}

	for ( ; curr_orbital != NULL; curr_orbital = next_orbital){
		for (next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){
			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct taintInternal*)node_cursor->ptr;

				if (internal_cursor->taint == 0){
					internal_cursor->taint = TAINTED;
					internal_cursor->next = next_orbital;
					next_orbital = internal_cursor;
				}
			}

			operation = ir_node_get_operation(curr_orbital->node);
			if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
				mask = overlap_get_mask(operation->size / 8);
				addr = operation->operation_type.mem.con_addr;
				size = operation->size / 8;

				for (node_cursor = operation->operation_type.mem.next; node_cursor != NULL && mask; node_cursor = operation->operation_type.mem.next){
					operation = ir_node_get_operation(node_cursor);

					if (operation->operation_type.mem.con_addr <= addr && operation->operation_type.mem.con_addr + operation->size / 8 > addr){
						if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
							mask &= 0xffffffff << (operation->operation_type.mem.con_addr + operation->size / 8 - addr);
						}
						else if(mask & ~(0xffffffff << (operation->operation_type.mem.con_addr + operation->size / 8 - addr))){
							internal_cursor = (struct taintInternal*)node_cursor->ptr;

							if (internal_cursor->taint == 0){
								internal_cursor->taint = TAINTED;
								internal_cursor->next = next_orbital;
								next_orbital = internal_cursor;
							}
						}
					}
					else if (addr <= operation->operation_type.mem.con_addr && addr + size > operation->operation_type.mem.con_addr){
						if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
							mask &= overlap_get_mask(size) >> (addr + size - operation->operation_type.mem.con_addr);
						}
						else if (mask & ~(overlap_get_mask(size) >> (addr + size - operation->operation_type.mem.con_addr))){
							internal_cursor = (struct taintInternal*)node_cursor->ptr;

							if (internal_cursor->taint == 0){
								internal_cursor->taint = TAINTED;
								internal_cursor->next = next_orbital;
								next_orbital = internal_cursor;
							}
						}
					}
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		internal_cursor = (struct taintInternal*)node_cursor->ptr;
		taint_buffer[i] = internal_cursor->taint;
	}

	free(internals);

	return 0;
}

#define IRBUFFER_MIN_SIZE 8

struct memAccess{
	struct node* 	node;
	uint32_t 		index;
	uint32_t 		offset;
};

#define memAccess_get_addr(mem_access) (ir_node_get_operation((mem_access)->node)->operation_type.mem.con_addr + (mem_access)->offset)

static int32_t compare_memAccess_address_then_order(void* arg1, void* arg2){
	struct memAccess* mem_access1 = (struct memAccess*)arg1;
	struct memAccess* mem_access2 = (struct memAccess*)arg2;

	if (memAccess_get_addr(mem_access1) < memAccess_get_addr(mem_access2)){
		return -1;
	}
	else if (memAccess_get_addr(mem_access1) > memAccess_get_addr(mem_access2)){
		return 1;
	}
	else if (mem_access1->index > mem_access2->index){
		return -1;
	}
	else if (mem_access1->index < mem_access2->index){
		return 1;
	}
	else{
		return 0;
	}
}

struct memoryBuffer{
	ADDRESS 			address_start;
	uint32_t 			size;
	struct memAccess* 	access;
};

#define memoryBuffer_get_size(size) (sizeof(struct memoryBuffer) + sizeof(struct memAccess) * (size))

struct subMemoryBuffer{
	struct memoryBuffer* 	buffer;
	uint32_t 				offset;
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

static inline struct memoryBuffer* memoryBuffer_create(uint32_t size){
	struct memoryBuffer* mem_buf;

	if ((mem_buf = (struct memoryBuffer*)malloc(memoryBuffer_get_size(size))) != NULL){
		mem_buf->size 	= size;
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
	ADDRESS 				addr;
	struct memoryBuffer* 	mem_buf;
	uint32_t 				nb_mem_access;
	struct memAccess* 		mem_access;

	buffer_array = array_create(sizeof(struct memoryBuffer*));
	if (buffer_array == NULL){
		log_err("unable to create array");
		return NULL;
	}

	if ((mapping = array_create_mapping(mem_access_array, compare_memAccess_address_then_order)) == NULL){
		log_err("unable to create mapping");
		return buffer_array;
	}

	mem_access = array_get(mem_access_array, mapping[0]);
	addr = memAccess_get_addr(mem_access);

	if (is_write){
		for (i = 1, j = 1; i < array_get_length(mem_access_array); i++){
			mem_access = array_get(mem_access_array, mapping[i]);
			if (memAccess_get_addr(mem_access) != addr){
				mapping[j ++] = mapping[i];
				addr = memAccess_get_addr(mem_access);
			}
		}
		nb_mem_access = j;
	}
	else{
		for (i = 1, j = 0; i < array_get_length(mem_access_array); i++){
			mem_access = array_get(mem_access_array, mapping[i]);
			if (memAccess_get_addr(mem_access) != addr){
				addr = memAccess_get_addr(mem_access);
				j++;
			}
			mapping[j] = mapping[i];
		}
		nb_mem_access = j + 1;
	}

	for (i = 0; i < nb_mem_access; i = j){
		mem_access = array_get(mem_access_array, mapping[i]);
		addr = memAccess_get_addr(mem_access);

		for (j = i + 1; j < nb_mem_access; j++){
			mem_access = array_get(mem_access_array, mapping[j]);
			if (memAccess_get_addr(mem_access) != addr + (j - i)){
				break;
			}
		}

		if (j - i >= IRBUFFER_MIN_SIZE){
			if ((mem_buf = memoryBuffer_create(j - i)) != NULL){
				mem_buf->address_start 	= addr;
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


static uint8_t* memoryBuffer_get_taint(const struct memoryBuffer* mem_buf, struct graph* graph){
	uint8_t* taint_buffer;
	uint32_t i;

	if ((taint_buffer = malloc(mem_buf->size * graph->nb_node)) != NULL){
		for (i = 0; i < mem_buf->size; i++){
			if (i && mem_buf->access[i].node == mem_buf->access[i - 1].node){
				memcpy(taint_buffer + graph->nb_node * i, taint_buffer + graph->nb_node * (i -1), graph->nb_node);
			}
			else{
				if (taint(graph, mem_buf->access[i].node, taint_buffer + graph->nb_node * i)){
					log_err("unable to compute distance from");
					free(taint_buffer);
					return NULL;
				}
			}
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return taint_buffer;
}

static void memoryBuffer_search_dependence(struct memoryBuffer* in_buf, struct memoryBuffer* ou_buf, const uint8_t* taint_buffer, struct graph* graph, struct set* buffer_couple_set){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	uint32_t 				l;
	uint32_t 				m;
	struct bufferCouple 	new_couple;
	struct bufferCouple* 	set_couple;
	struct subSet 			buffer_couple_sub_set;
	struct setIterator 		it;

	new_couple.in.buffer = in_buf;
	new_couple.ou.buffer = ou_buf;

	subSet_init(&buffer_couple_sub_set, buffer_couple_set);

	for (i = 0; i < in_buf->size; i++){
		for (j = 0; j < ou_buf->size; j++){
			for (k = 0; j + k < ou_buf->size; k++){
				if (taint_buffer[i * graph->nb_node + ou_buf->access[j + k].index] != TAINTED){
					break;
				}
			}
			if (k < IRBUFFER_MIN_SIZE){
				continue;
			}

			for (l = 1; l + i < in_buf->size; l++, k = m){
				for (m = 0; m < k; m++){
					if (taint_buffer[(l + i) * graph->nb_node + ou_buf->access[j + m].index] != TAINTED){
						break;
					}
				}

				if (m < IRBUFFER_MIN_SIZE){
					break;
				}

				if (++l < IRBUFFER_MIN_SIZE){
					continue;
				}

				new_couple.in.offset 	= i;
				new_couple.in.size 		= l;
				new_couple.ou.offset 	= j;
				new_couple.ou.size 		= m;

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
	}
}

void memoryBuffer_print(const struct memoryBuffer* mem_buf){
	printf("[" PRINTF_ADDR ", " PRINTF_ADDR "] size=%4u", mem_buf->address_start, mem_buf->address_start + mem_buf->size, mem_buf->size * 8);
}

static void memoryBuffer_delete_array(struct array* buffer_array){
	uint32_t i;

	for (i = 0; i < array_get_length(buffer_array); i++){
		free(*(struct memoryBuffer**)array_get(buffer_array, i));
	}
	array_delete(buffer_array);

}

#define subMemoryBuffer_get_start_address(sub_mem_buf) ((sub_mem_buf)->buffer->address_start + (sub_mem_buf)->offset)
#define subMemoryBuffer_get_stop_address(sub_mem_buf) (subMemoryBuffer_get_start_address(sub_mem_buf) + (sub_mem_buf)->size)

void subMemoryBuffer_print(const struct subMemoryBuffer* sub_mem_buf){
	printf("[" PRINTF_ADDR ", " PRINTF_ADDR "] size=%4u", subMemoryBuffer_get_start_address(sub_mem_buf), subMemoryBuffer_get_stop_address(sub_mem_buf), sub_mem_buf->size * 8);
}

static enum bufferCoupleCompare bufferCouple_compare_subset_superset(const struct bufferCouple* couple1, const struct bufferCouple* couple2){
	if (couple1->in.buffer != couple2->in.buffer || couple1->ou.buffer != couple2->ou.buffer){
		return BUFFERCOUPLECOMPARE_NONE;
	}

	if (couple1->in.offset <= couple2->in.offset && couple1->in.offset + couple1->in.size >= couple2->in.offset + couple2->in.size){
		if (couple1->ou.offset <= couple2->ou.offset && couple1->ou.offset + couple1->ou.size >= couple2->ou.offset + couple2->ou.size){
			return BUFFERCOUPLECOMPARE_SUPERSET;
		}
	}

	if (couple2->in.offset <= couple1->in.offset && couple2->in.offset + couple2->in.size >= couple1->in.offset + couple1->in.size){
		if (couple2->ou.offset <= couple1->ou.offset && couple2->ou.offset + couple2->ou.size >= couple1->ou.offset + couple1->ou.size){
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
	.in_size 		= {16,160},
	.ou_size 		= 16
};

static struct bufferSignature aes196 = {
	.symbol 		= {
		.name 		= "aes192",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {16,192},
	.ou_size 		= 16
};

static struct bufferSignature aes256 = {
	.symbol 		= {
		.name 		= "aes256",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {16,224},
	.ou_size 		= 16
};

static struct bufferSignature aes128_ks = {
	.symbol 		= {
		.name 		= "aes_ks_128",
	},
	.export_name 	= "ks",
	.nb_in 			= 1,
	.in_size 		= {16},
	.ou_size 		= 160
};

static struct bufferSignature md5_compress = {
	.symbol 		= {
		.name 		= "md5_compress",
	},
	.export_name 	= "compress",
	.nb_in 			= 2,
	.in_size 		= {16,64},
	.ou_size 		= 16
};

static struct bufferSignature sha1_compress = {
	.symbol 		= {
		.name 		= "sha1_compress",
	},
	.export_name 	= "compress",
	.nb_in 			= 2,
	.in_size 		= {20,64},
	.ou_size 		= 20
};

static struct bufferSignature xtea = {
	.symbol 		= {
		.name 		= "xtea_32",
	},
	.export_name 	= "block_cipher",
	.nb_in 			= 2,
	.in_size 		= {8,16},
	.ou_size 		= 8
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

static uint32_t bufferSignature_recursive_match(struct bufferSignature* buffer_signature, struct set* couple_set, struct subMemoryBuffer** in, struct subMemoryBuffer** ou, uint32_t nb_in, struct array* prim_para_array){
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
			result += bufferSignature_recursive_match(buffer_signature, couple_set, in, ou, nb_in + 1, prim_para_array);
		}
		else{
			result ++;

			if ((match = array_inc(prim_para_array)) != NULL){
				match->nb_in 			= buffer_signature->nb_in;
				match->nb_ou 			= 1;
				match->mem_para_buffer 	= (struct memParameter*)(match + 1);

				for (i = 0; i < buffer_signature->nb_in; i++){
					match->mem_para_buffer[i].addr = subMemoryBuffer_get_start_address(in[i]);
					match->mem_para_buffer[i].size = buffer_signature->in_size[i];
				}

				match->mem_para_buffer[i].addr = subMemoryBuffer_get_start_address(ou[0]);
				match->mem_para_buffer[i].size = buffer_signature->ou_size;
			}
			else{
				log_err("unable to increment array size");
			}
		}
	}

	return result;
}

static struct primitiveParameter* bufferSignature_search_buffer(struct set* couple_set, uint32_t* nb_prim_para){
	uint32_t 					i;
	uint32_t 					j;
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
		nb_match = bufferSignature_recursive_match(buffer_signature_buffer[i], couple_set, in, ou, 0, prim_para_array);

		if (nb_match){
			multiColumnPrinter_print(printer, buffer_signature_buffer[i]->symbol.name, nb_match, "0.0", NULL);
		}
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
	struct primitiveParameter* 	prim_para_buffer 		= NULL;

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
				else{
					for (j = 0; j < operation->size / 8; j++){
						mem_access.offset = j;

						if (array_add(read_mem_access_array, &mem_access) < 0){
						log_err("unable to add element to array");
						}
					}
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM 	: {
				if (operation->operation_type.mem.con_addr == MEMADDRESS_INVALID){
					log_warn("concrete memory address is unspecified, final results might be incorrect");
				}
				else{
					for (j = 0; j < operation->size / 8; j++){
						mem_access.offset = j;

						if (array_add(write_mem_access_array, &mem_access) < 0){
							log_err("unable to add element to array");
						}
					}
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
		uint8_t* 				taint_buffer;
		struct memoryBuffer* 	in_ptr;
		struct memoryBuffer* 	ou_ptr;

		in_ptr = *(struct memoryBuffer**)array_get(buf_in_array, i);
		if ((taint_buffer = memoryBuffer_get_taint(in_ptr, &(ir->graph))) != NULL){
			for (j = 0; j < array_get_length(buf_ou_array); j++){
				ou_ptr = *(struct memoryBuffer**)array_get(buf_ou_array, j);
				memoryBuffer_search_dependence(in_ptr, ou_ptr, taint_buffer, &(ir->graph), couple_set);
			}
			free(taint_buffer);
		}
		else{
			log_err("unable to compute taint buffer");
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

	prim_para_buffer = bufferSignature_search_buffer(couple_set, nb_prim_para);

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
