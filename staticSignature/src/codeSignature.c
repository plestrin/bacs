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

static void signatureOcurrence_init(struct codeSignature* code_signature, struct array* assignement_array);
static void signatureOcurrence_push(struct ir* ir, struct codeSignature* code_signature);
static void signatureOcurrence_pop(struct ir* ir, struct codeSignature* code_signature);

#if VERBOSE == 1

#define MAX_CLASS_THRESHOLD 2

enum parameterSimilarity{
	PARAMETER_EQUAL 	= 0x00000001,
	PARAMETER_PERMUT 	= 0x00000003,
	PARAMETER_OVERLAP 	= 0x00000007,
	PARAMETER_DISJOINT 	= 0x0000000f
};

#define parameterMapping_get_size(code_signature) (sizeof(struct parameterMapping) * ((code_signature)->nb_parameter_in + (code_signature)->nb_parameter_out) + sizeof(struct node*) * ((code_signature)->nb_frag_tot_in + (code_signature)->nb_frag_tot_out))
#define parameterMapping_get_node_buffer(mapping) ((struct node**)((char*)(mapping) + (mapping)->node_buffer_offset))

struct parameterMapping{
	uint32_t 					nb_fragment;
	size_t 						node_buffer_offset;
	enum parameterSimilarity 	similarity;
};

static enum parameterSimilarity signatureOccurence_find_parameter_similarity(struct node** parameter_list1, struct node** parameter_list2, uint32_t size);
static int32_t signatureOccurence_try_append_to_class(struct parameterMapping* class, struct parameterMapping* new_mapping, uint32_t nb_in, uint32_t nb_out);
static void signatureOccurence_print_location(struct parameterMapping* parameter);

static void signatureOccurence_print(struct codeSignature* codeSignature);

#endif

static void signatureOccurence_clean(struct codeSignature* code_signature);

/* ===================================================================== */
/* Code Signature Collection routines									 */
/* ===================================================================== */

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
	struct signatureNode* 	sig_node_cursor;
	uint32_t 				nb_unresolved_symbol;


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
		for (i = 0, nb_unresolved_symbol = 0; i < new_signature->symbol_table->nb_symbol; i++){
			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = syntax_node_get_codeSignature(node_cursor);

				if (!strncmp(new_signature->symbol_table->symbols[i].name, signature_cursor->symbol, CODESIGNATURE_NAME_MAX_SIZE)){
					signatureSymbol_set_id(new_signature->symbol_table->symbols + i, signature_cursor->id);
					symbolTableEntry_set_resolved(new_signature->symbol_table->symbols + i);

					if (graph_add_edge(&(collection->syntax_graph), node_cursor, syntax_node, &i) == NULL){
						printf("ERROR: in %s, unable to add edge to the syntax tree\n", __func__);
					}
				}
			}
			if (node_cursor == NULL){
				nb_unresolved_symbol ++;
			}
		}
	}
	else{
		nb_unresolved_symbol = 0;
	}

	if (new_signature->sub_graph_handle == NULL){
		if (nb_unresolved_symbol == 0){
			new_signature->sub_graph_handle = graphIso_create_sub_graph_handle(&(new_signature->graph), signatureNode_get_label, signatureEdge_get_label);
		}
	}
	else{
		printf("WARNING: in %s, sub graph handle is already built, symbols are ignored\n", __func__);
		new_signature->sub_graph_handle->graph = &(new_signature->graph);
	}

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);

		if (signature_cursor->symbol_table != NULL){
			for (i = 0, nb_unresolved_symbol = 0; i < signature_cursor->symbol_table->nb_symbol; i++){
				if (!strncmp(signature_cursor->symbol_table->symbols[i].name, new_signature->symbol, CODESIGNATURE_NAME_MAX_SIZE)){
					signatureSymbol_set_id(signature_cursor->symbol_table->symbols + i, new_signature->id);
					symbolTableEntry_set_resolved(signature_cursor->symbol_table->symbols + i);

					if (graph_add_edge(&(collection->syntax_graph), syntax_node, node_cursor, &i) == NULL){
						printf("ERROR: in %s, unable to add edge to the syntax tree\n", __func__);
					}
				}
				else if (!symbolTableEntry_is_resolved(signature_cursor->symbol_table->symbols + i)){
					nb_unresolved_symbol ++;
				}
			}

			if (nb_unresolved_symbol == 0 && signature_cursor->sub_graph_handle == NULL){
				signature_cursor->sub_graph_handle = graphIso_create_sub_graph_handle(&(signature_cursor->graph), signatureNode_get_label, signatureEdge_get_label);
			}
		}
	}

	graph_register_dotPrint_callback(&(new_signature->graph), NULL, codeSignature_dotPrint_node, codeSignature_dotPrint_edge, NULL);

	new_signature->occurence.nb_occurence 	= 0;
	new_signature->occurence.input_buffer 	= NULL;
	new_signature->occurence.output_buffer 	= NULL;
	new_signature->occurence.node_buffer 	= NULL;

	new_signature->nb_parameter_in 	= 0;
	new_signature->nb_parameter_out = 0;
	new_signature->nb_frag_tot_in 	= 0;
	new_signature->nb_frag_tot_out 	= 0;

	for (node_cursor = graph_get_head_node(&(new_signature->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		sig_node_cursor = (struct signatureNode*)&(node_cursor->data);
		if (sig_node_cursor->input_number > 0){
			new_signature->nb_frag_tot_in ++;
			if (sig_node_cursor->input_number > new_signature->nb_parameter_in){
				new_signature->nb_parameter_in = sig_node_cursor->input_number;
			}
		}
		if (sig_node_cursor->output_number > 0){
			new_signature->nb_frag_tot_out ++;
			if (sig_node_cursor->output_number > new_signature->nb_parameter_out){
				new_signature->nb_parameter_out = sig_node_cursor->output_number;
			}
		}
	}

	if (new_signature->nb_parameter_in == 0 || new_signature->nb_parameter_in == 0){
		printf("ERROR: in %s, the signature has an incorrect number of parameter\n", __func__);
	}
	
	return 0;
}

void codeSignature_search_collection(struct codeSignatureCollection* collection, struct ir** ir_buffer, uint32_t nb_ir){
	struct node* 				node_cursor;
	uint32_t 					i;
	uint32_t 					j;
	struct edge* 				edge_cursor;
	struct codeSignature* 		signature_cursor;
	struct codeSignature*		child;
	struct codeSignature**		signature_buffer;
	uint32_t 					nb_signature;
	struct graphIsoHandle* 		graph_handle;
	struct timespec 			timer1_start_time;
	struct timespec 			timer1_stop_time;
	struct timespec 			timer2_start_time;
	struct timespec 			timer2_stop_time;
	double 						timer2_elapsed_time;
	struct multiColumnPrinter* 	printer;
	struct array* 				assignement_array;
	uint32_t 					found;

	signature_buffer = (struct codeSignature**)malloc(sizeof(struct signature*) * collection->syntax_graph.nb_node);
	if (signature_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

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
		free(signature_buffer);
		return;
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer1_start_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	for (i = 0; i < nb_ir; i++){
		for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			signature_cursor = syntax_node_get_codeSignature(node_cursor);
			signature_cursor->state = 0;
		}

		do{
			nb_signature = 0;

			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = syntax_node_get_codeSignature(node_cursor);
				if (codeSignature_state_is_search(signature_cursor)){
					goto next1;
				}

				if (node_cursor->nb_edge_dst){
					for (edge_cursor = node_get_head_edge_dst(node_cursor), found = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						child = syntax_node_get_codeSignature(edge_get_src(edge_cursor));
						if (!codeSignature_state_is_search(child)){
							goto next1;
						}
						else if (codeSignature_state_is_found(child)){
							found = 1;
						}
					}
					if (!found){
						codeSignature_state_set_search(signature_cursor);
						goto next1;
					}
				}

				if (signature_cursor->sub_graph_handle == NULL){
					printf("WARNING: in %s, sub_graph_handle is still NULL for %s. It may be due to unresolved symbol(s)\n", __func__, signature_cursor->name);
					goto next1;
				}

				signature_buffer[nb_signature ++] = signature_cursor;

				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					child = syntax_node_get_codeSignature(edge_get_src(edge_cursor));
					if (codeSignature_state_is_found(child)){
						if (!codeSignature_state_is_pushed(child)){
							signatureOcurrence_push(ir_buffer[i], child);
						}
					}
				}

				next1:;
			}

			graph_handle = graphIso_create_graph_handle(&(ir_buffer[i]->graph), irNode_get_label, irEdge_get_label);
			if (graph_handle == NULL){
				printf("ERROR: in %s, unable to create graphHandle\n", __func__);
				break;
			}

			for (j = 0; j < nb_signature; j++){
				if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer2_start_time)){
					printf("ERROR: in %s, clock_gettime fails\n", __func__);
				}

				assignement_array = graphIso_search(graph_handle, signature_buffer[j]->sub_graph_handle);

				if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer2_stop_time)){
					printf("ERROR: in %s, clock_gettime fails\n", __func__);
				}
				timer2_elapsed_time = ((timer2_stop_time.tv_sec - timer2_start_time.tv_sec) + (timer2_stop_time.tv_nsec - timer2_start_time.tv_nsec) / 1000000000.);

				codeSignature_state_set_search(signature_buffer[j]);
				if (assignement_array == NULL){
					printf("ERROR: in %s, the subgraph isomorphism routine fails\n", __func__);
				}
				else if (array_get_length(assignement_array) > 0){
					signatureOcurrence_init(signature_buffer[j], assignement_array);
					multiColumnPrinter_print(printer, signature_buffer[j]->name, array_get_length(assignement_array), timer2_elapsed_time, NULL);
					array_delete(assignement_array);
					#if VERBOSE == 1
					signatureOccurence_print(signature_buffer[j]);
					#endif
				}
				else{
					array_delete(assignement_array);
				}
			}

			graphIso_delete_graph_handle(graph_handle);

			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = syntax_node_get_codeSignature(node_cursor);
				if (!codeSignature_state_is_search(signature_cursor) || !codeSignature_state_is_found(signature_cursor) || !codeSignature_state_is_pushed(signature_cursor)){
					goto next2;
				}

				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					child = syntax_node_get_codeSignature(edge_get_dst(edge_cursor));
					if (!codeSignature_state_is_search(child)){
						goto next2;
					}
				}

				signatureOcurrence_pop(ir_buffer[i], signature_cursor);

				next2:;
			}

		} while(nb_signature);
		multiColumnPrinter_print_horizontal_separator(printer);

		for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			signature_cursor = syntax_node_get_codeSignature(node_cursor);
			signatureOccurence_clean(signature_cursor);
		}
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer1_stop_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	multiColumnPrinter_delete(printer);
	printf("Total elapsed time: %f\n", (timer1_stop_time.tv_sec - timer1_start_time.tv_sec) + (timer1_stop_time.tv_nsec - timer1_start_time.tv_nsec) / 1000000000.);

	free(signature_buffer);
}

static void signatureOcurrence_init(struct codeSignature* code_signature, struct array* assignement_array){
	uint32_t 				i;
	uint32_t 				j;
	struct node**			assignement;
	uint32_t 				nb_input;
	uint32_t 				nb_output;
	struct signatureNode* 	sig_node;

	if (code_signature->occurence.nb_occurence || code_signature->occurence.node_buffer != NULL || code_signature->occurence.input_buffer != NULL || code_signature->occurence.output_buffer != NULL){
		printf("WARNING: in %s, the signatureOccurence structure has not been cleaned or initialized correctly\n", __func__);
	}

	code_signature->occurence.nb_occurence = array_get_length(assignement_array);
	code_signature->occurence.input_buffer = (struct signatureLink*)malloc(sizeof(struct signatureLink) * code_signature->occurence.nb_occurence * code_signature->nb_frag_tot_in);
	code_signature->occurence.output_buffer = (struct signatureLink*)malloc(sizeof(struct signatureLink) * code_signature->occurence.nb_occurence * code_signature->nb_frag_tot_out);
	code_signature->occurence.node_buffer = (struct node**)calloc(code_signature->occurence.nb_occurence, sizeof(struct signatureLink));

	if (code_signature->occurence.node_buffer == NULL || code_signature->occurence.input_buffer == NULL || code_signature->occurence.output_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	
		code_signature->occurence.nb_occurence = 0;
		if (code_signature->occurence.input_buffer != NULL){
			free(code_signature->occurence.input_buffer);
			code_signature->occurence.input_buffer = NULL;
		}
		if (code_signature->occurence.output_buffer != NULL){
			free(code_signature->occurence.output_buffer);
			code_signature->occurence.output_buffer = NULL;
		}
		if (code_signature->occurence.node_buffer != NULL){
			free(code_signature->occurence.node_buffer);
			code_signature->occurence.node_buffer = NULL;
		}

		return;
	}

	for (i = 0; i < code_signature->occurence.nb_occurence; i++){
		assignement = (struct node**)array_get(assignement_array, i);
		nb_input = 0;
		nb_output = 0;

		for (j = 0; j < code_signature->graph.nb_node; j++){
			sig_node = (struct signatureNode*)&(code_signature->sub_graph_handle->node_tab[j].node->data);

			if (sig_node->input_number > 0){
				code_signature->occurence.input_buffer[i * code_signature->nb_frag_tot_in + nb_input].node = assignement[j];
				code_signature->occurence.input_buffer[i * code_signature->nb_frag_tot_in + nb_input].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_INPUT(sig_node->input_frag_order, sig_node->input_number);
				nb_input ++;
			}

			if (sig_node->output_number > 0){
				code_signature->occurence.output_buffer[i * code_signature->nb_frag_tot_out + nb_output].node = assignement[j];
				code_signature->occurence.output_buffer[i * code_signature->nb_frag_tot_out + nb_output].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(sig_node->output_frag_order, sig_node->output_number);
				nb_output ++;
			}
		}
	}

	codeSignature_state_set_found(code_signature);
}

static void signatureOcurrence_push(struct ir* ir, struct codeSignature* code_signature){
	uint32_t i;
	uint32_t j;

	codeSignature_state_set_pushed(code_signature);
	for (i = 0; i < code_signature->occurence.nb_occurence; i++){
		code_signature->occurence.node_buffer[i] = ir_add_symbol(ir, code_signature);
		if (code_signature->occurence.node_buffer[i] == NULL){
			printf("ERROR: in %s, unable to add symbolic node to IR\n", __func__);
			continue;
		}

		for (j = 0; j < code_signature->nb_frag_tot_in; j++){
			if (ir_add_macro_dependence(ir, code_signature->occurence.input_buffer[i * code_signature->nb_frag_tot_in + j].node, code_signature->occurence.node_buffer[i], code_signature->occurence.input_buffer[i * code_signature->nb_frag_tot_in + j].edge_desc) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}

		for (j = 0; j < code_signature->nb_frag_tot_out; j++){
			if (ir_add_macro_dependence(ir, code_signature->occurence.node_buffer[i], code_signature->occurence.output_buffer[i * code_signature->nb_frag_tot_out + j].node, code_signature->occurence.output_buffer[i * code_signature->nb_frag_tot_out + j].edge_desc) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}

	}
}

static void signatureOcurrence_pop(struct ir* ir, struct codeSignature* code_signature){
	uint32_t i;

	codeSignature_state_set_poped(code_signature);
	for (i = 0; i < code_signature->occurence.nb_occurence; i++){
		if (code_signature->occurence.node_buffer[i] != NULL){
			ir_remove_node(ir, code_signature->occurence.node_buffer[i]);	
		}
		code_signature->occurence.node_buffer[i] = NULL;
	}
}

#if VERBOSE == 1

static enum parameterSimilarity signatureOccurence_find_parameter_similarity(struct node** parameter_list1, struct node** parameter_list2, uint32_t size){
	uint32_t 					i;
	uint32_t 					j;
	enum parameterSimilarity 	result = PARAMETER_EQUAL;
	uint32_t 					nb_found = 0;

	for (i = 0; i < size; i++){
		for (j = 0; j < size; j++){
			if (parameter_list1[i] == parameter_list2[j]){
				nb_found ++;
				if (i != j){
					result |= 0x00000002;
				}
				break;
			}
		}
	}

	if (nb_found == 0){
		result = PARAMETER_DISJOINT;
	}
	else if (nb_found != size){
		result = PARAMETER_OVERLAP;
	}

	return result;
}

static int32_t signatureOccurence_try_append_to_class(struct parameterMapping* class, struct parameterMapping* new_mapping, uint32_t nb_in, uint32_t nb_out){
	uint32_t 					i;
	enum parameterSimilarity* 	similarity;

	similarity = (enum parameterSimilarity*)alloca(sizeof(enum parameterSimilarity) * (nb_in + nb_out));

	for (i = 0; i < nb_in; i++){
		similarity[i] = signatureOccurence_find_parameter_similarity(parameterMapping_get_node_buffer(class + i), parameterMapping_get_node_buffer(new_mapping + i), class[i].nb_fragment);
		if (similarity[i] == PARAMETER_OVERLAP || similarity[i] == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < nb_out; i++){
		similarity[nb_in + i] = signatureOccurence_find_parameter_similarity(parameterMapping_get_node_buffer(class + nb_in + i), parameterMapping_get_node_buffer(new_mapping + nb_in + i), class[nb_in + i].nb_fragment);
		if (similarity[nb_in + i] == PARAMETER_OVERLAP || similarity[nb_in + i] == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < nb_in + nb_out; i++){
		class[i].similarity |= similarity[i];
	}

	return 0;
}

static void signatureOccurence_print_location(struct parameterMapping* parameter){
	uint32_t 			i;
	struct irOperation* operation;
	uint8_t 			is_input_buffer;
	struct node* 		address;
	struct node* 		base;
	uint64_t 			buffer_start_offset;
	uint8_t* 			buffer;
	uint8_t 			buffer_access_size;
	uint64_t 			offset;

	if (parameter->nb_fragment == 0){
		return;
	}

	buffer = alloca(sizeof(uint8_t) * (2 * parameter->nb_fragment - 1));
	memset(buffer, 0, sizeof(uint8_t) * (2 * parameter->nb_fragment - 1));

	for (i = 0, is_input_buffer = 1, base = NULL; i < parameter->nb_fragment && is_input_buffer == 1; i++){
		operation = ir_node_get_operation(parameterMapping_get_node_buffer(parameter)[i]);
		if (operation->type == IR_OPERATION_TYPE_IN_MEM){
			address = edge_get_src(node_get_head_edge_dst(parameterMapping_get_node_buffer(parameter)[i]));
			if (ir_node_get_operation(address)->type == IR_OPERATION_TYPE_INST && ir_node_get_operation(address)->operation_type.inst.opcode == IR_ADD && address->nb_edge_dst == 2){
				struct node* op1;
				struct node* op2;

				op1 = edge_get_src(node_get_head_edge_dst(address));
				if (ir_node_get_operation(op1)->type != IR_OPERATION_TYPE_IMM){
					op2 = edge_get_src(edge_get_next_dst(node_get_head_edge_dst(address)));
				}
				else{
					op2 = op1;
					op1 = edge_get_src(edge_get_next_dst(node_get_head_edge_dst(address)));
				}

				if (ir_node_get_operation(op1)->type != IR_OPERATION_TYPE_IMM && ir_node_get_operation(op2)->type == IR_OPERATION_TYPE_IMM){
					if (base == NULL || (base == op1 && buffer_access_size == ir_node_get_operation(parameterMapping_get_node_buffer(parameter)[i])->size)){
						if (base == NULL){
							base = op1;
							buffer_access_size = ir_node_get_operation(parameterMapping_get_node_buffer(parameter)[i])->size;
							buffer_start_offset = ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2));
							if (buffer_start_offset >= (parameter->nb_fragment - 1) * (buffer_access_size / 8)){
								buffer_start_offset -= (parameter->nb_fragment - 1) * (buffer_access_size / 8);
								buffer[parameter->nb_fragment - 1] = 1;
							}
							else{
								buffer[buffer_start_offset / (buffer_access_size /8)] = 1;
								buffer_start_offset = 0;
							}
							
						}
						else{
							offset = ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2)); 
							if (buffer_start_offset <= offset && offset < buffer_start_offset + 2 * parameter->nb_fragment * (buffer_access_size / 8)){
								if ((offset - buffer_start_offset) % (buffer_access_size / 8) == 0){
									if (buffer[(offset - buffer_start_offset) / (buffer_access_size / 8)] == 0){
										buffer[(offset - buffer_start_offset) / (buffer_access_size / 8)] = 1;
									}
									else{
										is_input_buffer = 0;
									}
								}
								else{
									is_input_buffer = 0;
								}
							}
							else{
								is_input_buffer = 0;
							}
						}
					}
					else{
						is_input_buffer = 0;
					}
				}
				else{
					is_input_buffer = 0;
				}
			}
			else{
				is_input_buffer = 0;
			}
		}
		else{
			is_input_buffer = 0;
		}
	}

	if (is_input_buffer){
		for (i = 0; i < 2 * parameter->nb_fragment - 1; i++){
			if (buffer[i]){
				offset = buffer_start_offset + i * (buffer_access_size / 8);
				break;  
			}
		}

		for ( ; i < 2 * parameter->nb_fragment - 1; i++){
			if (!buffer[i]){
				break;
			}
		}

		if ((buffer_start_offset + i * (buffer_access_size / 8)) - offset == parameter->nb_fragment * (buffer_access_size / 8)){
			ir_print_location_node(base, NULL);
			printf("[%llu:%llu]\n", offset, buffer_start_offset + i * (buffer_access_size / 8));
			return;
		}
	}

	printf("{");
	for (i = 0; i < parameter->nb_fragment; i++){
		ir_print_location_node(parameterMapping_get_node_buffer(parameter)[i], NULL);
		if (i != parameter->nb_fragment - 1){
			printf(" ");
		}
		
	}
	printf("}\n");
}

static void signatureOccurence_print(struct codeSignature* code_signature){
	uint32_t 					i;
	uint32_t 					j;
	struct signatureNode* 		sig_node;
	uint32_t* 					nb_frag_input 		= NULL;
	uint32_t* 					nb_frag_output 		= NULL;
	struct parameterMapping* 	parameter_mapping 	= NULL;
	struct array* 				class_array 		= NULL;
	struct signatureLink* 		link;
	struct parameterMapping* 	class;

	if (code_signature->occurence.node_buffer == NULL || code_signature->occurence.input_buffer == NULL || code_signature->occurence.output_buffer == NULL){
		printf("WARNING: in %s, the signatureOccurence structure has not been initialized correctly\n", __func__);
		goto exit;
	}

	nb_frag_input = (uint32_t*)calloc(code_signature->nb_frag_tot_in, sizeof(uint32_t));
	nb_frag_output = (uint32_t*)calloc(code_signature->nb_frag_tot_out, sizeof(uint32_t));

	if (nb_frag_input == NULL || nb_frag_input == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	for (i = 0; i < code_signature->graph.nb_node; i++){
		sig_node = (struct signatureNode*)&(code_signature->sub_graph_handle->node_tab[i].node->data);

		if (sig_node->input_number > 0){
			nb_frag_input[sig_node->input_number - 1] ++;
		}
		if (sig_node->output_number > 0){
			nb_frag_output[sig_node->output_number - 1] ++;
		}
	}

	for (i = 0; i < code_signature->graph.nb_node; i++){
		sig_node = (struct signatureNode*)&(code_signature->sub_graph_handle->node_tab[i].node->data);

		if (sig_node->input_number > 0){
			if (sig_node->input_frag_order > nb_frag_input[sig_node->input_number - 1]){
				printf("ERROR: in %s, input frag number %u is out of range %u\n", __func__, sig_node->input_frag_order, nb_frag_input[sig_node->input_number - 1]);
				goto exit;
			}
		}
		if (sig_node->output_number > 0){
			if (sig_node->output_frag_order > nb_frag_output[sig_node->output_number - 1]){
				printf("ERROR: in %s, output frag number %u is out of range %u\n", __func__, sig_node->output_frag_order, nb_frag_output[sig_node->output_number - 1]);
				goto exit;
			}
		}
	}

	parameter_mapping = (struct parameterMapping*)malloc(parameterMapping_get_size(code_signature));
	if (parameter_mapping == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	for (i = 0, j = 0; i < code_signature->nb_parameter_in; i++){
		parameter_mapping[i].nb_fragment 			= nb_frag_input[i];
		parameter_mapping[i].node_buffer_offset 	= (code_signature->nb_parameter_in - i + code_signature->nb_parameter_out) * sizeof(struct parameterMapping) + j * sizeof(struct node*);
		parameter_mapping[i].similarity 			= PARAMETER_EQUAL;
		j += nb_frag_input[i];
	}

	for (i = 0; i < code_signature->nb_parameter_out; i++){
		parameter_mapping[code_signature->nb_parameter_in + i].nb_fragment 			= nb_frag_output[i];
		parameter_mapping[code_signature->nb_parameter_in + i].node_buffer_offset 	= (code_signature->nb_parameter_out - i) * sizeof(struct parameterMapping) + j * sizeof(struct node*);
		parameter_mapping[code_signature->nb_parameter_in + i].similarity 			= PARAMETER_EQUAL;
		j += nb_frag_output[i];
	}

	class_array = array_create(parameterMapping_get_size(code_signature));
	if (class_array == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		goto exit;
	}

	for (i = 0; i < code_signature->occurence.nb_occurence; i++){
		for (j = 0; j < code_signature->nb_frag_tot_in; j++){
			link = code_signature->occurence.input_buffer + (i * code_signature->nb_frag_tot_in) + j;
			parameterMapping_get_node_buffer(parameter_mapping + (IR_DEPENDENCE_MACRO_DESC_GET_ARG(link->edge_desc) - 1))[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) - 1] = link->node;
		}
		for (j = 0; j < code_signature->nb_frag_tot_out; j++){
			link = code_signature->occurence.output_buffer + (i * code_signature->nb_frag_tot_out) + j;
			parameterMapping_get_node_buffer(parameter_mapping + (code_signature->nb_parameter_in + IR_DEPENDENCE_MACRO_DESC_GET_ARG(link->edge_desc) - 1))[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) - 1] = link->node;
		}

		for (j = 0; j < array_get_length(class_array); j++){
			class = (struct parameterMapping*)array_get(class_array, j);
			if (signatureOccurence_try_append_to_class(class, parameter_mapping, code_signature->nb_parameter_in, code_signature->nb_parameter_out) == 0){
				break;
			}
		}
		if (j == array_get_length(class_array)){
			if (array_add(class_array, parameter_mapping) < 0){
				printf("ERROR: in %s, unable to add new element to array\n", __func__);
			}
		}
	}

	if (array_get_length(class_array) > MAX_CLASS_THRESHOLD){
		printf("WARNING: in %s, too many result clusters to be printed (%d)\n", __func__, array_get_length(class_array));
		goto exit;
	}

	for (i = 0; i < array_get_length(class_array); i++){
		class = (struct parameterMapping*)array_get(class_array, i);
		
		printf("CLUSTER %u/%u:\n", i + 1, array_get_length(class_array));
		for (j = 0; j < code_signature->nb_parameter_in; j++){
			if (class[j].similarity == PARAMETER_EQUAL){
				printf("\t Parameter I%u EQUAL = ", j);
			}
			else if (class[j].similarity == PARAMETER_PERMUT){
				printf("\t Parameter I%u PERMUT = ", j);
			}
			else{
				printf("\t Parameter I%u UNKNOWN = ", j);
			}
			signatureOccurence_print_location(class + j);
		}
		for (j = 0; j < code_signature->nb_parameter_out; j++){
			if (class[code_signature->nb_parameter_in + j].similarity == PARAMETER_EQUAL){
				printf("\t Parameter O%u EQUAL = ", j);
			}
			else if (class[code_signature->nb_parameter_in + j].similarity == PARAMETER_PERMUT){
				printf("\t Parameter O%u PERMUT = ", j);
			}
			else{
				printf("\t Parameter O%u UNKNOWN = ", j);
			}
			signatureOccurence_print_location(class + code_signature->nb_parameter_in + j);
		}
	}

	exit:
	if (nb_frag_input != NULL){
		free(nb_frag_input);
	}
	if (nb_frag_output != NULL){
		free(nb_frag_output);
	}
	if (parameter_mapping != NULL){
		free(parameter_mapping);
	}
	if (class_array != NULL){
		array_delete(class_array);
	}

}

#endif

static void signatureOccurence_clean(struct codeSignature* code_signature){
	uint32_t i;

	if (code_signature->occurence.node_buffer != NULL){
		for (i = 0; i < code_signature->occurence.nb_occurence; i++){
			if (code_signature->occurence.node_buffer[i] != NULL){
				printf("ERROR: in %s, this case is not supposed to happen, every instance should have been poped at this point\n", __func__);
			}
		}
	}

	code_signature->occurence.nb_occurence = 0;

	if (code_signature->occurence.input_buffer != NULL){
		free(code_signature->occurence.input_buffer);
		code_signature->occurence.input_buffer = NULL;
	}
	if (code_signature->occurence.output_buffer != NULL){
		free(code_signature->occurence.output_buffer);
		code_signature->occurence.output_buffer = NULL;
	}
	if (code_signature->occurence.node_buffer != NULL){
		free(code_signature->occurence.node_buffer);
		code_signature->occurence.node_buffer = NULL;
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
			if (!symbolTableEntry_is_resolved(signature_node->node_type.symbol)){
				printf("WARNING: in %s, the current node is an unresolved symbol (%s) setting its label to joker\n", __func__, signature_node->node_type.symbol->name);
				return SUBGRAPHISOMORPHISM_JOKER_LABEL;
			}
			else{
				return signature_node->node_type.symbol->id;
			}
		}
	}

	printf("ERROR: in %s, this case is not supposed to happen (incorrect signature node type)\n", __func__);
	return SUBGRAPHISOMORPHISM_JOKER_LABEL;
}

uint32_t irEdge_get_label(struct edge* edge){
	struct irDependence* dependence = ir_edge_get_dependence(edge);

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 		: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_ADDRESS 	: {
			return IR_DEPENDENCE_TYPE_ADDRESS;
		}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_SUBSTITUTE 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_MACRO 		: {
			return IR_DEPENDENCE_TYPE_MACRO | dependence->dependence_type.macro;
		}
	}

	return IR_DEPENDENCE_TYPE_DIRECT;
}

uint32_t signatureEdge_get_label(struct edge* edge){
	struct signatureEdge* signature_edge = (struct signatureEdge*)&(edge->data);

	return signature_edge->type | signature_edge->macro_desc;
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
	if (graphPrintDot_print(&(collection->syntax_graph), "collection.dot", NULL, NULL)){
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
				if (symbolTableEntry_is_resolved(signature_cursor->symbol_table->symbols + j)){
					nb_resolved ++;
				}
			}

			snprintf(symbol_str, 10, "%u/%u", nb_resolved, signature_cursor->symbol_table->nb_symbol);
		}

		if (graphPrintDot_print(&(signature_cursor->graph), file_name, NULL, NULL)){
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
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
			fprintf(file, "[label=\"disp\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {
			fprintf(file, "[label=\"/\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	:
		case IR_DEPENDENCE_TYPE_SUBSTITUTE 	: {
			fprintf(file, "[label=\"s\"]"); 		/* the s is used to tag special operands */
			break;
		}
		case IR_DEPENDENCE_TYPE_MACRO 		: {
			if (IR_DEPENDENCE_MACRO_DESC_IS_INPUT(edge->macro_desc)){
				fprintf(file, "[label=\"I%uF%u\"]", IR_DEPENDENCE_MACRO_DESC_GET_ARG(edge->macro_desc), IR_DEPENDENCE_MACRO_DESC_GET_FRAG(edge->macro_desc));
			}
			else{
				fprintf(file, "[label=\"O%uF%u\"]", IR_DEPENDENCE_MACRO_DESC_GET_ARG(edge->macro_desc), IR_DEPENDENCE_MACRO_DESC_GET_FRAG(edge->macro_desc));
			}
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void syntaxGraph_dotPrint_node(void* data, FILE* file, void* arg){
	struct codeSignature* signature = (struct codeSignature*)data;

	fprintf(file, "[label=\"%s\"]", signature->name);
}