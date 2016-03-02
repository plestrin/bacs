#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <search.h>

#include "signatureCollection.h"
#include "graphPrintDot.h"
#include "multiColumn.h"
#include "base.h"

#define SIGNATURE_NAMEENGINE_MAX_ENTRY 512

struct nameEngineData{
	uint32_t 					id;
	char 						name[SIGNATURE_NAME_MAX_SIZE];
	struct signatureCollection* collection;
};

struct nameEngine{
	struct hsearch_data 	hashtable;
	uint32_t 				nb_entry;
	uint32_t 				ref_count;
	struct nameEngineData 	entry_buffer[SIGNATURE_NAMEENGINE_MAX_ENTRY];
};

static struct nameEngine engine;

static void nameEngine_get(void){
	if (engine.ref_count == 0){
		if (hcreate_r(SIGNATURE_NAMEENGINE_MAX_ENTRY, &(engine.hashtable)) == 0){
			log_err("unable to create hashTable");
			return;
		}
	}
	engine.ref_count ++;
}

static uint32_t nameEngine_register_symbol(const char* name, struct signatureCollection* collection){
	ENTRY 					item;
	ENTRY* 					result;
	struct nameEngineData* 	data;

	if (engine.ref_count == 0){
		log_err("reference counter is equal to 0");
		return SIGNATURESYMBOL_RAW_ID;
	}

	if (engine.nb_entry == SIGNATURE_NAMEENGINE_MAX_ENTRY){
		log_err("the max number of entry has been reached");
		return SIGNATURESYMBOL_RAW_ID;
	}

	strncpy(engine.entry_buffer[engine.nb_entry].name, name, SIGNATURE_NAME_MAX_SIZE);

	data = engine.entry_buffer + engine.nb_entry;
	data->id = engine.nb_entry + 1;
	data->collection = collection;

	item.key = data->name;
	item.data = data;

	if (hsearch_r(item, ENTER, &result, &(engine.hashtable)) == 0){
		log_err("something went wrong with hsearch_r");
		return SIGNATURESYMBOL_RAW_ID;
	}

	if (result->data == item.data){
		engine.nb_entry ++;
	}

	data = (struct nameEngineData*)(result->data);

	if (data->collection != collection){
		log_warn_m("symbol: \"%s\" has been registered with a different collection", name);
	}

	return data->id;
}

static uint32_t nameEngine_search_symbol(char* name, struct signatureCollection* collection){
	ENTRY 					item;
	ENTRY* 					result;
	struct nameEngineData* 	data;

	if (engine.ref_count == 0){
		log_err("reference counter is equal to 0");
		return SIGNATURESYMBOL_RAW_ID;
	}

	item.key = name;

	if (hsearch_r(item, FIND, &result, &(engine.hashtable)) != 0){
		data = (struct nameEngineData*)(result->data);
		if (collection == NULL || data->collection == collection){
			return data->id;
		}
		else{
			log_warn_m("symbol: \"%s\" has been registered with a different collection", name);
		}
	}
	else if (errno != ESRCH){
		log_err("something went wrong with hsearch_r");
		return SIGNATURESYMBOL_RAW_ID;
	}

	return SIGNATURESYMBOL_RAW_ID;
}

static void nameEngine_release(void){
	if (engine.ref_count == 0){
		log_err("reference counter is equal to 0");
		return;
	}

	engine.ref_count --;
	if (engine.ref_count == 0){
		hdestroy_r(&(engine.hashtable));
		engine.nb_entry = 0;
	}
}

void signatureSymbol_register(struct signatureSymbol* symbol, const char* export_name, struct signatureCollection* collection){
	symbol->id = nameEngine_register_symbol(export_name, collection);
}

void signatureSymbol_fetch(struct signatureSymbol* symbol, struct signatureCollection* collection){
	symbol->id = nameEngine_search_symbol(symbol->name, collection);
}

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

void signatureCollection_init(struct signatureCollection* collection, size_t custom_signature_size, struct signatureCallback* callback){
	graph_init(&(collection->syntax_graph), custom_signature_size, sizeof(uint32_t));
	memcpy(&(collection->callback), callback, sizeof(struct signatureCallback));
	nameEngine_get();
}

int32_t signatureCollection_add(struct signatureCollection* collection, void* custom_signature, const char* export_name){
	struct node* 		syntax_node;
	struct node* 		node_cursor;
	struct signature* 	signature;
	struct signature* 	signature_cursor;
	uint32_t 			i;
	uint32_t 			nb_unresolved_symbol 	= 0;

	syntax_node = graph_add_node(&(collection->syntax_graph), custom_signature);
	if (syntax_node == NULL){
		log_err("unable to add code signature to the collection's syntax graph");
		return -1;
	}

	signature = signatureCollection_node_get_signature(syntax_node);
	signatureSymbol_register(&(signature->symbol), export_name, collection);
	if (!signatureSymbol_is_resolved(&(signature->symbol))){
		log_err_m("unable to register signature: \"%s\" with the export name: \"%s\"", signature->symbol.name, export_name);
		graph_remove_node(&(collection->syntax_graph), syntax_node);
		return -1;
	}

	if (signature->symbol_table != NULL){
		for (i = 0; i < signature->symbol_table->nb_symbol; i++){
			signatureSymbol_fetch(signature->symbol_table->symbols + i, collection);

			if (signatureSymbol_is_resolved(signature->symbol_table->symbols + i)){
				for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
					signature_cursor = signatureCollection_node_get_signature(node_cursor);

					if (signature != signature_cursor && signature->symbol_table->symbols[i].id == signature_cursor->symbol.id){
						if (graph_add_edge(&(collection->syntax_graph), node_cursor, syntax_node, &i) == NULL){
							log_err("unable to add edge to the syntax tree");
						}
					}
				}
			}
			else{
				nb_unresolved_symbol ++;
			}
		}
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

		if (signature != signature_cursor && signature_cursor->symbol_table != NULL){
			for (i = 0, nb_unresolved_symbol = 0; i < signature_cursor->symbol_table->nb_symbol; i++){
				if (signatureSymbol_is_resolved(signature_cursor->symbol_table->symbols + i)){
					if (signature_cursor->symbol_table->symbols[i].id == signature->symbol.id){
						if (graph_add_edge(&(collection->syntax_graph), syntax_node, node_cursor, &i) == NULL){
							log_err("unable to add edge to the syntax tree");
						}
					}
				}
				else if (!strncmp(signature_cursor->symbol_table->symbols[i].name, export_name, SIGNATURE_NAME_MAX_SIZE)){
					signature_cursor->symbol_table->symbols[i].id = signature->symbol.id;

					if (graph_add_edge(&(collection->syntax_graph), syntax_node, node_cursor, &i) == NULL){
						log_err("unable to add edge to the syntax tree");
					}
				}
				else{
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

								if (child->symbol.id == signature_cursor->symbol_table->symbols[j].id && signature_state_is_found(child)){
									break;
								}
							}
							if (edge_cursor == NULL){
								signature_state_set_search(signature_cursor);
								goto next1;
							}
						}
						else{
							log_warn_m("unresolved symbol(s) for signature %s", signature_cursor->symbol.name);
							signature_state_set_search(signature_cursor);
							goto next1;
						}
					}
				}

				if (signature_cursor->sub_graph_handle == NULL){
					log_warn_m("sub_graph_handle is still NULL for %s. It may be due to unresolved symbol(s)", signature_cursor->symbol.name);
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
						multiColumnPrinter_print(printer, signature_buffer[j]->symbol.name, array_get_length(assignement_array), timer2_elapsed_time, NULL);
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
	multiColumnPrinter_set_column_size(printer, 1, 3);
	multiColumnPrinter_set_column_size(printer, 2, sizeof(file_name) - 1);
	multiColumnPrinter_set_column_size(printer, 3, sizeof(symbol_str) - 1);

	multiColumnPrinter_set_title(printer, 0, "NAME");
	multiColumnPrinter_set_title(printer, 1, "ID");
	multiColumnPrinter_set_title(printer, 2, "FILE_NAME");
	multiColumnPrinter_set_title(printer, 3, "RESOLVE");

	multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_INT32);

	multiColumnPrinter_print_header(printer);

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = signatureCollection_node_get_signature(node_cursor);

		snprintf(file_name, sizeof(file_name), "%s.dot", signature_cursor->symbol.name);

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

		multiColumnPrinter_print(printer, signature_cursor->symbol.name, signature_cursor->symbol.id, file_name, symbol_str, NULL);
	}

	multiColumnPrinter_delete(printer);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void signatureCollection_dotPrint_node(void* data, FILE* file, void* arg){
	struct signature* signature = (struct signature*)data;

	fprintf(file, "[label=\"%s\"]", signature->symbol.name);
}

void signatureCollection_clean(struct signatureCollection* collection){
	struct node* node_cursor;

	for (node_cursor = graph_get_head_node(&(collection->syntax_graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_clean(signatureCollection_node_get_signature(node_cursor));
	}

	graph_clean(&(collection->syntax_graph));
	nameEngine_release();
}