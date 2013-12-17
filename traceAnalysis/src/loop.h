#ifndef LOOP_H
#define LOOP_H

#include <stdint.h>

#include "array.h"
#include "instruction.h"

#define LOOP_MINIMAL_CORE_LENGTH 3

struct loop{
	uint32_t 				offset;
	uint32_t 				length;
	uint32_t 				nb_iteration;
	uint32_t				epilogue;
};

struct loopToken{
	uint32_t 				offset;
	uint32_t 				length;
	uint32_t 				id;
	uint32_t 				iteration;
};

struct loopEngine{
	struct array 			element_array;
	struct loop* 			loops;
	uint32_t 				nb_loop;
};

struct loopEngine* loopEngine_create();
int32_t loopEngine_init(struct loopEngine* engine);

int32_t loopEngine_add(struct loopEngine* engine, struct instruction* instruction);
int32_t loopEngine_process(struct loopEngine* engine);
int32_t loopEngine_remove_redundant_loop(struct loopEngine* engine);
int32_t loopEngine_pack_epilogue(struct loopEngine* engine);
void loopEngine_print_loop(struct loopEngine* engine);

int32_t loopEngine_export_traceFragment(struct loopEngine* engine, struct array* array);

void loopEngine_clean(struct loopEngine* engine);
void loopEngine_delete(struct loopEngine* engine);

#endif