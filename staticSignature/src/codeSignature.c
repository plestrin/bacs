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

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);

		if (!strncmp(code_signature->name, signature_cursor->name, CODESIGNATURE_NAME_MAX_SIZE)){
			printf("ERROR: in %s, there is already a code signature in the collection with the name: \"%s\"\n", __func__, code_signature->name);
			return - 1;
		}
	}

	syntax_node = graph_add_node(&(collection->syntax_graph), code_signature);
	if (syntax_node == NULL){
		printf("ERROR: in %s, unable to add code signature to the collection's syntax graph\n", __func__);
		return -1;
	}

	new_signature = syntax_node_get_codeSignature(syntax_node);
	new_signature->id = codeSignature_get_new_id(collection);
	if (new_signature->sub_graph_handle == NULL){
		new_signature->sub_graph_handle = graphIso_create_sub_graph_handle(&(new_signature->graph), signatureNode_get_label, signatureEdge_get_label);
	}
	else{
		new_signature->sub_graph_handle->graph = &(new_signature->graph);
	}
	graph_register_dotPrint_callback(&(new_signature->graph), NULL, codeSignature_dotPrint_node, codeSignature_dotPrint_edge, NULL);

	if (new_signature->symbol_table != NULL){
		for (i = 0; i < new_signature->symbol_table->nb_symbol; i++){
			if (!signatureSymbol_is_resolved(new_signature->symbol_table->symbols + i)){
				for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
					signature_cursor = syntax_node_get_codeSignature(node_cursor);

					if (!strncmp(new_signature->symbol_table->symbols[i].name, signature_cursor->name, CODESIGNATURE_NAME_MAX_SIZE)){
						signatureSymbol_set_id(new_signature->symbol_table->symbols + i, signature_cursor->id);

						if (graph_add_edge_(&(collection->syntax_graph), node_cursor, syntax_node) == NULL){
							printf("ERROR: in %s, unable to add edge to the syntax tree\n", __func__);
						}
					}
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);

		if (signature_cursor->symbol_table != NULL){
			for (i = 0; i < signature_cursor->symbol_table->nb_symbol; i++){
				if (!signatureSymbol_is_resolved(signature_cursor->symbol_table->symbols + i)){
					if (!strncmp(signature_cursor->symbol_table->symbols[i].name, new_signature->name, CODESIGNATURE_NAME_MAX_SIZE)){
						signatureSymbol_set_id(signature_cursor->symbol_table->symbols + i, new_signature->id);

						if (graph_add_edge_(&(collection->syntax_graph), syntax_node, node_cursor) == NULL){
							printf("ERROR: in %s, unable to add edge to the syntax tree\n", __func__);
						}
					}
				}
			}
		}
	}
	
	return 0;
}

void codeSignature_search(struct codeSignatureCollection* collection, struct ir** ir_buffer, uint32_t nb_ir){
	struct node* 				node_cursor;
	uint32_t 					j;
	struct codeSignature* 		signature_cursor;
	struct graphIsoHandle* 		graph_handle;
	struct array* 				assignement_array;
	struct timespec 			timer_start_time;
	struct timespec 			timer_stop_time;
	double 						timer_elapsed_time = 0.0;
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

	for (j = 0; j < nb_ir; j++){
		graph_handle = graphIso_create_graph_handle(&(ir_buffer[j]->graph), irNode_get_label, irEdge_get_label);
		if (graph_handle == NULL){
			printf("ERROR: in %s, unable to create graphHandle\n", __func__);
			return;
		}

		for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			signature_cursor = syntax_node_get_codeSignature(node_cursor);

			assignement_array = graphIso_search(graph_handle, signature_cursor->sub_graph_handle);

			if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){
				printf("ERROR: in %s, clock_gettime fails\n", __func__);
			}
			timer_elapsed_time = ((timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.) - timer_elapsed_time;

			if (assignement_array == NULL){
				printf("ERROR: in %s, the subgraph isomorphism routine fails\n", __func__);
			}
			else{
				if (array_get_length(assignement_array) > 0){
					multiColumnPrinter_print(printer, signature_cursor->name, array_get_length(assignement_array), timer_elapsed_time, NULL);
				}

				array_delete(assignement_array);
			}
		}

		graphIso_delete_graph_handle(graph_handle);
		multiColumnPrinter_print_horizontal_separator(printer);
	}

	multiColumnPrinter_delete(printer);
	printf("Total elapsed time: %f\n", (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.);
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
			return IR_INPUT;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			return IR_INPUT;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			return 0xfffffffd;
		}
		case IR_OPERATION_TYPE_INST 	: {
			return operation->operation_type.inst.opcode;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			return 0xfffffffe;
		}
		default : {
			printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
			return 0;
		}
	}
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

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, CODESIGNATURE_NAME_MAX_SIZE);
		multiColumnPrinter_set_column_size(printer, 1, CODESIGNATURE_NAME_MAX_SIZE + 3);
		multiColumnPrinter_set_column_size(printer, 2, 10);

		multiColumnPrinter_set_title(printer, 0, "NAME");
		multiColumnPrinter_set_title(printer, 1, "FILE_NAME");
		multiColumnPrinter_set_title(printer, 2, "SYMBOL");

		multiColumnPrinter_print_header(printer);
	}

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);

		snprintf(file_name, CODESIGNATURE_NAME_MAX_SIZE + 3, "%s.dot", signature_cursor->name);

		if (signature_cursor->symbol_table == NULL){
			snprintf(symbol_str, 10, "0/0");
		}
		else{
			uint32_t j;
			uint32_t nb_resolved;

			for (j = 0, nb_resolved = 0; j < signature_cursor->symbol_table->nb_symbol; j++){
				if (signatureSymbol_is_resolved(signature_cursor->symbol_table->symbols + j)){
					nb_resolved ++;
				}
			}

			snprintf(symbol_str, 10, "%u/%u", nb_resolved, signature_cursor->symbol_table->nb_symbol);
		}

		if (graphPrintDot_print(&(signature_cursor->graph), file_name, NULL)){
			printf("ERROR: in %s, graph printDot returned error code\n", __func__);
		}

		if (printer != NULL){
			multiColumnPrinter_print(printer, signature_cursor->name, file_name, symbol_str, NULL);
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
			fprintf(file, "[label=\"%s\"", irOpcode_2_string(node->node_type.opcode));
			break;
		}
		case SIGNATURE_NODE_TYPE_SYMBOL : {
			fprintf(file, "[label=\"%s\"", node->node_type.symbol->name);
			break;
		}
	}

	if (node->input_number > 0){
		fprintf(file, ",shape=\"box\"]");
	}
	else if (node->output_number > 0){
		fprintf(file, ",shape=\"invhouse\"]");
	}
	else{
		fprintf(file, "]");
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void codeSignature_dotPrint_edge(void* data, FILE* file, void* arg){
	struct signatureEdge* edge = (struct signatureEdge*)data;

	switch(edge->type){
		case IR_DEPENDENCE_TYPE_DIRECT 	: {
			break;
		}
		case IR_DEPENDENCE_TYPE_ADDRESS : {
			fprintf(file, "[label=\"@\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F1 	:{
			fprintf(file, "[label=\"I1F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F2 	:{
			fprintf(file, "[label=\"I1F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F3 	:{
			fprintf(file, "[label=\"I1F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I1F4 	:{
			fprintf(file, "[label=\"I1F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F1 	:{
			fprintf(file, "[label=\"I2F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F2 	:{
			fprintf(file, "[label=\"I2F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F3 	:{
			fprintf(file, "[label=\"I2F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I2F4 	:{
			fprintf(file, "[label=\"I2F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F1 	:{
			fprintf(file, "[label=\"I3F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F2 	:{
			fprintf(file, "[label=\"I3F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F3 	:{
			fprintf(file, "[label=\"I3F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I3F4 	:{
			fprintf(file, "[label=\"I3F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F1 	:{
			fprintf(file, "[label=\"I4F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F2 	:{
			fprintf(file, "[label=\"I4F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F3 	:{
			fprintf(file, "[label=\"I4F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_I4F4 	:{
			fprintf(file, "[label=\"I4F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F1 	:{
			fprintf(file, "[label=\"O1F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F2 	:{
			fprintf(file, "[label=\"O1F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F3 	:{
			fprintf(file, "[label=\"O1F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O1F4 	:{
			fprintf(file, "[label=\"O1F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F1 	:{
			fprintf(file, "[label=\"O2F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F2 	:{
			fprintf(file, "[label=\"O2F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F3 	:{
			fprintf(file, "[label=\"O2F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O2F4 	:{
			fprintf(file, "[label=\"O2F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F1 	:{
			fprintf(file, "[label=\"O3F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F2 	:{
			fprintf(file, "[label=\"O3F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F3 	:{
			fprintf(file, "[label=\"O3F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O3F4 	:{
			fprintf(file, "[label=\"O3F4\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F1 	:{
			fprintf(file, "[label=\"O4F1\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F2 	:{
			fprintf(file, "[label=\"O4F2\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F3 	:{
			fprintf(file, "[label=\"O4F3\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_O4F4 	:{
			fprintf(file, "[label=\"O4F4\"]");
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void syntaxGraph_dotPrint_node(void* data, FILE* file, void* arg){
	struct codeSignature* signature = (struct codeSignature*)data;

	fprintf(file, "[label=\"%s\"]", signature->name);
}