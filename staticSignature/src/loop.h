#ifndef LOOP_H
#define LOOP_H

#include <stdint.h>

#include "array.h"
#include "instruction.h"
#include "trace.h"

#define LOOP_MINIMAL_CORE_LENGTH 	3 		/* Min length of the iteration body: must be larger or equal than one */
#define LOOP_MAXIMAL_CORE_LENGTH 	500 	/* Max length of the iteration body */
#define LOOP_MINIMAL_NB_ITERATION 	2 		/* Min number of iteration */

struct loop{
	uint32_t 		offset;
	uint32_t 		length;
	uint32_t 		nb_iteration;
	uint32_t		epilogue;
};

struct loopToken{
	uint32_t 		offset;
	uint32_t 		length;
	uint32_t 		nb_iteration;
};

struct loopEngine{
	struct trace* 	trace;
	struct loop* 	loops;
	uint32_t 		nb_loop;
};

struct loopEngine* loopEngine_create(struct trace* trace);
int32_t loopEngine_init(struct loopEngine* engine, struct trace* trace);

int32_t loopEngine_process_strict(struct loopEngine* engine);
int32_t loopEngine_process_norder(struct loopEngine* engine);

int32_t loopEngine_remove_redundant_loop_strict(struct loopEngine* engine);
int32_t loopEngine_remove_redundant_loop_packed(struct loopEngine* engine);
int32_t loopEngine_remove_redundant_loop_nested(struct loopEngine* engine);
void loopEngine_print_loop(struct loopEngine* engine);

int32_t loopEngine_export_it(struct loopEngine* engine, struct array* frag_array, uint32_t loop_index, uint32_t iteration_index);
int32_t loopEngine_export_all(struct loopEngine* engine, struct array* frag_array, int32_t loop_index);
int32_t loopEngine_export_noEp(struct loopEngine* engine, struct array* frag_array, int32_t loop_index);

void loopEngine_clean(struct loopEngine* engine);
void loopEngine_delete(struct loopEngine* engine);

#endif