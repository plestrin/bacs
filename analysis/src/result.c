#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "result.h"
#include "base.h"

/* ===================================================================== */
/* result routines														 */
/* ===================================================================== */

int32_t result_init(struct result* result, struct codeSignature* code_signature, struct array* assignment_array){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	struct node**	assignment_node;
	struct edge** 	assignment_edge;
	uint32_t 		nb_input;
	uint32_t 		nb_output;
	uint32_t 		nb_intern;

	result->code_signature 		= code_signature;
	result->nb_occurrence 		= array_get_length(assignment_array);
	result->nb_node_in 			= codeSignature_get_nb_frag_in(code_signature);
	result->nb_node_ou 			= codeSignature_get_nb_frag_ou(code_signature);
	result->nb_node_intern 		= code_signature->signature.graph.nb_node - (result->nb_node_in + result->nb_node_ou);
	result->nb_edge 			= code_signature->signature.graph.nb_edge;

	result->in_mapping_buffer 	= (struct signatureLink*)malloc(sizeof(struct signatureLink) * result->nb_occurrence * result->nb_node_in);
	result->ou_mapping_buffer 	= (struct signatureLink*)malloc(sizeof(struct signatureLink) * result->nb_occurrence * result->nb_node_ou);
	result->intern_node_buffer 	= (struct virtualNode*)malloc(sizeof(struct virtualNode) * result->nb_occurrence * result->nb_node_intern);
	result->edge_buffer 		= (struct virtualEdge*)malloc(sizeof(struct virtualEdge) * result->nb_occurrence * result->nb_edge);
	result->symbol_node_buffer 	= (struct node**)calloc(result->nb_occurrence, sizeof(struct node*));

	if (result->in_mapping_buffer == NULL || result->ou_mapping_buffer == NULL || result->intern_node_buffer == NULL || result->edge_buffer == NULL || result->symbol_node_buffer == NULL){
		log_err("unable to allocate memory");

		if (result->in_mapping_buffer != NULL){
			free(result->in_mapping_buffer);
		}
		if (result->ou_mapping_buffer != NULL){
			free(result->ou_mapping_buffer);
		}
		if (result->intern_node_buffer != NULL){
			free(result->intern_node_buffer);
		}
		if (result->edge_buffer != NULL){
			free(result->edge_buffer);
		}
		if (result->symbol_node_buffer != NULL){
			free(result->symbol_node_buffer);
		}

		return -1;
	}

	for (i = 0; i < result->nb_occurrence; i++){
		assignment_node = (struct node**)array_get(assignment_array, i);
		assignment_edge = (struct edge**)(assignment_node + code_signature->signature.graph.nb_node);
		nb_input 	= 0;
		nb_output 	= 0;
		nb_intern 	= 0;

		for (j = 0; j < code_signature->signature.graph.nb_node; j++){
			struct codeSignatureNode* 	sig_node;
			struct irOperation* 		operation;

			sig_node = (struct codeSignatureNode*)node_get_data(code_signature->signature.sub_graph_handle->sub_node_tab[j].node);
			operation = ir_node_get_operation(assignment_node[j]);

			if (operation->type == IR_OPERATION_TYPE_SYMBOL && operation->operation_type.symbol.result != NULL){
				if (sig_node->input_number > 0){
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].virtual_node.node 		= NULL;
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].virtual_node.result 	= operation->operation_type.symbol.result;
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].virtual_node.index 	= operation->operation_type.symbol.index;
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_INPUT(sig_node->input_frag_order, sig_node->input_number);
					nb_input ++;
				}

				if (sig_node->output_number > 0){
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].virtual_node.node 	= NULL;
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].virtual_node.result 	= operation->operation_type.symbol.result;
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].virtual_node.index 	= operation->operation_type.symbol.index;
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(sig_node->output_frag_order, sig_node->output_number);
					nb_output ++;
				}

				if (sig_node->input_number == 0 && sig_node->output_number == 0){
					result->intern_node_buffer[i * result->nb_node_intern + nb_intern].node 			= NULL;
					result->intern_node_buffer[i * result->nb_node_intern + nb_intern].result 			= operation->operation_type.symbol.result;
					result->intern_node_buffer[i * result->nb_node_intern + nb_intern].index 			= operation->operation_type.symbol.index;
					nb_intern ++;
				}
			}
			else{
				if (sig_node->input_number > 0){
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].virtual_node.node 		= assignment_node[j];
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].virtual_node.result 	= NULL;
					result->in_mapping_buffer[i * result->nb_node_in + nb_input].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_INPUT(sig_node->input_frag_order, sig_node->input_number);
					nb_input ++;
				}

				if (sig_node->output_number > 0){
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].virtual_node.node 	= assignment_node[j];
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].virtual_node.result 	= NULL;
					result->ou_mapping_buffer[i * result->nb_node_ou + nb_output].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(sig_node->output_frag_order, sig_node->output_number);
					nb_output ++;
				}

				if (sig_node->input_number == 0 && sig_node->output_number == 0){
					result->intern_node_buffer[i * result->nb_node_intern + nb_intern].node 			= assignment_node[j];
					result->intern_node_buffer[i * result->nb_node_intern + nb_intern].result 			= NULL;
					nb_intern ++;
				}
			}
		}

		for (j = 0; j < result->nb_edge; j++){
			struct irDependence* 	dependence;
			struct irOperation* 	operation;

			dependence = ir_edge_get_dependence(assignment_edge[j]);
			if (dependence->type == IR_DEPENDENCE_TYPE_MACRO){
				if (IR_DEPENDENCE_MACRO_DESC_IS_INPUT(dependence->dependence_type.macro)){
					operation = ir_node_get_operation(edge_get_dst(assignment_edge[j]));
					if (operation->type == IR_OPERATION_TYPE_SYMBOL){
						result->edge_buffer[i * result->nb_edge + j].edge 	= NULL;
						result->edge_buffer[i * result->nb_edge + j].result = operation->operation_type.symbol.result;
						result->edge_buffer[i * result->nb_edge + j].index 	= operation->operation_type.symbol.index;
					}
					else{
						log_err("dst of a macro edge is not a symbol");
						result->edge_buffer[i * result->nb_edge + j].edge 	= NULL;
						result->edge_buffer[i * result->nb_edge + j].result = NULL;
						result->edge_buffer[i * result->nb_edge + j].index 	= 0;
					}
				}
				else{
					operation = ir_node_get_operation(edge_get_src(assignment_edge[j]));
					if (operation->type == IR_OPERATION_TYPE_SYMBOL){
						result->edge_buffer[i * result->nb_edge + j].edge 	= NULL;
						result->edge_buffer[i * result->nb_edge + j].result = operation->operation_type.symbol.result;
						result->edge_buffer[i * result->nb_edge + j].index 	= operation->operation_type.symbol.index;
					}
					else{
						log_err("src of a macro edge is not a symbol");
						result->edge_buffer[i * result->nb_edge + j].edge 	= NULL;
						result->edge_buffer[i * result->nb_edge + j].result = NULL;
						result->edge_buffer[i * result->nb_edge + j].index 	= 0;
					}
				}
			}
			else{
				result->edge_buffer[i * result->nb_edge + j].edge 			= assignment_edge[j];
				result->edge_buffer[i * result->nb_edge + j].result 		= NULL;
				result->edge_buffer[i * result->nb_edge + j].index 			= 0;
			}
		}

		qsort_r(result->edge_buffer + i * result->nb_edge, result->nb_edge, sizeof(struct virtualEdge), (__compar_d_fn_t)memcmp, (void*)sizeof(struct virtualEdge));

		for (j = 1, k = 0; j < result->nb_edge; j++){
			if (memcmp(result->edge_buffer + i * result->nb_edge + k, result->edge_buffer + i * result->nb_edge + j, sizeof(struct virtualEdge))){
				k ++;
				if (k != j){
					memcpy(result->edge_buffer + i * result->nb_edge + k, result->edge_buffer + i * result->nb_edge + j, sizeof(struct virtualEdge));
				}
			}
		}

		k ++;
		memset(result->edge_buffer + i * result->nb_edge + k, 0, sizeof(struct virtualEdge) * (j - k));

		#ifdef EXTRA_CHECK
		if (nb_input != result->nb_node_in){
			log_err("wrong number of input nodes");
		}
		if (nb_output != result->nb_node_ou){
			log_err("wrong number of output nodes");
		}
		if (nb_intern != result->nb_node_intern){
			log_err("wrong number of intern nodes");
		}
		#endif
	}

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void result_push(struct result* result, struct ugraph* graph_layer, struct ir* ir){
	uint32_t 				i;
	uint32_t 				j;
	struct edge* 			edge;
	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	struct set* 			edge_set 		= NULL;
	struct unode* 			layer_node 		= NULL;
	struct unode*			unode_cursor;

	if (graph_layer != NULL){
		if ((edge_set = set_create(sizeof(struct edge*), 128)) == NULL){
			log_err("unable to create set");
			return;
		}

		for (unode_cursor = ugraph_get_head_node(graph_layer); unode_cursor != NULL; unode_cursor = unode_get_next(unode_cursor)){
			unode_get_layerNodeData(unode_cursor)->selected = 0xffffffff;
		}
	}
	#endif

	for (i = 0; i < result->nb_occurrence; i++){
		result->symbol_node_buffer[i] = ir_add_symbol_(ir, &(result->code_signature->signature.symbol), result, i);
		if (result->symbol_node_buffer[i] == NULL){
			log_err("unable to add symbolic node to IR");
			continue;
		}

		#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
		if (graph_layer != NULL){
			if ((layer_node = ugraph_add_node_(graph_layer)) == NULL){
				log_err("unable to add unode to ugraph");
			}
			else{
				struct layerNodeData* data;

				data = unode_get_layerNodeData(layer_node);

				data->selected 		= i;
				data->type 			= LAYERNODE_TYPE_MACRO;
				#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1
				data->nb_edge 		= 0;
				data->edge_buffer 	= 0;
				#endif

				set_empty(edge_set)
				result_get_edge_footprint(result, i, edge_set);
			}
		}
		#endif

		for (j = 0; j < result->nb_node_in; j++){
			virtualNode_push(result->in_mapping_buffer[i * result->nb_node_in + j].virtual_node)
			if (result->in_mapping_buffer[i * result->nb_node_in + j].virtual_node.node == NULL){
				log_err("unsatisfied input dependence");
				continue;
			}

			if ((edge = ir_add_macro_dependence(ir, result->in_mapping_buffer[i * result->nb_node_in + j].virtual_node.node, result->symbol_node_buffer[i], result->in_mapping_buffer[i * result->nb_node_in + j].edge_desc)) == NULL){
				log_err("unable to add dependence to IR");
			}
			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			else if (layer_node != NULL){
				edge->ptr = layer_node;
			}
			#endif
		}

		for (j = 0; j < result->nb_node_ou; j++){
			virtualNode_push(result->ou_mapping_buffer[i * result->nb_node_ou + j].virtual_node)
			if (result->ou_mapping_buffer[i * result->nb_node_ou + j].virtual_node.node == NULL){
				log_err("unsatisfied output dependence");
				continue;
			}

			if ((edge = ir_add_macro_dependence(ir, result->symbol_node_buffer[i], result->ou_mapping_buffer[i * result->nb_node_ou + j].virtual_node.node, result->ou_mapping_buffer[i * result->nb_node_ou + j].edge_desc)) == NULL){
				log_err("unable to add dependence to IR");
			}
			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			else if (layer_node != NULL){
				edge->ptr = layer_node;
			}
			#endif
		}

		#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
		if (layer_node != NULL){
			struct edge** 	edge_buffer;
			uint32_t 		nb_edge;

			result->symbol_node_buffer[i]->ptr = layer_node;

			if ((edge_buffer = (struct edge**)set_export_buffer_unique(edge_set, &nb_edge)) == NULL){
				log_err("unable to export set unique");
				continue;
			}

			if (!nb_edge){
				continue;
			}

			#define layer_add_conditionnal_edge(graph_layer, new_unode, old_unode) 							\
				if (unode_get_layerNodeData(old_unode)->selected != i){ 									\
					if (ugraph_add_edge_((graph_layer), (new_unode), (old_unode)) == NULL){ 				\
						log_err("unable to add uedge to ugraph"); 											\
					} 																						\
					else{ 																					\
						unode_get_layerNodeData(old_unode)->selected = i; 									\
					} 																						\
				}

			#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1
			{
				uint8_t* 				taken;
				uint32_t 				nb_orphan;
				struct unode* 			new_node;
				struct layerNodeData* 	new_data;

				if ((taken = (uint8_t*)calloc(nb_edge, sizeof(uint8_t))) == NULL){
					log_err("unable to allocate memory");
					goto exit;
				}

				for (j = 0, nb_orphan = 0; j < nb_edge; j++){
					if (edge_buffer[j]->ptr != NULL){
						if (!taken[j]){
							uint32_t 				k;
							uint32_t 				l;
							uint32_t 				m;
							struct uedge* 			uedge_cursor;
							struct edge** 			tmp_edge_buffer;
							uint32_t 				nb_tmp_edge;
							struct unode* 			old_node;
							struct layerNodeData* 	old_data;
							int32_t 				cmp;

							old_node = edge_buffer[j]->ptr;
							old_data = unode_get_layerNodeData(old_node);

							if ((tmp_edge_buffer = (struct edge**)calloc(old_data->nb_edge, sizeof(struct edge*))) == NULL){
								log_err("unable to allocate memory");
								continue;
							}

							for (k = j, l = 0, nb_tmp_edge = 0; k < nb_edge && l < old_data->nb_edge; ){
								cmp = memcmp(edge_buffer + k, old_data->edge_buffer + l, sizeof(struct edge*));

								if (cmp < 0){
									k ++;
								}
								else if (cmp > 0){
									l ++;
								}
								else{
									taken[k ++] = 1;
									tmp_edge_buffer[l ++] = (void*)1;
									nb_tmp_edge ++;
								}
							}

							#ifdef EXTRA_CHECK
							if (!nb_tmp_edge || !taken[j]){
								log_err("edge is not in edge buffer");
							}
							#endif

							if (nb_tmp_edge == old_data->nb_edge){
								free(tmp_edge_buffer);

								for (uedge_cursor = unode_get_head_edge(old_node); uedge_cursor != NULL; uedge_cursor = uedge_get_next(uedge_cursor)){
									layer_add_conditionnal_edge(graph_layer, layer_node, uedge_get_endp(uedge_cursor))
								}

								if (ugraph_add_edge_(graph_layer, layer_node, old_node) == NULL){
									log_err("unable to add uedge to ugraph");
								}
							}
							else{
								if ((new_node = ugraph_add_node_(graph_layer)) == NULL){
									log_err("unable to add unode to ugraph");
									free(tmp_edge_buffer);
									continue;
								}

								new_data = unode_get_layerNodeData(new_node);

								new_data->type 			= LAYERNODE_TYPE_PRIM;
								new_data->nb_edge 		= nb_tmp_edge;
								new_data->edge_buffer 	= tmp_edge_buffer;

								for (k = 0, l = 0, m = 0; k < old_data->nb_edge; k++){
									if (tmp_edge_buffer[k] == (void*)1){
										tmp_edge_buffer[l ++] = old_data->edge_buffer[k];
										old_data->edge_buffer[k]->ptr = new_node;
									}
									else{
										old_data->edge_buffer[m ++] = old_data->edge_buffer[k];
									}
								}

								old_data->nb_edge -= nb_tmp_edge;

								for (uedge_cursor = unode_get_head_edge(old_node); uedge_cursor != NULL; uedge_cursor = uedge_get_next(uedge_cursor)){
									if (ugraph_add_edge_(graph_layer, new_node, uedge_get_endp(uedge_cursor)) == NULL){
										log_err("unable to add uedge to ugraph");
									}

									layer_add_conditionnal_edge(graph_layer, layer_node, uedge_get_endp(uedge_cursor))
								}

								if (ugraph_add_edge_(graph_layer, layer_node, new_node) == NULL){
									log_err("unable to add uedge to ugraph");
								}
							}
						}
					}
					else{
						nb_orphan ++;
					}
				}

				free(taken);

				if (nb_orphan){
					if ((new_node = ugraph_add_node_(graph_layer)) == NULL){
						log_err("unable to add unode to ugraph");
						goto exit;
					}

					new_data = unode_get_layerNodeData(new_node);

					new_data->type 		= LAYERNODE_TYPE_PRIM;
					new_data->nb_edge 	= 0;

					if ((new_data->edge_buffer = (struct edge**)malloc(nb_orphan * sizeof(struct edge*))) == NULL){
						log_err("unable to allocate memory");
						goto exit;
					}

					for (j = 0; j < nb_edge; j++){
						if (edge_buffer[j]->ptr == NULL){
							new_data->edge_buffer[new_data->nb_edge ++] = edge_buffer[j];
							edge_buffer[j]->ptr = new_node;
						}
					}

					if (ugraph_add_edge_(graph_layer, layer_node, new_node) == NULL){
						log_err("unable to add uedge to ugraph");
					}
				}
			}

			exit:

			#else

			for (j = 0; j < nb_edge; j++){
				if (edge_buffer[j]->ptr != NULL){
					struct uedge* uedge_cursor;

					for (uedge_cursor = unode_get_head_edge((struct unode*)(edge_buffer[j]->ptr)); uedge_cursor != NULL; uedge_cursor = uedge_get_next(uedge_cursor)){
						layer_add_conditionnal_edge(graph_layer, layer_node, uedge_get_endp(uedge_cursor))
					}
					if (ugraph_add_edge_(graph_layer, layer_node, edge_buffer[j]->ptr) == NULL){
						log_err("unable to add uedge to ugraph");
					}
				}
				else if ((edge_buffer[j]->ptr = ugraph_add_node_(graph_layer)) != NULL){
					unode_get_layerNodeData(edge_buffer[j]->ptr)->type = LAYERNODE_TYPE_PRIM;
					if (ugraph_add_edge_(graph_layer, layer_node, edge_buffer[j]->ptr) == NULL){
						log_err("unable to add uedge to ugraph");
					}
				}
				else{
					log_err("unable to add unode to ugraph");
				}
			}

			#endif

			free(edge_buffer);
		}
		#endif
	}

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	if (edge_set != NULL){
		set_delete(edge_set);
	}
	#endif
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void result_pop(struct result* result, struct ugraph* graph_layer, struct ir* ir){
	uint32_t i;
	uint32_t j;

	for (i = 0; i < result->nb_occurrence; i++){
		if (result->symbol_node_buffer[i] != NULL){
			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			if (graph_layer != NULL && result->symbol_node_buffer[i]->ptr != NULL){
				ugraph_remove_node(graph_layer, (struct unode*)result->symbol_node_buffer[i]->ptr);
			}
			#endif

			graph_remove_node(&(ir->graph), result->symbol_node_buffer[i]);
		}
		result->symbol_node_buffer[i] = NULL;

		for (j = 0; j < result->nb_node_in; j++){
			virtualNode_pop(result->in_mapping_buffer[i * result->nb_node_in + j].virtual_node)
		}

		for (j = 0; j < result->nb_node_ou; j++){
			virtualNode_pop(result->ou_mapping_buffer[i * result->nb_node_ou + j].virtual_node)
		}
	}
}

void result_get_intern_node_footprint(const struct result* result, uint32_t index, struct set* set){
	uint32_t i;

	#ifdef EXTRA_CHECK
	if (index >= result->nb_occurrence){
		log_err_m("incorrect index %u (max is %u)", index, result->nb_occurrence);
		return;
	}
	#endif

	for (i = 0; i < result->nb_node_intern; i++){
		if (result->intern_node_buffer[index * result->nb_node_intern + i].node == NULL){
			result_get_intern_node_footprint(result->intern_node_buffer[index * result->nb_node_intern + i].result, result->intern_node_buffer[index * result->nb_node_intern + i].index, set);
		}
		else if (set_add(set, &(result->intern_node_buffer[index * result->nb_node_intern + i].node)) < 0){
			log_err("unable to add element to set");
		}
	}
}

void result_get_edge_footprint(const struct result* result, uint32_t index, struct set* set){
	uint32_t i;

	#ifdef EXTRA_CHECK
	if (index >= result->nb_occurrence){
		log_err_m("incorrect index %u (max is %u)", index, result->nb_occurrence);
		return;
	}
	#endif

	for (i = 0; i < result->nb_edge; i++){
		if (result->edge_buffer[index * result->nb_edge + i].edge == NULL){
			if (result->edge_buffer[index * result->nb_edge + i].result == NULL){
				break;
			}
			else{
				result_get_edge_footprint(result->edge_buffer[index * result->nb_edge + i].result, result->edge_buffer[index * result->nb_edge + i].index, set);
			}
		}
		else if (set_add(set, &(result->edge_buffer[index * result->nb_edge + i].edge)) < 0){
			log_err("unable to add element to set");
		}
	}
}

void result_remove_edge_footprint(struct result* result, struct ir* ir){ /* To be deleted? */
	uint32_t 		i;
	uint32_t 		j;
	struct edge* 	edge_curr;
	struct edge* 	edge_next;

	for (i = 0; i < result->nb_occurrence; i++){
		for (j = 0; j < result->nb_node_ou; j++){
			if (result->ou_mapping_buffer[i * result->nb_node_ou + j].virtual_node.node == NULL){
				log_err("unsatisfied input dependence");
				continue;
			}

			for (edge_curr = node_get_head_edge_dst(result->ou_mapping_buffer[i * result->nb_node_ou + j].virtual_node.node); edge_curr != NULL; edge_curr = edge_next){
				edge_next = edge_get_next_dst(edge_curr);

				if (ir_edge_get_dependence(edge_curr)->type != IR_DEPENDENCE_TYPE_MACRO){
					ir_remove_dependence(ir, edge_curr);
				}
			}
		}
	}
}

#define MAX_CLASS_THRESHOLD 2

void result_print(struct result* result){
	uint32_t 				i;
	uint32_t 				j;
	struct symbolMapping* 	symbol_mapping;
	struct array* 			class_array;
	struct symbolMapping* 	class;

	if ((class_array = array_create(sizeof(struct symbolMapping*))) == NULL){
		log_err("unable to create array");
		return;
	}

	for (i = 0; i < result->nb_occurrence; i++){
		if ((symbol_mapping = symbolMapping_create_from_result(result, i)) == NULL){
			log_err("unable to create symbolMapping from result");
			continue;
		}

		for (j = 0; j < array_get_length(class_array); j++){
			class = *(struct symbolMapping**)array_get(class_array, j);
			if (symbolMapping_may_append(class, symbol_mapping) == 0){
				free(symbol_mapping);
				symbol_mapping = NULL;
				break;
			}
		}
		if (symbol_mapping != NULL){
			if (array_add(class_array, &symbol_mapping) < 0){
				log_err("unable to add new element to array");
				free(symbol_mapping);
				symbol_mapping = NULL;
			}
		}
	}

	if (array_get_length(class_array) > MAX_CLASS_THRESHOLD){
		log_warn_m("too many result clusters to be printed (%d) for " ANSI_COLOR_BOLD "%s" ANSI_COLOR_RESET, array_get_length(class_array), result->code_signature->signature.symbol.name);
		goto exit;
	}

	for (i = 0; i < array_get_length(class_array); i++){
		class = *(struct symbolMapping**)array_get(class_array, i);

		printf(ANSI_COLOR_BOLD "%s" ANSI_COLOR_RESET " CLUSTER %u/%u:\n", result->code_signature->signature.symbol.name, i + 1, array_get_length(class_array));
		for (j = 0; j < class[0].nb_parameter; j++){
			if (class[0].mapping_buffer[j].similarity == PARAMETER_EQUAL){
				printf("\t Parameter I%u EQUAL = ", j);
			}
			else if (class[0].mapping_buffer[j].similarity == PARAMETER_PERMUT){
				printf("\t Parameter I%u PERMUT = ", j);
			}
			else{
				printf("\t Parameter I%u UNKNOWN = ", j);
			}
			parameterMapping_print_location(class[0].mapping_buffer + j);
		}
		for (j = 0; j < class[1].nb_parameter; j++){
			if (class[1].mapping_buffer[j].similarity == PARAMETER_EQUAL){
				printf("\t Parameter O%u EQUAL = ", j);
			}
			else if (class[1].mapping_buffer[j].similarity == PARAMETER_PERMUT){
				printf("\t Parameter O%u PERMUT = ", j);
			}
			else{
				printf("\t Parameter O%u UNKNOWN = ", j);
			}
			parameterMapping_print_location(class[1].mapping_buffer + j);
		}
	}

	exit:
	for (i = 0; i < array_get_length(class_array); i++){
		free(*(struct symbolMapping**)array_get(class_array, i));
	}
	array_delete(class_array);
}

/* ===================================================================== */
/* parameterMapping & symbolMapping routines							 */
/* ===================================================================== */

static enum parameterSimilarity parameterMapping_get_similarity(const struct parameterMapping* parameter_mapping1, const struct parameterMapping* parameter_mapping2){
	uint32_t 					i;
	uint32_t 					j;
	enum parameterSimilarity 	result;
	uint32_t 					nb_found;

	if (parameter_mapping1->nb_fragment == parameter_mapping2->nb_fragment){
		for (i = 0, nb_found = 0, result = PARAMETER_EQUAL; i < parameter_mapping1->nb_fragment; i++){
			for (j = 0; j < parameter_mapping2->nb_fragment; j++){
				if (parameter_mapping1->ptr_buffer[i] == parameter_mapping2->ptr_buffer[j]){
					nb_found ++;
					if (i != j){
						result = PARAMETER_PERMUT;
					}
					break;
				}
			}
		}

		if (nb_found && parameter_mapping1->nb_fragment != nb_found){
			return PARAMETER_OVERLAP;
		}
	}
	else if (parameter_mapping1->nb_fragment < parameter_mapping2->nb_fragment){
		for (i = 0, nb_found = 0; i < parameter_mapping1->nb_fragment; i++){
			for (j = 0; j < parameter_mapping2->nb_fragment; j++){
				if (parameter_mapping1->ptr_buffer[i] == parameter_mapping2->ptr_buffer[j]){
					nb_found ++;
					break;
				}
			}
		}

		if (nb_found && parameter_mapping1->nb_fragment != nb_found){
			return PARAMETER_OVERLAP;
		}

		result = PARAMETER_SUPERSET;
	}
	else{
		for (i = 0, nb_found = 0; i < parameter_mapping2->nb_fragment; i++){
			for (j = 0; j < parameter_mapping1->nb_fragment; j++){
				if (parameter_mapping2->ptr_buffer[i] == parameter_mapping1->ptr_buffer[j]){
					nb_found ++;
					break;
				}
			}
		}

		if (nb_found && parameter_mapping2->nb_fragment != nb_found){
			return PARAMETER_OVERLAP;
		}

		result = PARAMETER_SUBSET;
	}

	if (nb_found == 0){
		return PARAMETER_DISJOINT;
	}

	return result;
}

void parameterMapping_print_location(const struct parameterMapping* mapping){
	uint32_t 			i;
	struct irOperation* operation;
	uint8_t 			is_input_buffer;
	struct node* 		address;
	struct node* 		base;
	uint64_t 			buffer_start_offset = 0;
	uint8_t* 			buffer;
	uint8_t 			buffer_access_size 	= 0;
	uint64_t 			offset 				= 0;

	if (mapping->nb_fragment == 0){
		return;
	}

	buffer = alloca(sizeof(uint8_t) * (2 * mapping->nb_fragment - 1));
	memset(buffer, 0, sizeof(uint8_t) * (2 * mapping->nb_fragment - 1));

	for (i = 0, is_input_buffer = 1, base = NULL; i < mapping->nb_fragment && is_input_buffer == 1; i++){
		operation = ir_node_get_operation(mapping->ptr_buffer[i]);
		if (operation->type == IR_OPERATION_TYPE_IN_MEM){
			address = edge_get_src(node_get_head_edge_dst(mapping->ptr_buffer[i]));
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
					if (base == NULL || (base == op1 && buffer_access_size == ir_node_get_operation(mapping->ptr_buffer[i])->size)){
						if (base == NULL){
							base = op1;
							buffer_access_size = ir_node_get_operation(mapping->ptr_buffer[i])->size;
							buffer_start_offset = ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2));
							if (buffer_start_offset >= (mapping->nb_fragment - 1) * (buffer_access_size / 8)){
								buffer_start_offset -= (mapping->nb_fragment - 1) * (buffer_access_size / 8);
								buffer[mapping->nb_fragment - 1] = 1;
							}
							else{
								buffer[buffer_start_offset / (buffer_access_size /8)] = 1;
								buffer_start_offset = 0;
							}

						}
						else{
							offset = ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2));
							if (buffer_start_offset <= offset && offset < buffer_start_offset + 2 * mapping->nb_fragment * (buffer_access_size / 8)){
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
		for (i = 0; i < 2 * mapping->nb_fragment - 1; i++){
			if (buffer[i]){
				offset = buffer_start_offset + i * (buffer_access_size / 8);
				break;
			}
		}

		for ( ; i < 2 * mapping->nb_fragment - 1; i++){
			if (!buffer[i]){
				break;
			}
		}

		if ((buffer_start_offset + i * (buffer_access_size / 8)) - offset == mapping->nb_fragment * (buffer_access_size / 8)){
			ir_print_location_node(base);
			printf("[%llu:%llu]\n", offset, buffer_start_offset + i * (buffer_access_size / 8));
			return;
		}
	}

	fputc('{', stdout);
	for (i = 0; i < mapping->nb_fragment; i++){
		ir_print_location_node(mapping->ptr_buffer[i]);
		if (i != mapping->nb_fragment - 1){
			fputc(' ', stdout);
		}

	}
	printf("}\n");
}

static struct symbolMapping* symbolMapping_create(uint32_t nb_para_in, uint32_t nb_para_ou, const uint32_t* nb_frag_in, const uint32_t* nb_frag_ou){
	uint32_t 				i;
	size_t 					size;
	struct symbolMapping* 	mapping;

	size = 0;
	for (i = 0, size += sizeof(struct symbolMapping) + nb_para_in * sizeof(struct parameterMapping); i < nb_para_in; i++){
		size += nb_frag_in[i] * sizeof(struct node*);
	}
	for (i = 0, size += sizeof(struct symbolMapping) + nb_para_ou * sizeof(struct parameterMapping); i < nb_para_ou; i++){
		size += nb_frag_ou[i] * sizeof(struct node*);
	}

	if ((mapping = (struct symbolMapping*)calloc(size, 1)) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	size = 2 * sizeof(struct symbolMapping) + (nb_para_in + nb_para_ou) * sizeof(struct parameterMapping);

	mapping[0].nb_parameter 	= nb_para_in;
	mapping[0].mapping_buffer 	= (struct parameterMapping*)(mapping + 2);

	for (i = 0; i < nb_para_in; i++){
		mapping[0].mapping_buffer[i].nb_fragment 	= nb_frag_in[i];
		mapping[0].mapping_buffer[i].ptr_buffer 	= (struct node**)((char*)mapping + size);
		size += nb_frag_in[i] * sizeof(struct node*);
	}

	mapping[1].nb_parameter 	= nb_para_ou;
	mapping[1].mapping_buffer 	= mapping[0].mapping_buffer + nb_para_in;

	for (i = 0; i < nb_para_ou; i++){
		mapping[1].mapping_buffer[i].nb_fragment 	= nb_frag_ou[i];
		mapping[1].mapping_buffer[i].ptr_buffer 	= (struct node**)((char*)mapping + size);
		size += nb_frag_ou[i] * sizeof(struct node*);
	}

	return mapping;
}

static void symbolMapping_adjust(struct symbolMapping* mapping){
	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t l;

	for (i = 0, l = 0; i < mapping->nb_parameter; i ++){
		for (j = 0, k = 0; j < mapping->mapping_buffer[i].nb_fragment; j++){
			if (mapping->mapping_buffer[i].ptr_buffer[j] == NULL){
				log_warn_m("missing *%uF%u for mapping", i + 1, j + 1);
			}
			else{
				mapping->mapping_buffer[i].ptr_buffer[k] = mapping->mapping_buffer[i].ptr_buffer[j];
				k ++;
			}
		}
		if (k != 0){
			mapping->mapping_buffer[l].nb_fragment = k;
			mapping->mapping_buffer[l].ptr_buffer = mapping->mapping_buffer[i].ptr_buffer;
			l ++;
		}
	}
	mapping->nb_parameter = l;
}

struct symbolMapping* symbolMapping_create_from_result(struct result* result, uint32_t index){
	uint32_t 				i;
	struct signatureLink* 	link;
	struct symbolMapping* 	mapping;

	if ((mapping = symbolMapping_create(result->code_signature->nb_para_in, result->code_signature->nb_para_ou, result->code_signature->nb_frag_in, result->code_signature->nb_frag_ou)) == NULL){
		log_err("unable to create symbolMapping");
		return NULL;
	}

	for (i = 0; i < result->nb_node_in; i++){
		link = result->in_mapping_buffer + (index * result->nb_node_in) + i;
		if (link->virtual_node.node == NULL){
			log_err("input node is virtual, I don't kown how to handle that case");
			continue;
		}
		if (!IR_DEPENDENCE_MACRO_DESC_IS_INPUT(link->edge_desc)){
			log_err("incorrect macro descriptor");
			continue;
		}
		if (IR_DEPENDENCE_MACRO_DESC_GET_PARA(link->edge_desc) > mapping[0].nb_parameter){
			log_err("incorrect macro descriptor");
			continue;
		}
		if (IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) > mapping[0].mapping_buffer[IR_DEPENDENCE_MACRO_DESC_GET_PARA(link->edge_desc) - 1].nb_fragment){
			log_err("incorrect macro descriptor");
			continue;
		}
		mapping[0].mapping_buffer[IR_DEPENDENCE_MACRO_DESC_GET_PARA(link->edge_desc) - 1].ptr_buffer[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) - 1] = link->virtual_node.node;
	}

	symbolMapping_adjust(mapping);

	for (i = 0; i < result->nb_node_ou; i++){
		link = result->ou_mapping_buffer + (index * result->nb_node_ou) + i;
		if (link->virtual_node.node == NULL){
			log_err("output node is virtual, I don't kown how to handle that case");
		}
		if (!IR_DEPENDENCE_MACRO_DESC_IS_OUTPUT(link->edge_desc)){
			log_err("incorrect macro descriptor");
			continue;
		}
		if (IR_DEPENDENCE_MACRO_DESC_GET_PARA(link->edge_desc) > mapping[1].nb_parameter){
			log_err("incorrect macro descriptor");
			continue;
		}
		if (IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) > mapping[1].mapping_buffer[IR_DEPENDENCE_MACRO_DESC_GET_PARA(link->edge_desc) - 1].nb_fragment){
			log_err("incorrect macro descriptor");
			continue;
		}
		mapping[1].mapping_buffer[IR_DEPENDENCE_MACRO_DESC_GET_PARA(link->edge_desc) - 1].ptr_buffer[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) - 1] = link->virtual_node.node;
	}

	symbolMapping_adjust(mapping + 1);

	return mapping;
}

struct symbolMapping* symbolMapping_create_from_ir(struct node* node){
	struct edge* 			edge_cursor;
	uint32_t 				nb_para_in;
	uint32_t 				nb_para_ou;
	uint32_t 				nb_frag_in[CODESIGNATURE_NB_PARA_MAX] = {0};
	uint32_t 				nb_frag_ou[CODESIGNATURE_NB_PARA_MAX] = {0};
	struct irDependence* 	dependence_cursor;
	struct symbolMapping* 	mapping;

	for (edge_cursor = node_get_head_edge_dst(node), nb_para_in = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		dependence_cursor = ir_edge_get_dependence(edge_cursor);

		if (dependence_cursor->type == IR_DEPENDENCE_TYPE_MACRO){
			if (IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) > CODESIGNATURE_NB_PARA_MAX){
				log_warn_m("signature \"%s\" has too many input parameter: %u -> limiting to %u", ir_node_get_operation(node)->operation_type.symbol.sym_sig->name, IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro), CODESIGNATURE_NB_PARA_MAX);
				nb_para_in = CODESIGNATURE_NB_PARA_MAX;
				nb_frag_in[CODESIGNATURE_NB_PARA_MAX - 1] = max(nb_frag_in[CODESIGNATURE_NB_PARA_MAX - 1], IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro));
			}
			else{
				nb_para_in = max(nb_para_in, IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro));
				nb_frag_in[IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) - 1] = max(nb_frag_in[IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) - 1], IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro));
			}
		}
	}

	for (edge_cursor = node_get_head_edge_src(node), nb_para_ou = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		dependence_cursor = ir_edge_get_dependence(edge_cursor);

		if (dependence_cursor->type == IR_DEPENDENCE_TYPE_MACRO){
			if (IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) > CODESIGNATURE_NB_PARA_MAX){
				log_warn_m("signature \"%s\" has too many output parameter: %u -> limiting to %u", ir_node_get_operation(node)->operation_type.symbol.sym_sig->name, IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro), CODESIGNATURE_NB_PARA_MAX);
				nb_para_ou = CODESIGNATURE_NB_PARA_MAX;
				nb_frag_ou[CODESIGNATURE_NB_PARA_MAX - 1] = max(nb_frag_ou[CODESIGNATURE_NB_PARA_MAX - 1], IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro));
			}
			else{
				nb_para_ou = max(nb_para_ou, IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro));
				nb_frag_ou[IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) - 1] = max(nb_frag_ou[IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) - 1], IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro));
			}
		}
	}

	if ((mapping = symbolMapping_create(nb_para_in, nb_para_ou, nb_frag_in, nb_frag_ou)) == NULL){
		log_err("unable to create symbolMapping");
		return NULL;
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		dependence_cursor = ir_edge_get_dependence(edge_cursor);

		if (dependence_cursor->type == IR_DEPENDENCE_TYPE_MACRO){
			if (IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) > CODESIGNATURE_NB_PARA_MAX){
				mapping[0].mapping_buffer[IR_DEPENDENCE_TYPE_MACRO - 1].ptr_buffer[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro) - 1] = edge_get_src(edge_cursor);
			}
			else{
				mapping[0].mapping_buffer[IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) - 1].ptr_buffer[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro) - 1] = edge_get_src(edge_cursor);
			}
		}
	}

	symbolMapping_adjust(mapping);

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		dependence_cursor = ir_edge_get_dependence(edge_cursor);

		if (dependence_cursor->type == IR_DEPENDENCE_TYPE_MACRO){
			if (IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) > CODESIGNATURE_NB_PARA_MAX){
				mapping[1].mapping_buffer[IR_DEPENDENCE_TYPE_MACRO - 1].ptr_buffer[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro) - 1] = edge_get_dst(edge_cursor);
			}
			else{
				mapping[1].mapping_buffer[IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence_cursor->dependence_type.macro) - 1].ptr_buffer[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence_cursor->dependence_type.macro) - 1] = edge_get_dst(edge_cursor);
			}
		}
	}

	symbolMapping_adjust(mapping + 1);

	return mapping;
}

int32_t symbolMapping_may_append(struct symbolMapping* mapping_dst, struct symbolMapping* mapping_src){
	uint32_t 					i;
	enum parameterSimilarity* 	ou_similarity;
	enum parameterSimilarity* 	in_similarity;

	if (mapping_src[0].nb_parameter != mapping_dst[0].nb_parameter || mapping_src[1].nb_parameter != mapping_dst[1].nb_parameter){
		return -1;
	}

	in_similarity = (enum parameterSimilarity*)alloca(sizeof(enum parameterSimilarity) * mapping_src[0].nb_parameter);
	ou_similarity = (enum parameterSimilarity*)alloca(sizeof(enum parameterSimilarity) * mapping_src[1].nb_parameter);

	for (i = 0; i < mapping_src[0].nb_parameter; i++){
		in_similarity[i] = parameterMapping_get_similarity(mapping_src[0].mapping_buffer + i, mapping_dst[0].mapping_buffer + i);
		if (in_similarity[i] == PARAMETER_OVERLAP || in_similarity[i] == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < mapping_src[1].nb_parameter; i++){
		ou_similarity[i] = parameterMapping_get_similarity(mapping_src[1].mapping_buffer + i, mapping_dst[1].mapping_buffer + i);
		if (ou_similarity[i] == PARAMETER_OVERLAP || ou_similarity[i] == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < mapping_src[0].nb_parameter; i++){
		mapping_dst[0].mapping_buffer[i].similarity |= in_similarity[i];
	}

	for (i = 0; i < mapping_src[1].nb_parameter; i++){
		mapping_dst[1].mapping_buffer[i].similarity |= ou_similarity[i];
	}

	return 0;
}
