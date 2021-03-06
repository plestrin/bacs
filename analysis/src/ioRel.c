#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "ioRel.h"
#include "array.h"
#include "printBuffer.h"
#include "base.h"

static void change_endianness(uint32_t* dst, const uint32_t* src, size_t size);

struct concreteMemAccess{
	ADDRESS 	address;
	uint32_t 	order;
	uint8_t 	value;
	enum {
		CONCRETE_READ,
		CONCRETE_WRITE
	} 			type;
};

static int32_t concreteMemAccess_compare(void* arg1, void* arg2){
	struct concreteMemAccess* access1 = (struct concreteMemAccess*)arg1;
	struct concreteMemAccess* access2 = (struct concreteMemAccess*)arg2;

	if (access1->address < access2->address){
		return -1;
	}
	else if (access1->address > access2->address){
		return 1;
	}
	else if (access1->order < access2->order){
		return -1;
	}
	else if (access1->order > access2->order){
		return 1;
	}
	else if (access1->type == CONCRETE_READ && access2->type == CONCRETE_WRITE){
		return -1;
	}
	else if (access1->type == CONCRETE_WRITE && access2->type == CONCRETE_READ){
		return 1;
	}
	else{
		log_err("this case is not supposed to happen");
		return 0;
	}
}

struct searchableMemoryHeader{
	ADDRESS 	address;
	size_t 		size;
	uint8_t* 	ptr;
};

struct searchableMemory{
	uint32_t 						nb_segment;
	struct searchableMemoryHeader* 	headers;
};

static struct searchableMemory* searchableMemory_create(struct concreteMemAccess* cma_buffer, uint32_t nb_cma){
	struct searchableMemory* 	mem;
	uint8_t* 					ptr;
	uint32_t 					i;
	uint32_t 					j;

	if ((mem = (struct searchableMemory*)malloc(sizeof(struct searchableMemory) + nb_cma * (1 + sizeof(struct searchableMemoryHeader)))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	ptr = (uint8_t*)(mem + 1);

	mem->nb_segment = 0;
	mem->headers = (struct searchableMemoryHeader*)(ptr + nb_cma);

	for (i = 0; i < nb_cma; i = j){
		ptr[0] = cma_buffer[i].value;
		for (j = i + 1; j < nb_cma; j++){
			if (cma_buffer[j - 1].address + 1 != cma_buffer[j].address){
				#ifdef EXTRA_CHECK
				if (cma_buffer[j - 1].address + 1 > cma_buffer[j].address){
					log_err("incorrect concrete memory access ordering");
				}
				#endif
				break;
			}
			ptr[j - i] = cma_buffer[j].value;
		}

		mem->headers[mem->nb_segment].address 	= cma_buffer[i].address;
		mem->headers[mem->nb_segment].size 		= j - i;
		mem->headers[mem->nb_segment].ptr 		= ptr;

		mem->nb_segment ++;
		ptr += j - i;
	}

	return mem;
}

static void searchableMemory_print(const struct searchableMemory* mem, const char* desc){
	uint32_t i;

	printf("*** SEARCHABLE MEMORY %s ***\n", desc);

	for (i = 0; i < mem->nb_segment; i++){
		printf("\t[" PRINTF_ADDR " : " PRINTF_ADDR "] ", mem->headers[i].address, mem->headers[i].address + mem->headers[i].size);
		fprintBuffer_raw(stdout, (char*)mem->headers[i].ptr, mem->headers[i].size);
		printf("\n");
	}
}

static int32_t searchableMemory_search(const struct searchableMemory* mem, const uint8_t* buffer, size_t size){
	uint32_t i;
	uint32_t j;

	for (i = 0; i < mem->nb_segment; i++){
		for (j = 0; size + j <= mem->headers[i].size; j++){
			if (!memcmp(mem->headers[i].ptr + j, buffer, size)){
				return 1;
			}
		}
	}

	return 0;
}

struct smIterator{
	const struct searchableMemory* 	mem;
	uint32_t 						current_seg;
	size_t 							current_offset;
};

static const uint8_t* smIterator_get_first(struct smIterator* it, const struct searchableMemory* mem, size_t size){
	for (it->mem = mem, it->current_seg = 0, it->current_offset = 0; it->current_seg < mem->nb_segment; it->current_seg ++, it->current_offset = 0){
		if (size + it->current_offset <= mem->headers[it->current_seg].size){
			return it->mem->headers[it->current_seg].ptr + it->current_offset;
		}
	}

	return NULL;
}

static const uint8_t* smIterator_get_next(struct smIterator* it, size_t size){
	it->current_offset ++;
	for (; it->current_seg < it->mem->nb_segment; it->current_seg ++, it->current_offset = 0){
		if (size + it->current_offset <= it->mem->headers[it->current_seg].size){
			return it->mem->headers[it->current_seg].ptr + it->current_offset;
		}
	}

	return NULL;
}

static uint32_t trace_create_searchableMemory(struct trace* trace, struct searchableMemory** mem_read, struct searchableMemory** mem_writ){
	uint32_t 					i;
	uint32_t 					j;
	struct concreteMemAccess 	cma;
	struct array* 				cma_array;
	uint32_t* 					cma_mapping;
	struct instructionIterator 	it;
	uint32_t 					order;
	struct concreteMemAccess* 	cma_buffer;
	struct concreteMemAccess* 	cma_read_buffer;
	struct concreteMemAccess* 	cma_writ_buffer;
	uint32_t 					nb_cma;
	uint32_t 					nb_read_cma;
	uint32_t 					nb_writ_cma;

	*mem_read = NULL;
	*mem_writ = NULL;

	if (trace->mem_trace == NULL || trace->mem_trace->mem_addr_buffer == NULL || trace->mem_trace->mem_valu_buffer == NULL){
		log_err("missing address or value");
		return -1;
	}

	if (assembly_get_first_instruction(&(trace->assembly), &it)){
		log_err("unable to fetch first instruction from the assembly");
		return -1;
	}

	if ((cma_array = array_create(sizeof(struct concreteMemAccess))) == NULL){
		log_err("unable to create array");
		return -1;
	}

	for (order = 0; ; order ++){
		const xed_inst_t* 		xi;
		const xed_operand_t* 	xed_op;
		uint32_t 				nb_read_mem;
		uint32_t 				nb_write_mem;
		uint32_t 				nb_mem;
		uint32_t 				mem_descriptor;
		uint32_t 				offset;

		if (xed_decoded_inst_get_iclass(&(it.xedd)) == XED_ICLASS_NOP){
			goto next;
		}

		if (!instructionIterator_is_mem_addr_valid(&it)){
			log_err_m("memory address is invalid for instruction %u", it.instruction_index);
			goto next;
		}

		xi = xed_decoded_inst_inst(&(it.xedd));
		for (i = 0, nb_read_mem = 0, nb_mem = 0; i < xed_inst_noperands(xi); i++){
			xed_op = xed_inst_operand(xi, i);

			switch (xed_operand_name(xed_op)){
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					if (xed_operand_read(xed_op)){
						mem_descriptor = MEMADDRESS_DESCRIPTOR_CLEAN;
						memAddress_descriptor_set_read(mem_descriptor, nb_read_mem);

						cma.address = memAddress_get_and_check(trace->mem_trace->mem_addr_buffer + instructionIterator_get_mem_addr_index(&it) + nb_mem, mem_descriptor);
						cma.order 	= order;
						cma.type 	= CONCRETE_READ;

						for (j = 0; j < xed_decoded_inst_get_memory_operand_length(&(it.xedd), nb_mem); j++){
							cma.value = trace->mem_trace->mem_valu_buffer[(instructionIterator_get_mem_addr_index(&it) + nb_read_mem) * MEMVALUE_PADDING + j];
							if (array_add(cma_array, &cma) < 0){
								log_err("unable to add element to array");
							}
							cma.address ++;
						}

						nb_read_mem ++;
					}

					nb_mem ++;
					break;
				}
				case XED_OPERAND_AGEN 	:
				case XED_OPERAND_IMM0 	:
				case XED_OPERAND_REG0 	:
				case XED_OPERAND_REG1 	:
				case XED_OPERAND_REG2 	:
				case XED_OPERAND_REG3 	:
				case XED_OPERAND_REG4 	:
				case XED_OPERAND_REG5 	:
				case XED_OPERAND_REG6 	:
				case XED_OPERAND_REG7 	:
				case XED_OPERAND_REG8 	:
				case XED_OPERAND_RELBR 	:
				case XED_OPERAND_BASE0 	:
				case XED_OPERAND_BASE1 	: {
					break;
				}
				default 				: {
					log_err_m("operand type not supported: %s for instruction %s", xed_operand_enum_t2str(xed_operand_name(xed_op)), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it.xedd))));
					break;
				}
			}
		}
		for (i = 0, nb_write_mem = 0, nb_mem = 0; i < xed_inst_noperands(xi); i++){
			xed_op = xed_inst_operand(xi, i);

			switch (xed_operand_name(xed_op)){
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					if (xed_operand_written(xed_op)){
						mem_descriptor = MEMADDRESS_DESCRIPTOR_CLEAN;
						memAddress_descriptor_set_write(mem_descriptor, nb_write_mem);

						cma.address = memAddress_get_and_check(trace->mem_trace->mem_addr_buffer + instructionIterator_get_mem_addr_index(&it) + nb_mem, mem_descriptor);
						cma.order 	= order;
						cma.type 	= CONCRETE_WRITE;

						/* This is relatively complex. Refer to lightTracer.cpp */
						if (!nb_read_mem){
							offset = 0;
						}
						else if (xed_operand_read(xed_op)){
							offset = 0;
						}
						else if (nb_read_mem == 1){
							offset = 1;
						}
						else{
							offset = 2;
						}

						for (j = 0; j < xed_decoded_inst_get_memory_operand_length(&(it.xedd), nb_mem); j++){
							cma.value = trace->mem_trace->mem_valu_buffer[(instructionIterator_get_mem_addr_index(&it) + offset) * MEMVALUE_PADDING + (MEMVALUE_PADDING / 2) + j];
							if (array_add(cma_array, &cma) < 0){
								log_err("unable to add element to array");
							}
							cma.address ++;
						}

						nb_write_mem ++;
					}
					nb_mem ++;
					break;
				}
				default 				: {
					break;
				}
			}
		}

		next:
		if (instructionIterator_get_instruction_index(&it) == assembly_get_nb_instruction(&(trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(trace->assembly), &it)){
				log_err("unable to fetch next instruction from the assembly");
				break;
			}
		}
	}

	if (!array_get_length(cma_array)){
		array_delete(cma_array);
		return 0;
	}

	if ((cma_mapping = array_create_mapping(cma_array, concreteMemAccess_compare)) == NULL){
		log_err("unable to create mapping");
		array_delete(cma_array);
		return -1;
	}

	if ((cma_buffer = (struct concreteMemAccess*)malloc(sizeof(struct concreteMemAccess) * array_get_length(cma_array))) == NULL){
		log_err("unable to allocate memory");
		free(cma_mapping);
		array_delete(cma_array);
		return -1;
	}

	memcpy(cma_buffer, array_get(cma_array, cma_mapping[0]), sizeof(struct concreteMemAccess));
	for (i = 1, nb_cma = 1; i < array_get_length(cma_array); i++){
		memcpy(cma_buffer + nb_cma, array_get(cma_array, cma_mapping[i]), sizeof(struct concreteMemAccess));
		if (cma_buffer[nb_cma - 1].address != cma_buffer[nb_cma].address){
			nb_cma ++;
		}
		else if (cma_buffer[nb_cma - 1].type == CONCRETE_READ && cma_buffer[nb_cma].type == CONCRETE_WRITE){
			nb_cma ++;
		}
		else if (cma_buffer[nb_cma - 1].type == CONCRETE_WRITE && cma_buffer[nb_cma].type == CONCRETE_WRITE){
			memcpy(cma_buffer + nb_cma - 1, cma_buffer + nb_cma, sizeof(struct concreteMemAccess));
		}
		#ifdef EXTRA_CHECK
		else if (cma_buffer[nb_cma].type == CONCRETE_READ && cma_buffer[nb_cma - 1].value != cma_buffer[nb_cma].value){
			log_err_m("found inconsistency at address " PRINTF_ADDR " (v1: %02x, o1: %u) - (v1: %02x, o1: %u)", cma_buffer[nb_cma].address, cma_buffer[nb_cma - 1].value, cma_buffer[nb_cma - 1].order, cma_buffer[nb_cma].value, cma_buffer[nb_cma].order);
			free(cma_buffer);
			free(cma_mapping);
			array_delete(cma_array);
			return -1;
		}
		#endif
	}

	free(cma_mapping);
	array_delete(cma_array);

	cma_read_buffer = (struct concreteMemAccess*)malloc(sizeof(struct concreteMemAccess) * nb_cma);
	cma_writ_buffer = (struct concreteMemAccess*)malloc(sizeof(struct concreteMemAccess) * nb_cma);

	if (cma_read_buffer == NULL || cma_writ_buffer == NULL){
		log_err("unable to allocate memory");
		if (cma_read_buffer != NULL){
			free(cma_read_buffer);
		}
		if (cma_writ_buffer != NULL){
			free(cma_writ_buffer);
		}
		free(cma_buffer);

		return -1;
	}

	for (i = 0, nb_read_cma = 0, nb_writ_cma = 0; i < nb_cma; i++){
		if (cma_buffer[i].type == CONCRETE_READ){
			memcpy(cma_read_buffer + nb_read_cma, cma_buffer + i, sizeof(struct concreteMemAccess));
			nb_read_cma ++;
		}
		else{
			memcpy(cma_writ_buffer + nb_writ_cma, cma_buffer + i, sizeof(struct concreteMemAccess));
			nb_writ_cma ++;
		}
	}

	free(cma_buffer);

	if (!nb_read_cma || !nb_writ_cma){
		free(cma_read_buffer);
		free(cma_writ_buffer);
		return 0;
	}

	*mem_read = searchableMemory_create(cma_read_buffer, nb_read_cma);
	*mem_writ = searchableMemory_create(cma_writ_buffer, nb_writ_cma);

	free(cma_read_buffer);
	free(cma_writ_buffer);

	if (*mem_read == NULL || *mem_writ == NULL){
		log_err("unable to create searchableMemory");
		if (*mem_read != NULL){
			free(*mem_read);
			*mem_read = NULL;
		}
		if (*mem_writ != NULL){
			free(*mem_writ);
			*mem_writ = NULL;
		}
		return -1;
	}

	return 0;
}

static uint32_t std_prim_search_xtea_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);
static uint32_t std_prim_search_xtea_dec(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);
static uint32_t std_prim_search_md5_comp(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);
static uint32_t std_prim_search_sha_comp(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);
static uint32_t std_prim_search_aes128_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);
static uint32_t std_prim_search_aes192_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);
static uint32_t std_prim_search_aes256_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ);

void trace_search_io(struct trace* trace){
	struct searchableMemory* 	mem_read;
	struct searchableMemory* 	mem_writ;
	uint32_t 					found;

	if (trace_create_searchableMemory(trace, &mem_read, &mem_writ)){
		log_err("unable to create searchable memory");
		return;
	}

	if (mem_read != NULL && mem_writ != NULL){
		found  = std_prim_search_xtea_enc(mem_read, mem_writ);
		found |= std_prim_search_xtea_dec(mem_read, mem_writ);
		found |= std_prim_search_md5_comp(mem_read, mem_writ);
		found |= std_prim_search_sha_comp(mem_read, mem_writ);
		found |= std_prim_search_aes128_enc(mem_read, mem_writ);
		found |= std_prim_search_aes192_enc(mem_read, mem_writ);
		found |= std_prim_search_aes256_enc(mem_read, mem_writ);

		if (!found){
			searchableMemory_print(mem_read, "INPUT");
			searchableMemory_print(mem_writ, "OUTPUT");
		}
	}

	if (mem_read != NULL){
		free(mem_read);
	}
	if (mem_writ != NULL){
		free(mem_writ);
	}
}

static void primitiveParameter_recursive_search(struct primitiveParameter* prim_para, struct searchableMemory* mem_read, struct searchableMemory* mem_writ, uint32_t progress, void(*callback)(const struct primitiveParameter* prim_para)){
	uint32_t i;

	if (progress < prim_para->nb_in){
		for (i = 0; i < mem_read->nb_segment; i++){
			if (prim_para->mem_para_buffer[progress].addr - mem_read->headers[i].address <= mem_read->headers[i].size && prim_para->mem_para_buffer[progress].size <= mem_read->headers[i].size - (prim_para->mem_para_buffer[progress].addr - mem_read->headers[i].address)){
				prim_para->mem_para_buffer[progress].ptr = mem_read->headers[i].ptr + (prim_para->mem_para_buffer[progress].addr - mem_read->headers[i].address);

				if (progress + 1 == prim_para->nb_in + prim_para->nb_ou){
					callback(prim_para);
				}
				else{
					primitiveParameter_recursive_search(prim_para, mem_read, mem_writ, progress + 1, callback);
				}
			}
		}
	}
	else{
		for (i = 0; i < mem_writ->nb_segment; i++){
			if (prim_para->mem_para_buffer[progress].addr - mem_writ->headers[i].address <= mem_writ->headers[i].size && prim_para->mem_para_buffer[progress].size <= mem_writ->headers[i].size - (prim_para->mem_para_buffer[progress].addr - mem_writ->headers[i].address)){
				prim_para->mem_para_buffer[progress].ptr = mem_writ->headers[i].ptr + (prim_para->mem_para_buffer[progress].addr - mem_writ->headers[i].address);

				if (progress + 1 == prim_para->nb_in + prim_para->nb_ou){
					callback(prim_para);
				}
				else{
					primitiveParameter_recursive_search(prim_para, mem_read, mem_writ, progress + 1, callback);
				}
			}
		}
	}
}

static void primitiveParameter_print_match(const struct primitiveParameter* prim_para){
	uint32_t i;

	puts("*** Content of Primitive Parameter ***");

	for (i = 0; i < prim_para->nb_in; i++){
		printf("\tIn %u [" PRINTF_ADDR " : " PRINTF_ADDR "] ", i, prim_para->mem_para_buffer[i].addr, prim_para->mem_para_buffer[i].addr + prim_para->mem_para_buffer[i].size);
		fprintBuffer_raw(stdout, (char*)prim_para->mem_para_buffer[i].ptr, prim_para->mem_para_buffer[i].size);
		printf("\n");
	}

	for (i = 0; i < prim_para->nb_ou; i++){
		printf("\tOu %u [" PRINTF_ADDR " : " PRINTF_ADDR "] ", i, prim_para->mem_para_buffer[i + prim_para->nb_in].addr, prim_para->mem_para_buffer[i + prim_para->nb_in].addr + prim_para->mem_para_buffer[i + prim_para->nb_in].size);
		fprintBuffer_raw(stdout, (char*)prim_para->mem_para_buffer[i + prim_para->nb_in].ptr, prim_para->mem_para_buffer[i + prim_para->nb_in].size);
		printf("\n");
	}
}

static void primitiveParameter_check(const struct primitiveParameter* prim_para){
	uint32_t found = 0;

	/* AES 128 */
	if (prim_para->nb_in == 2 && prim_para->nb_ou == 1 && prim_para->mem_para_buffer[0].size == 16 && prim_para->mem_para_buffer[1].size == 160 && prim_para->mem_para_buffer[2].size == 16){
		symmetric_key 	skey;
		uint8_t 		key[16];
		uint8_t 		block_ou[16];

		/* Endianness 1 */
		if (aes_setup(prim_para->mem_para_buffer[1].ptr, 16, 10, &skey) != CRYPT_OK){
			log_err("unable to setup aes128 key");
		}
		else{
			if (aes_ecb_encrypt(prim_para->mem_para_buffer[0].ptr, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes128");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_aes128_enc              | 1            | 0.0");
					found = 1;
				}
			}
		}

		/* Endianness 2 */
		change_endianness((uint32_t*)key, (uint32_t*)prim_para->mem_para_buffer[1].ptr, sizeof(key) / sizeof(uint32_t));
		if (aes_setup(key, sizeof(key), 10, &skey) != CRYPT_OK){
			log_err("unable to setup aes128 key");
		}
		else{
			if (aes_ecb_encrypt(prim_para->mem_para_buffer[0].ptr, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes128");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_aes128_enc              | 1            | 0.0");
					found = 1;
				}
			}
		}
	}

	/* AES 192 */
	if (prim_para->nb_in == 2 && prim_para->nb_ou == 1 && prim_para->mem_para_buffer[0].size == 16 && prim_para->mem_para_buffer[1].size == 192 && prim_para->mem_para_buffer[2].size == 16){
		symmetric_key 	skey;
		uint8_t 		key[24];
		uint8_t 		block_ou[16];

		/* Endianness 1 */
		if (aes_setup(prim_para->mem_para_buffer[1].ptr, 24, 12, &skey) != CRYPT_OK){
			log_err("unable to setup aes192 key");
		}
		else{
			if (aes_ecb_encrypt(prim_para->mem_para_buffer[0].ptr, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes192");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_aes192_enc              | 1            | 0.0");
					found = 1;
				}
			}
		}

		/* Endianness 2 */
		change_endianness((uint32_t*)key, (uint32_t*)prim_para->mem_para_buffer[1].ptr, sizeof(key) / sizeof(uint32_t));
		if (aes_setup(key, sizeof(key), 12, &skey) != CRYPT_OK){
			log_err("unable to setup aes192 key");
		}
		else{
			if (aes_ecb_encrypt(prim_para->mem_para_buffer[0].ptr, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes192");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_aes192_enc              | 1            | 0.0");
					found = 1;
				}
			}
		}
	}

	/* AES 256 */
	if (prim_para->nb_in == 2 && prim_para->nb_ou == 1 && prim_para->mem_para_buffer[0].size == 16 && prim_para->mem_para_buffer[1].size == 224 && prim_para->mem_para_buffer[2].size == 16){
		symmetric_key 	skey;
		uint8_t 		key[32];
		uint8_t 		block_ou[16];

		/* Endianness 1 */
		if (aes_setup(prim_para->mem_para_buffer[1].ptr, 32, 14, &skey) != CRYPT_OK){
			log_err("unable to setup aes256 key");
		}
		else{
			if (aes_ecb_encrypt(prim_para->mem_para_buffer[0].ptr, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes256");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_aes256_enc              | 1            | 0.0");
					found = 1;
				}
			}
		}

		/* Endianness 2 */
		change_endianness((uint32_t*)key, (uint32_t*)prim_para->mem_para_buffer[1].ptr, sizeof(key) / sizeof(uint32_t));
		if (aes_setup(key, sizeof(key), 14, &skey) != CRYPT_OK){
			log_err("unable to setup aes256 key");
		}
		else{
			if (aes_ecb_encrypt(prim_para->mem_para_buffer[0].ptr, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes256");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_aes256_enc              | 1            | 0.0");
					found = 1;
				}
			}
		}
	}

	/* MD5 */
	if (prim_para->nb_in == 2 && prim_para->nb_ou == 1 && prim_para->mem_para_buffer[0].size == 16 && prim_para->mem_para_buffer[1].size == 64 && prim_para->mem_para_buffer[2].size == 16){
		uint8_t 	msg[64];
		hash_state 	md;

		memcpy(md.md5.state, prim_para->mem_para_buffer[0].ptr, sizeof(md.md5.state));
		md.md5.curlen = 0;
		md.md5.length = 0;

		memcpy(msg, prim_para->mem_para_buffer[1].ptr, sizeof(msg));
		if (md5_process(&md, msg, sizeof(msg)) != CRYPT_OK){
			log_err("unable to process md5");
		}
		else{
			if (!memcmp((uint8_t*)md.md5.state, prim_para->mem_para_buffer[2].ptr, sizeof(md.md5.state))){
				puts("verifier_md5_compress            | 1            | 0.0");
				found = 1;
			}
		}
	}

	/* SHA1 */
	if (prim_para->nb_in == 2 && prim_para->nb_ou == 1 && prim_para->mem_para_buffer[0].size == 20 && prim_para->mem_para_buffer[1].size == 64 && prim_para->mem_para_buffer[2].size == 20){
		uint8_t 	msg[64];
		hash_state 	md;

		/* Endianness 1 */
		memcpy(md.sha1.state, prim_para->mem_para_buffer[0].ptr, sizeof(md.sha1.state));
		md.sha1.curlen = 0;
		md.sha1.length = 0;

		memcpy(msg, prim_para->mem_para_buffer[1].ptr, sizeof(msg));
		if (sha1_process(&md, msg, sizeof(msg)) != CRYPT_OK){
			log_err("unable to process sha1");
		}
		else{
			if (!memcmp((uint8_t*)md.sha1.state, prim_para->mem_para_buffer[2].ptr, sizeof(md.sha1.state))){
				puts("verifier_sha1_compress           | 1            | 0.0");
				found = 1;
			}
		}

		/* Endianness 2 */
		memcpy(md.sha1.state, prim_para->mem_para_buffer[0].ptr, sizeof(md.sha1.state));
		md.sha1.curlen = 0;
		md.sha1.length = 0;

		change_endianness((uint32_t*)msg, (uint32_t*)msg, sizeof(msg) / sizeof(uint32_t));
		if (sha1_process(&md, msg, sizeof(msg)) != CRYPT_OK){
			log_err("unable to process sha1");
		}
		else{
			if (!memcmp((uint8_t*)md.sha1.state, prim_para->mem_para_buffer[2].ptr, sizeof(md.sha1.state))){
				puts("verifier_sha1_compress           | 1            | 0.0");
				found = 1;
			}
		}
	}

	/* XTEA */
	if (prim_para->nb_in == 2 && prim_para->nb_ou == 1 && prim_para->mem_para_buffer[0].size == 8 && prim_para->mem_para_buffer[1].size == 16 && prim_para->mem_para_buffer[2].size == 8){
		symmetric_key 	skey;
		uint8_t 		block_in[8];
		uint8_t 		block_ou[8];

		if (xtea_setup(prim_para->mem_para_buffer[1].ptr, 16, 32, &skey) != CRYPT_OK){
			log_err("unable to setup xtea key");;
		}
		else{
			/* ENC Endianness 1 */
			memcpy(block_in, prim_para->mem_para_buffer[0].ptr, sizeof(block_in));
			if (xtea_ecb_encrypt(block_in, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt xtea");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_xtea_enc                | 1            | 0.0");
					found = 1;
				}
			}

			/* DEC Endianness 1 */
			if (xtea_ecb_decrypt(block_in, block_ou, &skey) != CRYPT_OK){
				log_err("unable to decrypt xtea");
			}
			else{
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_xtea_dec                | 1            | 0.0");
					found = 1;
				}
			}

			/* ENC Endianness 2 */
			change_endianness((uint32_t*)block_in, (uint32_t*)block_in, sizeof(block_in) / sizeof(uint32_t));
			if (xtea_ecb_encrypt(block_in, block_ou, &skey) != CRYPT_OK){
				log_err("unable to encrypt xtea");
			}
			else{
				change_endianness((uint32_t*)block_ou, (uint32_t*)block_ou, sizeof(block_ou) / sizeof(uint32_t));
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_xtea_enc                | 1            | 0.0");
					found = 1;
				}
			}

			/* DEC Endianness 2 */
			if (xtea_ecb_decrypt(block_in, block_ou, &skey) != CRYPT_OK){
				log_err("unable to decrypt xtea");
			}
			else{
				change_endianness((uint32_t*)block_ou, (uint32_t*)block_ou, sizeof(block_ou) / sizeof(uint32_t));
				if (!memcmp(block_ou, prim_para->mem_para_buffer[2].ptr, sizeof(block_ou))){
					puts("verifier_xtea_dec                | 1            | 0.0");
					found = 1;
				}
			}
		}
	}

	if (!found){
		primitiveParameter_print_match(prim_para);
	}
}

void trace_check_io(struct trace* trace, struct primitiveParameter* prim_para_buffer, uint32_t nb_prim_para){
	struct searchableMemory* 	mem_read;
	struct searchableMemory* 	mem_writ;
	uint32_t 					i;

	if (trace_create_searchableMemory(trace, &mem_read, &mem_writ)){
		log_err("unable to create searchable memory");
		return;
	}

	for (i = 0; i < nb_prim_para; i++){
		primitiveParameter_recursive_search(prim_para_buffer + i, mem_read, mem_writ, 0, primitiveParameter_check);
	}

	if (mem_read != NULL){
		free(mem_read);
	}
	if (mem_writ != NULL){
		free(mem_writ);
	}
}

static void change_endianness(uint32_t* dst, const uint32_t* src, size_t size){
	size_t i;

	for (i = 0; i < size; i++){
		dst[i] = __builtin_bswap32(src[i]);
	}
}

static uint32_t std_prim_search_xtea_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_pt;
	struct smIterator 	it_key;
	const uint8_t* 		pt;
	uint8_t 			pt_inv[8];
	const uint8_t* 		key;
	symmetric_key 		skey;
	uint8_t 			ct[8];
	uint32_t 			result;

	for (pt = smIterator_get_first(&it_pt, mem_read, 8), result = 0; pt != NULL; pt = smIterator_get_next(&it_pt, 8)){
		for (key = smIterator_get_first(&it_key, mem_read, 16); key != NULL; key = smIterator_get_next(&it_key, 16)){
			if (xtea_setup(key, 16, 32, &skey) != CRYPT_OK){
				log_err("unable to setup xtea key");
				break;
			}

			/* Endianness 1 */
			if (xtea_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt xtea");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("xtea_enc                         | 1            | 0.0");
				result = 1;
				continue;
			}

			/* Endianness 2*/
			change_endianness((uint32_t*)pt_inv, (const uint32_t*)pt, 2);
			if (xtea_ecb_encrypt(pt_inv, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt xtea");
				break;
			}

			change_endianness((uint32_t*)ct, (uint32_t*)ct, 2);
			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("xtea_enc                         | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}

static uint32_t std_prim_search_xtea_dec(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_ct;
	struct smIterator 	it_key;
	const uint8_t* 		ct;
	uint8_t 			ct_inv[8];
	const uint8_t* 		key;
	symmetric_key 		skey;
	uint8_t 			pt[8];
	uint32_t 			result;

	for (ct = smIterator_get_first(&it_ct, mem_read, 8), result = 0; ct != NULL; ct = smIterator_get_next(&it_ct, 8)){
		for (key = smIterator_get_first(&it_key, mem_read, 16); key != NULL; key = smIterator_get_next(&it_key, 16)){
			if (xtea_setup(key, 16, 32, &skey) != CRYPT_OK){
				log_err("unable to setup xtea key");
				break;
			}

			/* Endianness 1 */
			if (xtea_ecb_decrypt(ct, pt, &skey) != CRYPT_OK){
				log_err("unable to decrypt xtea");
				break;
			}

			if (searchableMemory_search(mem_writ, pt, sizeof(pt))){
				puts("xtea_dec                         | 1            | 0.0");
				result = 1;
				continue;
			}

			/* Endianness 2*/
			change_endianness((uint32_t*)ct_inv, (const uint32_t*)ct, 2);
			if (xtea_ecb_decrypt(ct_inv, pt, &skey) != CRYPT_OK){
				log_err("unable to decrypt xtea");
				break;
			}

			change_endianness((uint32_t*)pt, (uint32_t*)pt, 2);
			if (searchableMemory_search(mem_writ, pt, sizeof(pt))){
				puts("xtea_dec                         | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}

static uint32_t std_prim_search_md5_comp(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_state;
	struct smIterator 	it_msg;
	const uint8_t* 		state;
	const uint8_t* 		msg;
	hash_state 			md;
	uint32_t 			result;

	for (state = smIterator_get_first(&it_state, mem_read, sizeof(md.md5.state)), result = 0; state != NULL; state = smIterator_get_next(&it_state, sizeof(md.md5.state))){
		for (msg = smIterator_get_first(&it_msg, mem_read, 64); msg != NULL; msg = smIterator_get_next(&it_msg, 64)){
			memcpy(md.md5.state, state, sizeof(md.md5.state));
			md.md5.curlen = 0;
			md.md5.length = 0;

			if (md5_process(&md, msg, 64) != CRYPT_OK){
				log_err("unable to process md5");
				break;
			}

			if (searchableMemory_search(mem_writ, (uint8_t*)md.md5.state, sizeof(md.md5.state))){
				puts("md5_compress                     | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}

static uint32_t std_prim_search_sha_comp(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_state;
	struct smIterator 	it_msg;
	const uint8_t* 		state;
	const uint8_t* 		msg;
	uint8_t 			msg_inv[64];
	hash_state 			md;
	uint32_t 			result;

	for (state = smIterator_get_first(&it_state, mem_read, sizeof(md.sha1.state)), result = 0; state != NULL; state = smIterator_get_next(&it_state, sizeof(md.sha1.state))){
		for (msg = smIterator_get_first(&it_msg, mem_read, 64); msg != NULL; msg = smIterator_get_next(&it_msg, 64)){

			/* Endianness 1 */
			memcpy(md.sha1.state, state, sizeof(md.sha1.state));
			md.sha1.curlen = 0;
			md.sha1.length = 0;

			if (sha1_process(&md, msg, 64) != CRYPT_OK){
				log_err("unable to process sha1");
				break;
			}

			if (searchableMemory_search(mem_writ, (uint8_t*)md.sha1.state, sizeof(md.sha1.state))){
				puts("sha1_compress                    | 1            | 0.0");
				result = 1;
				continue;
			}

			/* Endianness 2 */
			memcpy(md.sha1.state, state, sizeof(md.sha1.state));
			md.sha1.curlen = 0;
			md.sha1.length = 0;

			change_endianness((uint32_t*)msg_inv, (uint32_t*)msg, 16);
			if (sha1_process(&md, msg_inv, 64) != CRYPT_OK){
				log_err("unable to process sha1");
				break;
			}

			if (searchableMemory_search(mem_writ, (uint8_t*)md.sha1.state, sizeof(md.sha1.state))){
				puts("sha1_compress                    | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}

static uint32_t std_prim_search_aes128_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_ct;
	struct smIterator 	it_key;
	const uint8_t* 		pt;
	const uint8_t* 		key;
	uint8_t 			key_inv[16];
	symmetric_key 		skey;
	uint8_t 			ct[16];
	uint32_t 			result;

	for (pt = smIterator_get_first(&it_ct, mem_read, 16), result = 0; pt != NULL; pt = smIterator_get_next(&it_ct, 16)){
		for (key = smIterator_get_first(&it_key, mem_read, 16); key != NULL; key = smIterator_get_next(&it_key, 16)){

			/* Endianness 1 */
			if (aes_setup(key, 16, 10, &skey) != CRYPT_OK){
				log_err("unable to setup aes128 key");
				break;
			}

			if (aes_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes128");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("aes128_enc                       | 1            | 0.0");
				result = 1;
				continue;
			}

			/* Endianness 2 */
			change_endianness((uint32_t*)key_inv, (uint32_t*)key, 4);
			if (aes_setup(key_inv, sizeof(key_inv), 10, &skey) != CRYPT_OK){
				log_err("unable to setup aes128 key");
				break;
			}

			if (aes_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes128");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("aes128_enc                       | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}

static uint32_t std_prim_search_aes192_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_ct;
	struct smIterator 	it_key;
	const uint8_t* 		pt;
	const uint8_t* 		key;
	uint8_t 			key_inv[24];
	symmetric_key 		skey;
	uint8_t 			ct[16];
	uint32_t 			result;

	for (pt = smIterator_get_first(&it_ct, mem_read, 16), result = 0; pt != NULL; pt = smIterator_get_next(&it_ct, 16)){
		for (key = smIterator_get_first(&it_key, mem_read, 24); key != NULL; key = smIterator_get_next(&it_key, 24)){

			/* Endianness 1 */
			if (aes_setup(key, 24, 12, &skey) != CRYPT_OK){
				log_err("unable to setup aes192 key");
				break;
			}

			if (aes_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes192");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("aes192_enc                       | 1            | 0.0");
				result = 1;
				continue;
			}

			/* Endianness 2 */
			change_endianness((uint32_t*)key_inv, (uint32_t*)key, 6);
			if (aes_setup(key_inv, sizeof(key_inv), 12, &skey) != CRYPT_OK){
				log_err("unable to setup aes192 key");
				break;
			}

			if (aes_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes192");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("aes192_enc                       | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}

static uint32_t std_prim_search_aes256_enc(const struct searchableMemory* mem_read, const struct searchableMemory* mem_writ){
	struct smIterator 	it_ct;
	struct smIterator 	it_key;
	const uint8_t* 		pt;
	const uint8_t* 		key;
	uint8_t 			key_inv[32];
	symmetric_key 		skey;
	uint8_t 			ct[16];
	uint32_t 			result;

	for (pt = smIterator_get_first(&it_ct, mem_read, 16), result = 0; pt != NULL; pt = smIterator_get_next(&it_ct, 16)){
		for (key = smIterator_get_first(&it_key, mem_read, 32); key != NULL; key = smIterator_get_next(&it_key, 32)){

			/* Endianness 1 */
			if (aes_setup(key, 32, 14, &skey) != CRYPT_OK){
				log_err("unable to setup aes256 key");
				break;
			}

			if (aes_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes256");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("aes256_enc                       | 1            | 0.0");
				result = 1;
				continue;
			}

			/* Endianness 2 */
			change_endianness((uint32_t*)key_inv, (uint32_t*)key, 8);
			if (aes_setup(key_inv, sizeof(key_inv), 14, &skey) != CRYPT_OK){
				log_err("unable to setup aes256 key");
				break;
			}

			if (aes_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
				log_err("unable to encrypt aes256");
				break;
			}

			if (searchableMemory_search(mem_writ, ct, sizeof(ct))){
				puts("aes256_enc                       | 1            | 0.0");
				result = 1;
			}
		}
	}

	return result;
}
