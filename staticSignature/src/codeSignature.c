#include <stdlib.h>
#include <stdio.h>

#include "codeSignature.h"
#include "multiColumn.h"

uint32_t irNode_get_label(struct node* node);
uint32_t signatureNode_get_label(struct node* node);

void codeSignature_dotPrint_node(void* data, FILE* file, void* arg);


struct codeSignatureCollection* codeSignature_create_collection(){
	struct codeSignatureCollection* collection;

	collection = (struct codeSignatureCollection*)malloc(sizeof(struct codeSignatureCollection));
	if (collection != NULL){
		if (codeSignature_init_collection(collection)){
			printf("ERROR: in %s, unable to init collection\n", __func__);
			free(collection);
			collection = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return collection;
}

int32_t codeSignature_add_signature_to_collection(struct codeSignatureCollection* collection, struct codeSignature* code_signature){
	struct codeSignature* 	new_signature;
	int32_t 				index;

	index = array_add(&(collection->signature_array), code_signature);
	if (index < 0){
		printf("ERROR: in %s, unable to add code signature to collection array\n", __func__);
		return -1;
	}
	else{
		new_signature = (struct codeSignature*)array_get(&(collection->signature_array), index);
		if (new_signature->sub_graph_handle == NULL){
			new_signature->sub_graph_handle = graphIso_create_sub_graph_handle(&(new_signature->graph), signatureNode_get_label);
		}
		else{
			new_signature->sub_graph_handle->graph = &(new_signature->graph);
		}
		graph_register_dotPrint_callback(&(new_signature->graph), NULL, codeSignature_dotPrint_node, NULL, NULL);
	}

	return 0;
}

void codeSignature_search(struct codeSignatureCollection* collection, struct ir* ir){
	uint32_t 				i;
	struct codeSignature* 	code_signature;
	struct graphIsoHandle* 	graph_handle;
	struct array* 			assignement_array;

	graph_handle = graphIso_create_graph_handle(&(ir->graph), irNode_get_label);
	if (graph_handle == NULL){
		printf("ERROR: in %s, unable to create graphHandle\n", __func__);
		return;
	}

	for (i = 0; i < array_get_length(&(collection->signature_array)); i++){
		code_signature = (struct codeSignature*)array_get(&(collection->signature_array), i);

		assignement_array = graphIso_search(graph_handle, code_signature->sub_graph_handle);
		if (assignement_array == NULL){
			printf("ERROR: in %s, the subgraph isomorphism routine fails\n", __func__);
		}
		else{
			/* a completer */

			if (array_get_length(assignement_array) > 0){
				printf("Found %u subgraph(s) instance for signature: %s\n", array_get_length(assignement_array), code_signature->name);
			}

			array_delete(assignement_array);
		}
	}

	graphIso_delete_graph_handle(graph_handle);
}

void codeSignature_empty_collection(struct codeSignatureCollection* collection){
	uint32_t 				i;
	struct codeSignature* 	code_signature;

	for (i = 0; i < array_get_length(&(collection->signature_array)); i++){
		code_signature = (struct codeSignature*)array_get(&(collection->signature_array), i);
		codeSignature_clean(code_signature);
	}

	array_empty(&(collection->signature_array));
}

void codeSignature_clean_collection(struct codeSignatureCollection* collection){
	uint32_t 				i;
	struct codeSignature* 	code_signature;

	for (i = 0; i < array_get_length(&(collection->signature_array)); i++){
		code_signature = (struct codeSignature*)array_get(&(collection->signature_array), i);
		codeSignature_clean(code_signature);
	}

	array_clean(&(collection->signature_array));
}

/* ===================================================================== */
/* Labelling routines													 */
/* ===================================================================== */

uint32_t signatureNode_get_label(struct node* node){
	struct signatureNode* signature_node = (struct signatureNode*)&(node->data);

	if (signature_node->opcode == IR_JOKER){
		return SUBGRAPHISOMORPHISM_JOKER_LABEL;
	}
	else{
		return (uint32_t)signature_node->opcode;
	}
}

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

/* ===================================================================== */
/* Printing routines													 */
/* ===================================================================== */

void codeSignature_printDot_collection(struct codeSignatureCollection* collection){
	uint32_t 					i;
	struct codeSignature* 		code_signature;
	struct multiColumnPrinter*	printer;
	char 						file_name[CODESIGNATURE_NAME_MAX_SIZE + 3];

	printer = multiColumnPrinter_create(stdout, 2, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, CODESIGNATURE_NAME_MAX_SIZE);
		multiColumnPrinter_set_column_size(printer, 1, CODESIGNATURE_NAME_MAX_SIZE + 3);
		multiColumnPrinter_set_title(printer, 0, "NAME");
		multiColumnPrinter_set_title(printer, 1, "FILE_NAME");

		multiColumnPrinter_print_header(printer);
	}

	for (i = 0; i < array_get_length(&(collection->signature_array)); i++){
		code_signature = (struct codeSignature*)array_get(&(collection->signature_array), i);

		snprintf(file_name, CODESIGNATURE_NAME_MAX_SIZE + 3, "%s.dot", code_signature->name);

		if (graphPrintDot_print(&(code_signature->graph), file_name, NULL)){
			printf("ERROR: in %s, graph printDot returned error code\n", __func__);
		}

		if (printer != NULL){
			multiColumnPrinter_print(printer, code_signature->name, file_name, NULL);
		}
	}

	if (printer != NULL){
		multiColumnPrinter_delete(printer);
	}
}


#pragma GCC diagnostic ignored "-Wunused-parameter"
void codeSignature_dotPrint_node(void* data, FILE* file, void* arg){
	struct signatureNode* node = (struct signatureNode*)data;

	if (node->input_number > 0){
		fprintf(file, "[label=\"%s\",shape=\"box\"]", irOpcode_2_string(node->opcode));
	}
	else if (node->output_number > 0){
		fprintf(file, "[label=\"%s\",shape=\"invhouse\"]", irOpcode_2_string(node->opcode));
	}
	else{
		fprintf(file, "[label=\"%s\"]", irOpcode_2_string(node->opcode));
	}
}