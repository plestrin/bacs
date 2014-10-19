#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "codeSignature.h"
#include "multiColumn.h"

uint32_t irNode_get_label(struct node* node);
uint32_t signatureNode_get_label(struct node* node);
uint32_t irEdge_get_label(struct edge* edge);
uint32_t signatureEdge_get_label(struct edge* edge);

void codeSignature_dotPrint_node(void* data, FILE* file, void* arg);
void codeSignature_dotPrint_edge(void* data, FILE* file, void* arg);

void syntaxGraph_dotPrint_node(void* data, FILE* file, void* arg);

static void codeSignature_recursive_search(struct node* syntax_node, struct ir* ir, struct graphIsoHandle** graph_handle, struct multiColumnPrinter* printer);
static int32_t codeSignature_is_occurence_in_ir(struct node** assignement, struct codeSignature* code_signature);
static void codeSignature_add_occurence_to_ir(struct ir* ir, struct node** assignement, struct codeSignature* code_signature);

static enum irDependenceType signatureNode_get_dependence_type(struct signatureNode* node);

struct codeSignatureCollection* codeSignature_create_collection(){
	struct codeSignatureCollection* collection;

	collection = (struct codeSignatureCollection*)malloc(sizeof(struct codeSignatureCollection));
	if (collection != NULL){
		codeSignature_init_collection(collection);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return collection;
}

int32_t codeSignature_add_signature_to_collection(struct codeSignatureCollection* collection, struct codeSignature* code_signature){
	struct node* 			syntax_node;
	struct node* 			node_cursor;
	struct codeSignature* 	new_signature;
	struct codeSignature* 	signature_cursor;
	uint32_t 				i;

	syntax_node = graph_add_node(&(collection->syntax_graph), code_signature);
	if (syntax_node == NULL){
		printf("ERROR: in %s, unable to add code signature to the collection's syntax graph\n", __func__);
		return -1;
	}

	new_signature = syntax_node_get_codeSignature(syntax_node);

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor != syntax_node){
			signature_cursor = syntax_node_get_codeSignature(node_cursor);

			if (!strncmp(new_signature->symbol, signature_cursor->symbol, CODESIGNATURE_NAME_MAX_SIZE)){
				new_signature->id = signature_cursor->id;
				break;
			}
		}
	}
	if (node_cursor == NULL){
		new_signature->id = codeSignature_get_new_id(collection);
	}

	if (new_signature->symbol_table != NULL){
		for (i = 0; i < new_signature->symbol_table->nb_symbol; i++){
			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = syntax_node_get_codeSignature(node_cursor);

				if (!strncmp(new_signature->symbol_table->symbols[i].name, signature_cursor->symbol, CODESIGNATURE_NAME_MAX_SIZE)){
					signatureSymbol_set_id(new_signature->symbol_table->symbols + i, signature_cursor->id);
					symbolTableEntry_set_resolved(new_signature->symbol_table->symbols[i].status);

					if (graph_add_edge(&(collection->syntax_graph), node_cursor, syntax_node, &i) == NULL){
						printf("ERROR: in %s, unable to add edge to the syntax tree\n", __func__);
					}
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);

		if (signature_cursor->symbol_table != NULL){
			for (i = 0; i < signature_cursor->symbol_table->nb_symbol; i++){
				if (!strncmp(signature_cursor->symbol_table->symbols[i].name, new_signature->symbol, CODESIGNATURE_NAME_MAX_SIZE)){
					signatureSymbol_set_id(signature_cursor->symbol_table->symbols + i, new_signature->id);
					symbolTableEntry_set_resolved(signature_cursor->symbol_table->symbols[i].status);

					if (graph_add_edge(&(collection->syntax_graph), syntax_node, node_cursor, &i) == NULL){
						printf("ERROR: in %s, unable to add edge to the syntax tree\n", __func__);
					}
				}
			}
		}
	}

	if (new_signature->sub_graph_handle == NULL){
		new_signature->sub_graph_handle = graphIso_create_sub_graph_handle(&(new_signature->graph), signatureNode_get_label, signatureEdge_get_label);
	}
	else{
		printf("WARNING: in %s, sub graph handle is already built, symbols are ignored\n", __func__);
		new_signature->sub_graph_handle->graph = &(new_signature->graph);
	}
	graph_register_dotPrint_callback(&(new_signature->graph), NULL, codeSignature_dotPrint_node, codeSignature_dotPrint_edge, NULL);
	
	return 0;
}

void codeSignature_search_collection(struct codeSignatureCollection* collection, struct ir** ir_buffer, uint32_t nb_ir){
	struct node* 				node_cursor;
	uint32_t 					i;
	uint32_t 					j;
	struct codeSignature* 		signature_cursor;
	struct graphIsoHandle* 		graph_handle;
	struct timespec 			timer_start_time;
	struct timespec 			timer_stop_time;
	struct multiColumnPrinter* 	printer;

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_DOUBLE);
		multiColumnPrinter_set_column_size(printer, 0, CODESIGNATURE_NAME_MAX_SIZE);
		multiColumnPrinter_set_title(printer, 0, "NAME");
		multiColumnPrinter_set_title(printer, 1, "FOUND");
		multiColumnPrinter_set_title(printer, 2, "TIME");

		multiColumnPrinter_print_header(printer);
	}
	else{
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		return;
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	for (i = 0, graph_handle = NULL; i < nb_ir; i++){
		for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			signature_cursor = syntax_node_get_codeSignature(node_cursor);
			signature_cursor->state = 0;

			if (signature_cursor->symbol_table != NULL){
				for (j = 0; j < signature_cursor->symbol_table->nb_symbol; j++){
					symbolTableEntry_set_not_found(signature_cursor->symbol_table->symbols[j].status);
				}
			}
		}

		for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			codeSignature_recursive_search(node_cursor, ir_buffer[i], &graph_handle, printer);
		}

		if (graph_handle != NULL){
			graphIso_delete_graph_handle(graph_handle);
			graph_handle = NULL;
		}
		multiColumnPrinter_print_horizontal_separator(printer);
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	multiColumnPrinter_delete(printer);
	printf("Total elapsed time: %f\n", (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.);
}

static void codeSignature_recursive_search(struct node* syntax_node, struct ir* ir, struct graphIsoHandle** graph_handle, struct multiColumnPrinter* printer){
	uint32_t 				i;
	struct codeSignature* 	code_signature;
	struct edge* 			edge_cursor;
	struct codeSignature*	child_code_signature;
	struct array* 			assignement_array;
	struct node**			assignement;
	struct timespec 		timer_start_time;
	struct timespec 		timer_stop_time;
	double 					timer_elapsed_time = 0.0;

	code_signature = syntax_node_get_codeSignature(syntax_node);
	if (codeSignature_state_is_search(code_signature)){
		return;
	}
	codeSignature_state_set_search(code_signature);

	for (edge_cursor = node_get_head_edge_dst(syntax_node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		child_code_signature = syntax_node_get_codeSignature(edge_get_src(edge_cursor));

		if (!codeSignature_state_is_search(child_code_signature)){
			codeSignature_recursive_search(edge_get_src(edge_cursor), ir, graph_handle, printer);
		}
		if (codeSignature_state_is_found(child_code_signature)){
			symbolTableEntry_set_found(code_signature->symbol_table->symbols[syntax_edge_get_index(edge_cursor)].status);
		}
	}

	if (code_signature->symbol_table != NULL){
		for (i = 0; i < code_signature->symbol_table->nb_symbol; i++){
			if (!symbolTableEntry_is_resolved(code_signature->symbol_table->symbols[i].status)){
				printf("WARNING: in %s, unresolved symbol: \"%s\", skip signature: \"%s\"\n", __func__, code_signature->symbol_table->symbols[i].name, code_signature->name);
				return;
			}

			if (!symbolTableEntry_is_found(code_signature->symbol_table->symbols[i].status)){
				return;
			}
		}
	}

	if (*graph_handle == NULL){
		*graph_handle = graphIso_create_graph_handle(&(ir->graph), irNode_get_label, irEdge_get_label);
		if (*graph_handle == NULL){
			printf("ERROR: in %s, unable to create graphHandle\n", __func__);
			return;
		}
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	assignement_array = graphIso_search(*graph_handle, code_signature->sub_graph_handle);

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}
	timer_elapsed_time = ((timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.);

	if (assignement_array == NULL){
		printf("ERROR: in %s, the subgraph isomorphism routine fails\n", __func__);
	}
	else if (array_get_length(assignement_array) > 0){
		codeSignature_state_set_found(code_signature);

		multiColumnPrinter_print(printer, code_signature->name, array_get_length(assignement_array), timer_elapsed_time, NULL);

		if (syntax_node->nb_edge_src > 0){
			for (i = 0; i < array_get_length(assignement_array); i++){
				assignement = (struct node**)array_get(assignement_array, i);
				if (!codeSignature_is_occurence_in_ir(assignement, code_signature)){
					codeSignature_add_occurence_to_ir(ir, assignement, code_signature);
				}
			}

			graphIso_delete_graph_handle(*graph_handle);
			*graph_handle = NULL;
		}
		array_delete(assignement_array);
	}
	else{
		array_delete(assignement_array);
	}
}

static int32_t codeSignature_is_occurence_in_ir(struct node** assignement, struct codeSignature* code_signature){
	uint32_t 				i;
	uint32_t 				j;
	enum irDependenceType 	dependence_type;
	struct node** 			candidats = NULL;
	uint32_t 				nb_candidat;
	struct edge* 			edge_cursor;
	struct irDependence* 	edge_data;
	struct signatureNode* 	sig_node;


	for (i = 0; i < code_signature->graph.nb_node; i++){
		sig_node = (struct signatureNode*)&(code_signature->sub_graph_handle->node_tab[i].node->data);

		if (sig_node->input_number > 0){
			dependence_type = signatureNode_get_dependence_type(sig_node);
			
			if (candidats == NULL){
				for (edge_cursor = node_get_head_edge_src(assignement[i]), nb_candidat = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					edge_data = ir_edge_get_dependence(edge_cursor);

					if (edge_data->type == dependence_type && ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_SYMBOL && ir_node_get_operation(edge_get_dst(edge_cursor))->operation_type.symbol.ptr == code_signature){
						nb_candidat ++;
					}
				}

				if (nb_candidat == 0){
					return 0;
				}
				candidats = (struct node**)malloc(sizeof(struct node*) * nb_candidat);
				if (candidats == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					return 0;
				}

				for (edge_cursor = node_get_head_edge_src(assignement[i]), nb_candidat = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					edge_data = ir_edge_get_dependence(edge_cursor);

					if (edge_data->type == dependence_type && ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_SYMBOL && ir_node_get_operation(edge_get_dst(edge_cursor))->operation_type.symbol.ptr == code_signature){
						candidats[nb_candidat ++] = edge_get_dst(edge_cursor);
					}
				}
			}
			else{
				for (j = 0; j < nb_candidat;){
					if (candidats[j] != NULL){
						for (edge_cursor = node_get_head_edge_src(assignement[i]); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
							edge_data = ir_edge_get_dependence(edge_cursor);

							if (edge_data->type == dependence_type && ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_SYMBOL && ir_node_get_operation(edge_get_dst(edge_cursor))->operation_type.symbol.ptr == code_signature && edge_get_dst(edge_cursor) == candidats[j]){
								break;
							}
						}

						if (edge_cursor == NULL){
							if (nb_candidat == 1){
								free(candidats);
								return 0;
							}
							else{
								candidats[j] = candidats[nb_candidat - 1];
								nb_candidat --;
							}
						}
						else{
							j ++;
						}
					}
					else{
						j ++;
					}
				}
			}
		}
		else if (sig_node->output_number > 0){
			dependence_type = signatureNode_get_dependence_type(sig_node);
			
			if (candidats == NULL){
				for (edge_cursor = node_get_head_edge_dst(assignement[i]), nb_candidat = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					edge_data = ir_edge_get_dependence(edge_cursor);

					if (edge_data->type == dependence_type && ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_SYMBOL && ir_node_get_operation(edge_get_src(edge_cursor))->operation_type.symbol.ptr == code_signature){
						nb_candidat ++;
					}
				}

				if (nb_candidat == 0){
					return 0;
				}
				candidats = (struct node**)malloc(sizeof(struct node*) * nb_candidat);
				if (candidats == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					return 0;
				}

				for (edge_cursor = node_get_head_edge_dst(assignement[i]), nb_candidat = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					edge_data = ir_edge_get_dependence(edge_cursor);

					if (edge_data->type == dependence_type && ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_SYMBOL && ir_node_get_operation(edge_get_src(edge_cursor))->operation_type.symbol.ptr == code_signature){
						candidats[nb_candidat ++] = edge_get_src(edge_cursor);
					}
				}
			}
			else{
				for (j = 0; j < nb_candidat;){
					if (candidats[j] != NULL){
						for (edge_cursor = node_get_head_edge_dst(assignement[i]); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							edge_data = ir_edge_get_dependence(edge_cursor);

							if (edge_data->type == dependence_type && ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_SYMBOL && ir_node_get_operation(edge_get_src(edge_cursor))->operation_type.symbol.ptr == code_signature && edge_get_src(edge_cursor) == candidats[j]){
								break;
							}
						}

						if (edge_cursor == NULL){
							if (nb_candidat == 1){
								free(candidats);
								return 0;
							}
							else{
								candidats[j] = candidats[nb_candidat - 1];
								nb_candidat --;
							}
						}
						else{
							j ++;
						}
					}
					else{
						j ++;
					}
				}
			}
		}
	}

	if (candidats != NULL){
		free(candidats);
	}

	return 1;
}

static void codeSignature_add_occurence_to_ir(struct ir* ir, struct node** assignement, struct codeSignature* code_signature){
	uint32_t 				i;
	struct node* 			ir_sym_node;
	struct signatureNode* 	sig_node;
	enum irDependenceType 	dependence_type;

	ir_sym_node = ir_add_symbol(ir, code_signature);
	if (ir_sym_node == NULL){
		printf("ERROR: in %s, unable to add symbolic node to IR\n", __func__);
		return;
	}

	for (i = 0; i < code_signature->graph.nb_node; i++){
		sig_node = (struct signatureNode*)&(code_signature->sub_graph_handle->node_tab[i].node->data);

		if (sig_node->input_number > 0){
			dependence_type = signatureNode_get_dependence_type(sig_node);
			if (ir_add_dependence(ir, assignement[i], ir_sym_node, dependence_type) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}

		if (sig_node->output_number > 0){
			dependence_type = signatureNode_get_dependence_type(sig_node);
			if (ir_add_dependence(ir, ir_sym_node, assignement[i], dependence_type) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}
	}
}

static enum irDependenceType signatureNode_get_dependence_type(struct signatureNode* node){
	if (node->input_number > 0){
		switch(node->input_number){
			case 1 : {
				switch(node->input_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_I1F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_I1F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_I1F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_I1F4;}
					default : {
						printf("ERROR: in %s, incorrect input frag order: %u\n", __func__, node->input_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
				break;
			}
			case 2 : {
				switch(node->input_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_I2F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_I2F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_I2F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_I2F4;}
					default : {
						printf("ERROR: in %s, incorrect input frag order: %u\n", __func__, node->input_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
				break;
			}
			case 3 : {
				switch(node->input_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_I3F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_I3F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_I3F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_I3F4;}
					default : {
						printf("ERROR: in %s, incorrect input frag order: %u\n", __func__, node->input_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
				break;
			}
			case 4 : {
				switch(node->input_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_I4F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_I4F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_I4F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_I4F4;}
					default : {
						printf("ERROR: in %s, incorrect input frag order: %u\n", __func__, node->input_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
				break;
			}
			case 5 : {
				switch(node->input_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_I5F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_I5F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_I5F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_I5F4;}
					default : {
						printf("ERROR: in %s, incorrect input frag order: %u\n", __func__, node->input_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
				break;
			}
			default : {
				printf("ERROR: in %s, incorrect input number: %u\n", __func__, node->input_number);
				return IR_DEPENDENCE_TYPE_DIRECT;
			}
		}
	}
	else if (node->output_number > 0){
		switch(node->output_number){
			case 1 : {
				switch(node->output_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_O1F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_O1F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_O1F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_O1F4;}
					default : {
						printf("ERROR: in %s, incorrect output frag order: %u\n", __func__, node->output_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
			}
			case 2 : {
				switch(node->output_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_O2F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_O2F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_O2F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_O2F4;}
					default : {
						printf("ERROR: in %s, incorrect output frag order: %u\n", __func__, node->output_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
			}
			case 3 : {
				switch(node->output_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_O3F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_O3F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_O3F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_O3F4;}
					default : {
						printf("ERROR: in %s, incorrect output frag order: %u\n", __func__, node->output_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
			}
			case 4 : {
				switch(node->output_frag_order){
					case 1 : {return IR_DEPENDENCE_TYPE_O4F1;}
					case 2 : {return IR_DEPENDENCE_TYPE_O4F2;}
					case 3 : {return IR_DEPENDENCE_TYPE_O4F3;}
					case 4 : {return IR_DEPENDENCE_TYPE_O4F4;}
					default : {
						printf("ERROR: in %s, incorrect output frag order: %u\n", __func__, node->output_frag_order);
						return IR_DEPENDENCE_TYPE_DIRECT;
					}
				}
			}
			default : {
				printf("ERROR: in %s, incorrect output number: %u\n", __func__, node->output_number);
				return IR_DEPENDENCE_TYPE_DIRECT;
			}
		}
	}
	else{
		return IR_DEPENDENCE_TYPE_DIRECT;
	}
}
void codeSignature_clean_collection(struct codeSignatureCollection* collection){
	struct node* 			node_cursor;
	struct codeSignature* 	signature_cursor;

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);
		codeSignature_clean(signature_cursor);
	}

	graph_clean(&(collection->syntax_graph));
}

/* ===================================================================== */
/* Labelling routines													 */
/* ===================================================================== */

uint32_t irNode_get_label(struct node* node){
	struct irOperation* operation = ir_node_get_operation(node);

	switch (operation->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			return 0xfffffffd;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			return IR_LOAD;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			return IR_STORE;
		}
		case IR_OPERATION_TYPE_INST 	: {
			return operation->operation_type.inst.opcode;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			return 0xfffffffe;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			return 0x0000ffff | (((struct codeSignature*)operation->operation_type.symbol.ptr)->id << 16);
		}
	}

	printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
	return 0;
}

uint32_t signatureNode_get_label(struct node* node){
	struct signatureNode* signature_node = (struct signatureNode*)&(node->data);

	switch(signature_node->type){
		case SIGNATURE_NODE_TYPE_OPCODE : {
			if (signature_node->node_type.opcode == IR_JOKER){
				return SUBGRAPHISOMORPHISM_JOKER_LABEL;
			}
			else{
				return (uint32_t)signature_node->node_type.opcode;
			}
		}
		case SIGNATURE_NODE_TYPE_SYMBOL : {
			return signature_node->node_type.symbol->id;
		}
	}

	printf("ERROR: in %s, this case is not supposed to happen (incorrect signature node type)\n", __func__);
	return SUBGRAPHISOMORPHISM_JOKER_LABEL;
}

uint32_t irEdge_get_label(struct edge* edge){
	struct irDependence* dependence = ir_edge_get_dependence(edge);

	if (dependence->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
		return IR_DEPENDENCE_TYPE_DIRECT;
	}

	return dependence->type;
}

uint32_t signatureEdge_get_label(struct edge* edge){
	struct signatureEdge* signature_edge = (struct signatureEdge*)&(edge->data);

	return signature_edge->type;
}

/* ===================================================================== */
/* Printing routines													 */
/* ===================================================================== */

void codeSignature_printDot_collection(struct codeSignatureCollection* collection){
	struct node* 				node_cursor;
	struct codeSignature* 		signature_cursor;
	struct multiColumnPrinter*	printer;
	char 						file_name[CODESIGNATURE_NAME_MAX_SIZE + 3];
	char 						symbol_str[10];

	graph_register_dotPrint_callback(&(collection->syntax_graph), NULL, syntaxGraph_dotPrint_node, NULL, NULL);
	printf("Print symbol dependency (syntax graph) in file: \"collection.dot\"\n");
	if (graphPrintDot_print(&(collection->syntax_graph), "collection.dot", NULL)){
		printf("ERROR: in %s, graph printDot returned error code\n", __func__);
	}

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, CODESIGNATURE_NAME_MAX_SIZE);
		multiColumnPrinter_set_column_size(printer, 1, CODESIGNATURE_NAME_MAX_SIZE);
		multiColumnPrinter_set_column_size(printer, 2, CODESIGNATURE_NAME_MAX_SIZE + 3);
		multiColumnPrinter_set_column_size(printer, 3, 10);

		multiColumnPrinter_set_title(printer, 0, "NAME");
		multiColumnPrinter_set_title(printer, 1, "SYMBOL");
		multiColumnPrinter_set_title(printer, 2, "FILE_NAME");
		multiColumnPrinter_set_title(printer, 3, "RESOLVE");

		multiColumnPrinter_print_header(printer);
	}

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);

		snprintf(file_name, CODESIGNATURE_NAME_MAX_SIZE + 3, "%s.dot", signature_cursor->name);

		if (signature_cursor->symbol_table == NULL){
			symbol_str[0] = '\0';
		}
		else{
			uint32_t j;
			uint32_t nb_resolved;

			for (j = 0, nb_resolved = 0; j < signature_cursor->symbol_table->nb_symbol; j++){
				if (symbolTableEntry_is_resolved(signature_cursor->symbol_table->symbols[j ].status)){
					nb_resolved ++;
				}
			}

			snprintf(symbol_str, 10, "%u/%u", nb_resolved, signature_cursor->symbol_table->nb_symbol);
		}

		if (graphPrintDot_print(&(signature_cursor->graph), file_name, NULL)){
			printf("ERROR: in %s, graph printDot returned error code\n", __func__);
		}

		if (printer != NULL){
			multiColumnPrinter_print(printer, signature_cursor->name, signature_cursor->symbol, file_name, symbol_str, NULL);
		}
	}

	if (printer != NULL){
		multiColumnPrinter_delete(printer);
	}
}


#pragma GCC diagnostic ignored "-Wunused-parameter"
void codeSignature_dotPrint_node(void* data, FILE* file, void* arg){
	struct signatureNode* node = (struct signatureNode*)data;

	switch (node->type){
		case SIGNATURE_NODE_TYPE_OPCODE : {
			if (node->input_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"box\"]", irOpcode_2_string(node->node_type.opcode), node->input_number, node->input_frag_order);
			}
			else if (node->output_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"invhouse\"]", irOpcode_2_string(node->node_type.opcode), node->output_number, node->output_frag_order);
			}
			else{
				fprintf(file, "[label=\"%s\"]", irOpcode_2_string(node->node_type.opcode));
			}
			break;
		}
		case SIGNATURE_NODE_TYPE_SYMBOL : {
			if (node->input_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"box\"]", node->node_type.symbol->name, node->input_number, node->input_frag_order);
			}
			else if (node->output_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"invhouse\"]", node->node_type.symbol->name, node->output_number, node->output_frag_order);
			}
			else{
				fprintf(file, "[label=\"%s\"]", node->node_type.symbol->name);
			}
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void codeSignature_dotPrint_edge(void* data, FILE* file, void* arg){
	struct signatureEdge* edge = (struct signatureEdge*)data;

	switch(edge->type){
		case IR_DEPENDENCE_TYPE_DIRECT 		: {
			break;
		}
		case IR_DEPENDENCE_TYPE_ADDRESS 	: {
			fprintf(file, "[label=\"@\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F1 		:{
			fprintf(file, "[label=\"I1F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F2 		:{
			fprintf(file, "[label=\"I1F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F3 		:{
			fprintf(file, "[label=\"I1F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F4 		:{
			fprintf(file, "[label=\"I1F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F1 		:{
			fprintf(file, "[label=\"I2F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F2 		:{
			fprintf(file, "[label=\"I2F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F3 		:{
			fprintf(file, "[label=\"I2F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F4 		:{
			fprintf(file, "[label=\"I2F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F1 		:{
			fprintf(file, "[label=\"I3F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F2 		:{
			fprintf(file, "[label=\"I3F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F3 		:{
			fprintf(file, "[label=\"I3F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F4 		:{
			fprintf(file, "[label=\"I3F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F1 		:{
			fprintf(file, "[label=\"I4F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F2 		:{
			fprintf(file, "[label=\"I4F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F3 		:{
			fprintf(file, "[label=\"I4F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F4 		:{
			fprintf(file, "[label=\"I4F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I5F1 		:{
			fprintf(file, "[label=\"I5F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I5F2 		:{
			fprintf(file, "[label=\"I5F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I5F3 		:{
			fprintf(file, "[label=\"I5F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I5F4 		:{
			fprintf(file, "[label=\"I5F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F1 		:{
			fprintf(file, "[label=\"O1F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F2 		:{
			fprintf(file, "[label=\"O1F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F3 		:{
			fprintf(file, "[label=\"O1F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F4 		:{
			fprintf(file, "[label=\"O1F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F1 		:{
			fprintf(file, "[label=\"O2F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F2 		:{
			fprintf(file, "[label=\"O2F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F3 		:{
			fprintf(file, "[label=\"O2F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F4 		:{
			fprintf(file, "[label=\"O2F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F1 		:{
			fprintf(file, "[label=\"O3F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F2 		:{
			fprintf(file, "[label=\"O3F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F3 		:{
			fprintf(file, "[label=\"O3F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F4 		:{
			fprintf(file, "[label=\"O3F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F1 		:{
			fprintf(file, "[label=\"O4F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F2 		:{
			fprintf(file, "[label=\"O4F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F3 		:{
			fprintf(file, "[label=\"O4F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F4 		:{
			fprintf(file, "[label=\"O4F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	:{
			fprintf(file, "[label=\"disp\"]");
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void syntaxGraph_dotPrint_node(void* data, FILE* file, void* arg){
	struct codeSignature* signature = (struct codeSignature*)data;

	fprintf(file, "[label=\"%s\"]", signature->name);
}