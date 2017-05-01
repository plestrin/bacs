#ifndef ASSEMBLYSCAN_H
#define ASSEMBLYSCAN_H

#include "assembly.h"
#include "codeMap.h"

#define ASSEMBLYSCAN_FILTER_BBL_SIZE 	0x00000001
#define ASSEMBLYSCAN_FILTER_BBL_RATIO 	0x00000002
#define ASSEMBLYSCAN_FILTER_BBL_EXEC 	0x00000004
#define ASSEMBLYSCAN_FILTER_FUNC_LEAF 	0x00000008
#define ASSEMBLYSCAN_FILTER_CST 		0x00000010
#define ASSEMBLYSCAN_FILTER_VERBOSE 	0x80000000

void assemblyScan_scan(struct assembly* assembly, void* call_graph, struct codeMap* cm, uint32_t filters);

uint32_t* assemblyScan_save_block_id_and_trim(struct assembly* assembly);
void assemblyScan_restore_block_id_and_free(struct assembly* assembly, uint32_t* block_id);

#endif
