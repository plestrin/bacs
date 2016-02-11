#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "signatureCollection.h"
#include "graphPrintDot.h"
#include "multiColumn.h"
#include "base.h"

static void signatureCollection_dotPrint_node(void* data, FILE* file, void* arg);

struct signatureCollection* signatureCollection_create(size_t signature_size, struct signatureCallback* callback){
	struct signatureCollection* collection;

	collection = (struct signatureCollection*)malloc(sizeof(struct signatureCollection));
	if (collection != NULL){
		signatureCollection_init(collection, signature_size, callback);
	}
	else{
		log_err("unable to allocate memory");
	}

	return collection;
}

int32_t signatureCollection_add(struct signatureCollection* collection, void* custom_signature){
	struct node* 		syntax_node;
	struct node* 		node_cursor;
	struct signature* 	signature;
	struct signature* 	signature_cursor;
	uint32_t 			i;
	uint32_t 			nb_unresolved_symbol;

	syntax_node = graph_add_node(&(collection->syntax_graph), custom_signature);
	if (syntax_node == NULL){
		log_err("unable to add code signature to the collection's syntax graph");
		return -1;
	}

	signature = signatureCollection_node_get_signature(syntax_node);

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor != syntax_node){
			signature_cursor = signatureCollection_node_get_signature(node_cursor);

			if (!strncmp(signature->symbol, signature_cursor->symbol, SIGNATURE_NAME_MAX_SIZE)){
				signature->id = signature_cursor->id;
				break;
			}
		}
	}
	if (node_cursor == NULL){
		signature->id = signatureCollection_get_new_id(collection);
	}

	if (signature->symbol_table != NULL){
		for (i = 0, nb_unresolved_symbol = 0; i < signature->symbol_table->nb_symbol; i++){
			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = signatureCollection_node_get_signature(node_cursor);

				if (!strncmp(signature->symbol_table->symbols[i].name, signature_cursor->symbol, SIGNATURE_NAME_MAX_SIZE)){
					signatureSymbol_set_id(signature->symbol_table->symbols + i, signature_cursor->id);

					if (graph_add_edge(&(collection->syntax_graph), node_cursor, syntax_node, &i) == NULL){
						log_err("unable to add edge to the syntax tree");
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

	if (signature->sub_graph_handle == NULL){
		if (nb_unresolved_symbol == 0){
			signature->sub_graph_handle = graphIso_create_sub_graph_handle(&(signature->graph), collection->callback.signatureNode_get_label, collection->callback.signatureEdge_get_label);
		}
	}
	else{
		log_warn("subgraph handle is already built, symbols are ignored");
		signature->sub_graph_handle->graph = &(signature->graph);
	}

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = signatureCollection_node_get_signature(node_cursor);

		if (signature_cursor->symbol_table != NULL){
			for (i = 0, nb_unresolved_symbol = 0; i < signature_cursor->symbol_table->nb_symbol; i++){
				if (!strncmp(signature_cursor->symbol_table->symbols[i].name, signature->symbol, SIGNATURE_NAME_MAX_SIZE)){
					signatureSymbol_set_id(signature_cursor->symbol_table->symbols + i, signature->id);

					if (graph_add_edge(&(collection->syntax_graph), syntax_node, node_cursor, &i) == NULL){
						log_err("unable to add edge to the syntax tree");
					}
				}
				else if (!signatureSymbol_is_resolved(signature_cursor->symbol_table->symbols + i)){
					nb_unresolved_symbol ++;
				}
			}

			if (nb_unresolved_symbol == 0 && signature_cursor->sub_graph_handle == NULL){
				signature_cursor->sub_graph_handle = graphIso_create_sub_graph_handle(&(signature_cursor->graph), collection->callback.signatureNode_get_label, collection->callback.signatureEdge_get_label);
			}
		}
	}

	return 0;
}

void signatureCollection_search(struct signatureCollection* collection, struct graphSearcher* graph_searcher_buffer, uint32_t nb_graph_searcher, uint32_t(*graphNode_get_label)(struct node*), uint32_t(*graphEdge_get_label)(struct edge*)){
	struct node* 				node_cursor;
	uint32_t 					i;
	uint32_t 					j;
	struct edge* 				edge_cursor;
	struct signature* 			signature_cursor;
	struct signature*			child;
	struct signature**			signature_buffer;
	uint32_t 					nb_signature;
	struct graphIsoHandle* 		graph_handle;
	struct timespec 			timer1_start_time;
	struct timespec 			timer1_stop_time;
	struct timespec 			timer2_start_time;
	struct timespec 			timer2_stop_time;
	double 						timer2_elapsed_time;
	struct multiColumnPrinter* 	printer;
	struct array* 				assignement_array;
	uint32_t 					nb_searched;

	signature_buffer = (struct signature**)malloc(sizeof(struct signature*) * signatureCollection_get_nb_signature(collection));
	if (signature_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_DOUBLE);
		multiColumnPrinter_set_column_size(printer, 0, SIGNATURE_NAME_MAX_SIZE);
		multiColumnPrinter_set_title(printer, 0, "NAME");
		multiColumnPrinter_set_title(printer, 1, "FOUND");
		multiColumnPrinter_set_title(printer, 2, "TIME");

		multiColumnPrinter_print_header(printer);
	}
	else{
		log_err("unable to create multiColumnPrinter");
		free(signature_buffer);
		return;
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer1_start_time)){
		log_err("clock_gettime fails");
	}

	for (i = 0; i < nb_graph_searcher; i++){
		for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			signature_cursor = signatureCollection_node_get_signature(node_cursor);
			signature_cursor->state = 0;
		}

		for (nb_searched = 0; nb_searched != collection->syntax_graph.nb_node; ){
			nb_signature = 0;
			nb_searched = 0;

			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = signatureCollection_node_get_signature(node_cursor);
				if (signature_state_is_search(signature_cursor)){
					nb_searched ++;
					goto next1;
				}

				if (signature_cursor->symbol_table != NULL){
					for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						child = signatureCollection_node_get_signature(edge_get_src(edge_cursor));
						if (!signature_state_is_search(child)){
							goto next1;
						}
					}

					for (j = 0; j < signature_cursor->symbol_table->nb_symbol; j++){
						if (signatureSymbol_is_resolved(signature_cursor->symbol_table->symbols + j)){
							for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
								child = signatureCollection_node_get_signature(edge_get_src(edge_cursor));

								if (child->id == signatureSymbol_get_id(signature_cursor->symbol_table->symbols + j) && signature_state_is_found(child)){
									break;
								}
							}
							if (edge_cursor == NULL){
								signature_state_set_search(signature_cursor);
								goto next1;
							}
						}
						else{
							log_warn_m("unresolved symbol(s) for signature %s", signature_cursor->name);
							signature_state_set_search(signature_cursor);
							goto next1;
						}
					}
				}

				if (signature_cursor->sub_graph_handle == NULL){
					log_warn_m("sub_graph_handle is still NULL for %s. It may be due to unresolved symbol(s)", signature_cursor->name);
					signature_state_set_search(signature_cursor);
					goto next1;
				}

				signature_buffer[nb_signature ++] = signature_cursor;

				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					child = signatureCollection_node_get_signature(edge_get_src(edge_cursor));
					if (signature_state_is_found(child) && !signature_state_is_pushed(child)){
						if (graph_searcher_buffer[i].result_push != NULL){
							graph_searcher_buffer[i].result_push(child->result_index, graph_searcher_buffer[i].arg);
						}
						signature_state_set_pushed(child);
					}
				}

				next1:;
			}

			if (nb_signature){
				graph_handle = graphIso_create_graph_handle(graph_searcher_buffer[i].graph, graphNode_get_label, graphEdge_get_label);
				if (graph_handle == NULL){
					log_err("unable to create graphHandle");
					break;
				}

				for (j = 0; j < nb_signature; j++){
					if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer2_start_time)){
						log_err("clock_gettime fails");
					}

					assignement_array = graphIso_search(graph_handle, signature_buffer[j]->sub_graph_handle);

					if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer2_stop_time)){
						log_err("clock_gettime fails");
					}
					timer2_elapsed_time = ((timer2_stop_time.tv_sec - timer2_start_time.tv_sec) + (timer2_stop_time.tv_nsec - timer2_start_time.tv_nsec) / 1000000000.);

					signature_state_set_search(signature_buffer[j]);
					if (assignement_array == NULL){
						log_err("the subgraph isomorphism routine fails");
					}
					else if (array_get_length(assignement_array) > 0){
						if (graph_searcher_buffer[i].result_register != NULL){
							signature_buffer[j]->result_index = graph_searcher_buffer[i].result_register(signature_buffer[j], assignement_array, graph_searcher_buffer[i].arg);
							if (signature_buffer[j]->result_index  < 0){
								log_err("unable to add element to array");
							}
						}
						else{
							signature_buffer[j]->result_index = -1;
						}

						signature_state_set_found(signature_buffer[j]);
						multiColumnPrinter_print(printer, signature_buffer[j]->name, array_get_length(assignement_array), timer2_elapsed_time, NULL);
						array_delete(assignement_array);
					}
					else{
						array_delete(assignement_array);
					}
				}

				graphIso_delete_graph_handle(graph_handle);
			}

			for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
				signature_cursor = signatureCollection_node_get_signature(node_cursor);
				if (!signature_state_is_search(signature_cursor) || !signature_state_is_found(signature_cursor) || !signature_state_is_pushed(signature_cursor)){
					goto next2;
				}

				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					child = signatureCollection_node_get_signature(edge_get_dst(edge_cursor));
					if (!signature_state_is_search(child)){
						goto next2;
					}
				}

				if (graph_searcher_buffer[i].result_pop != NULL){
					graph_searcher_buffer[i].result_pop(signature_cursor->result_index, graph_searcher_buffer[i].arg);
				}
				signature_state_set_poped(signature_cursor);

				next2:;
			}
		}
		multiColumnPrinter_print_horizontal_separator(printer);
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer1_stop_time)){
		log_err("clock_gettime fails");
	}

	multiColumnPrinter_delete(printer);
	log_info_m("total elapsed time: %f", (timer1_stop_time.tv_sec - timer1_start_time.tv_sec) + (timer1_stop_time.tv_nsec - timer1_start_time.tv_nsec) / 1000000000.);

	free(signature_buffer);
}

void signatureCollection_printDot(struct signatureCollection* collection){
	struct node* 				node_cursor;
	struct signature* 			signature_cursor;
	struct multiColumnPrinter*	printer;
	char 						file_name[SIGNATURE_NAME_MAX_SIZE + 4];
	char 						symbol_str[10];

	graph_register_dotPrint_callback(&(collection->syntax_graph), NULL, signatureCollection_dotPrint_node, NULL, NULL);
	log_info("print symbol dependency (syntax graph) in file: \"collection.dot\"");
	if (graphPrintDot_print(&(collection->syntax_graph), "collection.dot", NULL)){
		log_err("graph printDot returned error code");
		return;
	}

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer == NULL){
		log_err("unable to create multiColumnPrinter");
		return;
	}

	multiColumnPrinter_set_column_size(printer, 0, SIGNATURE_NAME_MAX_SIZE);
	multiColumnPrinter_set_column_size(printer, 1, SIGNATURE_NAME_MAX_SIZE);
	multiColumnPrinter_set_column_size(printer, 2, sizeof(file_name) - 1);
	multiColumnPrinter_set_column_size(printer, 3, sizeof(symbol_str) - 1);

	multiColumnPrinter_set_title(printer, 0, "NAME");
	multiColumnPrinter_set_title(printer, 1, "SYMBOL");
	multiColumnPrinter_set_title(printer, 2, "FILE_NAME");
	multiColumnPrinter_set_title(printer, 3, "RESOLVE");

	multiColumnPrinter_print_header(printer);

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = signatureCollection_node_get_signature(node_cursor);

		snprintf(file_name, sizeof(file_name), "%s.dot", signature_cursor->name);

		if (signature_cursor->symbol_table == NULL){
			symbol_str[0] = '\0';
		}
		else{
			uint32_t j;
			uint32_t nb_resolved;

			for (j = 0, nb_resolved = 0; j < signature_cursor->symbol_table->nb_symbol; j++){
				if (signatureSymbol_is_resolved(signature_cursor->symbol_table->symbols + j)){
					nb_resolved ++;
				}
			}

			snprintf(symbol_str, sizeof(symbol_str), "%u/%u", nb_resolved, signature_cursor->symbol_table->nb_symbol);
		}

		if (graphPrintDot_print(&(signature_cursor->graph), file_name, signature_cursor)){
			log_err("graph printDot returned error code");
		}

		multiColumnPrinter_print(printer, signature_cursor->name, signature_cursor->symbol, file_name, symbol_str, NULL);
	}

	multiColumnPrinter_delete(printer);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void signatureCollection_dotPrint_node(void* data, FILE* file, void* arg){
	struct signature* signature = (struct signature*)data;

	fprintf(file, "[label=\"%s\"]", signature->name);
}

void signatureCollection_clean(struct signatureCollection* collection){
	struct node* node_cursor;

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_clean(signatureCollection_node_get_signature(node_cursor));
	}

	graph_clean(&(collection->syntax_graph));
}