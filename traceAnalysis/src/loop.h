#ifndef LOOP_H
#define LOOP_H

#include <stdint.h>

#include "array.h"
#include "instruction.h"
#include "trace.h"

#define LOOP_MINIMAL_CORE_LENGTH 3

/* on va pouvoir faire le gros ménage dans cette section - attention avant de tout peter faire des mesure pour vérifier que tous ce passe bien */

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
	struct trace* 			trace;
	struct loop* 			loops;
	uint32_t 				nb_loop;
};

struct loopEngine* loopEngine_create(struct trace* trace);
int32_t loopEngine_init(struct loopEngine* engine, struct trace* trace);

int32_t loopEngine_process(struct loopEngine* engine);
int32_t loopEngine_remove_redundant_loop(struct loopEngine* engine);
int32_t loopEngine_pack_epilogue(struct loopEngine* engine);
void loopEngine_print_loop(struct loopEngine* engine);

int32_t loopEngine_export_it(struct loopEngine* engine, struct array* frag_array, uint32_t loop_index, uint32_t iteration_index);
int32_t loopEngine_export_all(struct loopEngine* engine, struct array* frag_array, int32_t loop_index);
int32_t loopEngine_export_noEp(struct loopEngine* engine, struct array* frag_array, int32_t loop_index);

void loopEngine_clean(struct loopEngine* engine);
void loopEngine_delete(struct loopEngine* engine);

#endif