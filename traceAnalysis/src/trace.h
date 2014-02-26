#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#include "instruction.h"
#include "array.h" /* temp */
#include "multiColumn.h"

struct trace{
	struct _instruction* instructions;
	struct operand* 	operands;
	uint8_t* 			data;

	uint32_t 			map_size_ins;
	uint32_t 			map_size_op;
	uint32_t 			map_size_data;

	uint32_t 			nb_instruction;

	uint32_t 			reference_count;
};

struct trace* trace_create(struct array* array); /*temp*/
int32_t trace_init(struct trace* trace, struct array* array); /*temp*/

struct multiColumnPrinter* trace_create_multiColumnPrinter();
void trace_print(struct trace* trace, uint32_t start, uint32_t stop, struct multiColumnPrinter* printer);

/* a completer */

/* on peut ajouter une petite méthode de print sans prétention - juste pour vérifier que tout ce passe correctement et prendre le truc en main */

void trace_clean(struct trace* trace);
void trace_delete(struct trace* trace);

#define trace_get_reference(trace) (trace)->reference_count ++



/* faire un certain nombre de macro pour pas que ça soir trop chiant à manipuler comme structure */

/* pour une intégration continue du bouzin:
 * 4 - utiliser comme source pour les traceFragments
 * 5 - utiliser des tailles variables pour les accès memoire / registre dans l'extraction dans arguments - ou du moins identifier clairement là ou il faudrait interfacer un nouveau truc
 * 6 - faire une stratégie d'inport dynamique pour la trace (ccol mais pas indispensable)
 * 7 - écrire directement la trace dans le bon format (mesurer le gain en performance)
 * 8 - enregistrer directement la trace dans le bon format (mesurer le gain ou pas en perfomance)
 * 9 - prendre en charge toute les instructions rencontrées et les registres XMM MMX

 * - GOOD job, mais maintenant je récupère des arguments gros et pas cools (encore pas mal de plaisir en prévision )

Bien réfléchir à l'objet traceFragment - ce n'et pas aussi simple que ça.

 */



#endif