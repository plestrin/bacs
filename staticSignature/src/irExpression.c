#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irExpression.h"
#include "dagPartialOrder.h"
#include "set.h"
#include "base.h"

#define IREXPRESSION_AFFINE_FORM_MAX_LEVEL 6

struct irAffineTerm{
	uint64_t 		coef;
	uint32_t 		sign;
	struct node*	variable;
};

#define irAffineTerm_is_signed(affine_term) 		((affine_term)->sign == 1)
#define irAffineTerm_get_coef(affine_term, size)	((affine_term)->coef & (0xffffffffffffffffULL >> (64 - (size))))

static struct node* irAffineTerm_export(struct irAffineTerm* term, struct ir* ir, uint32_t size){
	uint64_t 		coef;
	struct node*	new_op = NULL;
	struct node* 	new_im;

	coef = irAffineTerm_get_coef(term, size);

	if (term->variable == NULL || coef == 0){
		return ir_add_immediate(ir, size, term->coef);
	}

	if (coef == 1){
		return term->variable;
	}
	else if ((coef & (0xffffffffffffffffULL >> (64 - size))) == (0xffffffffffffffffULL >> (64 - size)) && irAffineTerm_is_signed(term)){
		if ((new_op = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_NEG, IR_OPERATION_DST_UNKOWN)) != NULL){
			if (ir_add_dependence(ir, term->variable, new_op, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add depedence to IR");
			}
		}
		else{
			log_err("unable to add operation to IR");
		}
		return new_op;
	}
	else{
		if ((new_im = ir_add_immediate(ir, size, term->coef)) != NULL){
			if (irAffineTerm_is_signed(term)){
				new_op = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_IMUL, IR_OPERATION_DST_UNKOWN);
			}
			else{
				new_op = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, size, IR_MUL, IR_OPERATION_DST_UNKOWN);
			}
			if (new_op != NULL){
				if (ir_add_dependence(ir, term->variable, new_op, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add depedence to IR");
				}
				if (ir_add_dependence(ir, new_im, new_op, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add depedence to IR");
				}
			}
			else{
				log_err("unable to add operation to IR");
			}
		}
		else{
			log_err("unable to add immediate to IR");
		}
		return new_op;
	}
}

struct irAffineForm{
	uint8_t 		size;
	struct node* 	root;
	struct set 		term_set;
};

#define IR_AFFINE_FORM_NB_TERM 16

#define irAffineForm_get_size() (sizeof(struct irAffineForm) - sizeof(struct set) + set_get_size(sizeof(struct irAffineTerm), IR_AFFINE_FORM_NB_TERM))

static struct irAffineForm* irAffineForm_create(struct node* root){
	struct irAffineForm* affine_form;

	affine_form = (struct irAffineForm*)malloc(irAffineForm_get_size());
	if (affine_form != NULL){
		affine_form->root 			= root;
		affine_form->size 			= ir_node_get_operation(root)->size;
		set_init(&(affine_form->term_set), sizeof(struct irAffineTerm), IR_AFFINE_FORM_NB_TERM);
	}
	else{
		log_err("unable to allocate memory");
	}

	return affine_form;
}

#define irAffineForm_add_term(affine_form, term) 	set_add(&((affine_form)->term_set), term)

static int32_t irAffineForm_is_affine_root(struct node* node){
	struct irOperation* operation;

	operation = ir_node_get_operation(node);
	if (operation->type != IR_OPERATION_TYPE_INST){
		return 1;
	}
	
	switch(operation->operation_type.inst.opcode){
		case IR_ADD : {
			return 0;
		}
		case IR_IMUL :
		case IR_MUL : {
			uint32_t 		nb_var;
			struct edge* 	edge_cursor;

			for (edge_cursor = node_get_head_edge_dst(node), nb_var = 0; edge_cursor != NULL && nb_var < 2; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					nb_var ++;
				}
			}

			return (nb_var < 2) ? 0 : 1;
		}
		case IR_NEG : {
			return 1;
		}
		case IR_SHL : {
			struct edge* 	edge_cursor;

			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
					if (ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_IMM){
						return 0;
					}
					else{
						return 1;
					}
				}
			}

			return 1;
		}
		default : {
			break;
		}
	}

	return 1;
}

static inline void irAffineForm_import_imm(struct node* node, struct irAffineForm** affine_form, uint64_t coef, uint8_t sign);
static inline void irAffineForm_import_var(struct node* node, struct irAffineForm** affine_form, uint64_t coef, uint8_t sign);
static inline void irAffineForm_import_add(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign);
static inline void irAffineForm_import_imul(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign);
static inline void irAffineForm_import_mul(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign);
static inline void irAffineForm_import_neg(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef);
static inline void irAffineForm_import_shl(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign);

static int32_t irAffineForm_import(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign){
	if (*affine_form == NULL){
		*affine_form = irAffineForm_create(node);
		if (*affine_form == NULL){
			log_err("unable to create affineForm");
			return -1;
		}
	}

	if (level == IREXPRESSION_AFFINE_FORM_MAX_LEVEL){
		irAffineForm_import_var(node, affine_form, coef, sign);
		return 0;
	}

	switch(ir_node_get_operation(node)->type){
		case IR_OPERATION_TYPE_IN_REG 	: 
		case IR_OPERATION_TYPE_IN_MEM 	:
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			irAffineForm_import_var(node, affine_form, coef, sign);
			break;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			irAffineForm_import_imm(node, affine_form, coef, sign);
			break;
		}
		case IR_OPERATION_TYPE_INST 	: {
			switch(ir_node_get_operation(node)->operation_type.inst.opcode){
				case IR_ADD : {
					irAffineForm_import_add(node, affine_form, level, coef, sign);
					break;
				}
				case IR_IMUL : {
					irAffineForm_import_imul(node, affine_form, level, coef, sign);
					break;
				}
				case IR_MUL : {
					irAffineForm_import_mul(node, affine_form, level, coef, sign);
					break;
				}
				case IR_NEG : {
					irAffineForm_import_neg(node, affine_form, level, coef);
					break;
				}
				case IR_SHL : {
					irAffineForm_import_shl(node, affine_form, level, coef, sign);
					break;
				}
				default : {
					irAffineForm_import_var(node, affine_form, coef, sign);
					break;
				}
			}
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			irAffineForm_import_var(node, affine_form, coef, sign);
			break;
		}
	}

	return 0;
}

static inline void irAffineForm_import_imm(struct node* node, struct irAffineForm** affine_form, uint64_t coef, uint8_t sign){
	struct irAffineTerm term;

	if (sign){
		term.coef 	= (int64_t)coef * ir_imm_operation_get_signed_value(ir_node_get_operation(node));
	}
	else{
		term.coef 	= coef * ir_imm_operation_get_unsigned_value(ir_node_get_operation(node));
	}
	term.sign 		= sign;
	term.variable 	= NULL;

	if (irAffineForm_add_term(*affine_form, &term) < 0){
		log_err("unable to add affineTerm to affineForm");
	}
}

static inline void irAffineForm_import_var(struct node* node, struct irAffineForm** affine_form, uint64_t coef, uint8_t sign){
	struct irAffineTerm term;

	term.coef 		= coef;
	term.sign 		= sign;
	term.variable 	= node;

	if (irAffineForm_add_term(*affine_form, &term) < 0){
		log_err("unable to add affineTerm to affineForm");
	}
}

static inline void irAffineForm_import_add(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		irAffineForm_import(edge_get_src(edge_cursor), affine_form, level + 1, coef, sign);
	}
}

static inline void irAffineForm_import_imul(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign){
	int64_t 				local_coef 		= 1;
	struct node*			var 			= NULL;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irAffineTerm 	term;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

		if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
			local_coef = local_coef * ir_imm_operation_get_signed_value(operation_cursor);
		}
		else if (var == NULL){
			var = edge_get_src(edge_cursor);
		}
		else{
			irAffineForm_import_var(node, affine_form, coef, sign);
			return;
		}
	}

	if (var == NULL){
		term.coef 		= coef * local_coef;
		term.sign 		= sign;
		term.variable 	= NULL;

		if (irAffineForm_add_term(*affine_form, &term) < 0){
			log_err("unable to add affineTerm to affineForm");
		}
	}
	else{
		irAffineForm_import(var, affine_form, level + 1, coef * local_coef, sign);
	}
}

static inline void irAffineForm_import_mul(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign){
	uint64_t 				local_coef 		= 1;
	struct node*			var 			= NULL;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor;
	struct irAffineTerm 	term;

	if (sign){
		log_warn("multiplication applied on signed value");
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

		if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
			local_coef = local_coef * ir_imm_operation_get_unsigned_value(operation_cursor);
		}
		else if (var == NULL){
			var = edge_get_src(edge_cursor);
		}
		else{
			irAffineForm_import_var(node, affine_form, coef, sign);
			return;
		}
	}

	if (var == NULL){
		term.coef 		= coef * local_coef;
		term.sign 		= sign;
		term.variable 	= NULL;

		if (irAffineForm_add_term(*affine_form, &term) < 0){
			log_err("unable to add affineTerm to affineForm");
		}
	}
	else{
		irAffineForm_import(var, affine_form, level + 1, coef * local_coef, sign);
	}
}

static inline void irAffineForm_import_neg(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		irAffineForm_import(edge_get_src(edge_cursor), affine_form, level + 1, -coef, 1);
	}

}

static inline void irAffineForm_import_shl(struct node* node, struct irAffineForm** affine_form, uint32_t level, uint64_t coef, uint8_t sign){
	struct node* arg_dirc = NULL;
	struct node* arg_disp = NULL;
	struct edge* edge_cursor;

	if (sign){
		log_warn("multiplication applied on signed value");
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			arg_dirc = edge_get_src(edge_cursor);
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_IMM){
			arg_disp = edge_get_src(edge_cursor);
		}
	}

	if (arg_dirc != NULL && arg_disp != NULL){
		irAffineForm_import(arg_dirc, affine_form, level + 1, coef << ir_imm_operation_get_unsigned_value(ir_node_get_operation(arg_disp)), sign);
	}
	else{
		irAffineForm_import_var(node, affine_form, coef, sign);
	}
}

static int32_t irAffineForm_simplify(struct irAffineForm* affine_form){
	struct setIterator 		iterator_1;
	struct setIterator 		iterator_2;
	struct irAffineTerm* 	term_1;
	struct irAffineTerm* 	term_2;
	uint32_t 				result = 0;

	for (term_1 = (struct irAffineTerm*)setIterator_get_first(&(affine_form->term_set), &iterator_1); term_1 != NULL; term_1 = (struct irAffineTerm*)setIterator_get_next(&iterator_1)){
		memcpy(&iterator_2, &iterator_1, sizeof(struct setIterator));

		for (term_2 = (struct irAffineTerm*)setIterator_get_next(&iterator_2); term_2 != NULL; term_2 = (struct irAffineTerm*)setIterator_get_next(&iterator_2)){
			if (term_1->variable == term_2->variable){
				term_1->coef += term_2->coef;
				setIterator_pop(&iterator_2);
				if (term_1->variable != NULL){
					result = 1;
				}
			}
		}
	}

	return result;
}

static void irAffineForm_export(struct irAffineForm* affine_form, struct ir* ir){
	struct setIterator 		iterator;
	struct irAffineTerm* 	term;
	struct edge* 			edge_cursor;
	struct edge** 			edge_buffer;
	uint32_t 				i;
	uint32_t 				nb_edge;
	struct node* 			term_node;

	nb_edge = affine_form->root->nb_edge_dst;
	edge_buffer = (struct edge**)alloca(sizeof(struct edge*) * nb_edge);

	for (edge_cursor = node_get_head_edge_dst(affine_form->root), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		edge_buffer[i ++] = edge_cursor;
	}

	ir_node_get_operation(affine_form->root)->operation_type.inst.opcode = IR_ADD;
	ir_node_get_operation(affine_form->root)->size = affine_form->size;

	for (term = (struct irAffineTerm*)setIterator_get_first(&(affine_form)->term_set, &iterator); term != NULL; term = (struct irAffineTerm*)setIterator_get_next(&iterator)){
		term_node = irAffineTerm_export(term, ir, affine_form->size);
		if (term_node != NULL){
			if (ir_add_dependence(ir, term_node, affine_form->root, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add dependence to IR");
			}
		}
	}

	for (i = 0; i < nb_edge; i++){
		ir_remove_dependence(ir, edge_buffer[i]);
	}
}

void irAffineForm_print(struct irAffineForm* affine_form){
	struct setIterator 		iterator;
	struct irAffineTerm* 	term;

	for (term = (struct irAffineTerm*)setIterator_get_first(&(affine_form)->term_set, &iterator); term != NULL; term = (struct irAffineTerm*)setIterator_get_next(&iterator)){
		if (term->sign){
			printf("%lld x %p ", (int64_t)(term->coef), (void*)(term->variable));
		}
		else{
			printf("%llu x %p ", term->coef, (void*)(term->variable));
		}
	}
	printf("\n");
}

#define irAffineForm_delete(affine_form) 						\
	set_clean(&((affine_form)->term_set)); 						\
	free(affine_form);

void ir_normalize_affine_expression(struct ir* ir,  uint8_t* modification){
	struct node* node_cursor;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort DAG");
	}

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (!irAffineForm_is_affine_root(node_cursor)){
			struct irAffineForm* affine_form = NULL;

			if (!irAffineForm_import(node_cursor, &affine_form, 0, 1, 0)){
				if (irAffineForm_simplify(affine_form)){
					irAffineForm_export(affine_form, ir);
					*modification = 1;
				}
				irAffineForm_delete(affine_form)
			}
			else{
				log_err("unable to import affineForm");
			}
		}
	}
}