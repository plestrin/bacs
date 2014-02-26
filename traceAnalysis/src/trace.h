#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#include "instruction.h"
#include "array.h" /* temp */
#include "multiColumn.h"

enum traceAllocation{
	TRACEALLOCATION_MALLOC,
	TRACEALLOCATION_MMAP
};

struct trace{
	struct _instruction* 	instructions;
	struct operand* 		operands;
	uint8_t* 				data;

	uint32_t 				alloc_size_ins;
	uint32_t 				alloc_size_op;
	uint32_t 				alloc_size_data;

	uint32_t 				nb_instruction;

	uint32_t 				reference_count;
	enum traceAllocation 	allocation_type;
};

struct trace* trace_create(struct array* array); /*temp*/
int32_t trace_init(struct trace* trace, struct array* array); /*temp*/

struct multiColumnPrinter* trace_create_multiColumnPrinter();
void trace_print(struct trace* trace, uint32_t start, uint32_t stop, struct multiColumnPrinter* printer);

int32_t trace_extract_segment(struct trace* trace_src, struct trace* trace_dst, uint32_t offset, uint32_t length);

void trace_clean(struct trace* trace);
void trace_delete(struct trace* trace);

#define trace_get_reference(trace) 					(trace)->reference_count ++
#define trace_get_ins_operands(trace, index) 		((trace)->operands + (trace)->instructions[(index)].operand_offset)
#define trace_get_ins_op_data(trace, i_ins, i_op)	((trace)->data + (trace)->operands[(trace)->instructions[(i_ins)].operand_offset + (i_op)].data_offset)


/* faire un certain nombre de macro pour pas que ça soir trop chiant à manipuler comme structure */

/* pour une intégration continue du bouzin:
 * 6 - écrire directement la trace dans le bon format (mesurer le gain en performance)
 * 7 - faire une stratégie d'import dynamique pour la trace (cool mais pas indispensable)
 * 8 - enregistrer directement la trace dans le bon format (mesurer le gain ou pas en perfomance)
 * 9 - prendre en charge toute les instructions rencontrées et les registres XMM MMX

 * - GOOD job, mais maintenant je récupère des arguments gros et pas cools (encore pas mal de plaisir en prévision )

Bien réfléchir à l'objet traceFragment - ce n'et pas aussi simple que ça.

 */



#endif