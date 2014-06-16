#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>
#include <alloca.h>

#include "graphMining.h"

int32_t compare_labelTabItem(const void* arg1, const void* arg2);
int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2);
int32_t compare_candidatKPath(const void* arg1, const void* arg2);

void debug_print_path(struct path* path, uint32_t length);
void debug_print_graph(struct graphMatrice* graph_matrice, uint32_t nb_path);
void debug_print_isomorphism(struct graphMatrice* m1, struct graphMatrice* m2, uint32_t nb_path);

#pragma GCC diagnostic ignored "-Wunused-parameter"
void dumb_tsearch_cleanner(void* arg){}

struct graphMining* graphMining_create(){
	struct graphMining* graph_mining;

	graph_mining = (struct graphMining*)malloc(sizeof(struct graphMining));
	if (graph_mining != NULL){
		if (graphMining_init(graph_mining)){
			printf("ERROR: in %s, unable to init graphMining structure\n", __func__);
			graph_mining = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return graph_mining;
}


int32_t graphMining_init(struct graphMining* graph_mining){
	graph_mining->nb_label 		= 0;
	graph_mining->label_tab 	= NULL;
	graph_mining->label_fast 	= NULL;

	if (array_init(&(graph_mining->frequent_path_array), sizeof(struct frequentKPath))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return -1;
	}

	if (array_init(&(graph_mining->frequent_graph_array), sizeof(struct graphMatriceBuffer*))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		array_clean(&(graph_mining->frequent_path_array));
		return -1;
	}

	return 0;
}

int32_t graphMining_init_label_tab(struct graph* graph, struct graphMining* graph_mining, uint32_t(*node_get_label)(struct node*)){
	struct node* 	node_cursor;
	uint32_t 		i;

	graph_mining->label_tab = (struct labelTabItem*)malloc(sizeof(struct labelTabItem) * graph->nb_node);
	if (graph_mining->label_tab == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	node_cursor = graph_get_head_node(graph);
	i = 0;
	while(node_cursor != NULL){
		graph_mining->label_tab[i].label = node_get_label(node_cursor);
		graph_mining->label_tab[i].node = node_cursor;

		node_cursor = node_get_next(node_cursor);
		i++;
	}

	if (i != graph->nb_node){
		printf("ERROR: in %s, incorrect number of node: %u vs %u", __func__, i, graph->nb_node);
		return -1;
	}

	return 0;
}

int32_t graphMining_init_label_fast(struct graph* graph, struct graphMining* graph_mining){
	uint32_t i;
	uint32_t prev_label_value;
	uint32_t prev_label_index;

	qsort (graph_mining->label_tab, graph->nb_node, sizeof(struct labelTabItem), compare_labelTabItem);

	for (i = 1, graph_mining->nb_label = 1, prev_label_value = graph_mining->label_tab[0].label; i < graph->nb_node; i++){
		if (graph_mining->label_tab[i].label != prev_label_value){
			graph_mining->nb_label ++;
			prev_label_value = graph_mining->label_tab[i].label;
		}
	}

	graph_mining->label_fast = (struct labelFastAccess*)malloc(sizeof(struct labelFastAccess) * graph_mining->nb_label);
	if (graph_mining->label_fast == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	/* pour le debug */
	printf("Nb label: %u\n", graph_mining->nb_label);

	for (i = 0; i < graph->nb_node; i++){
		if (i == 0){
			graph_mining->label_fast[0].label 	= graph_mining->label_tab[0].label;
			graph_mining->label_fast[0].offset 	= 0;
			graph_mining->label_fast[0].size 	= 1;
			prev_label_index = 0;
		}
		else{
			if (graph_mining->label_tab[i].label == graph_mining->label_fast[prev_label_index].label){
				graph_mining->label_fast[prev_label_index].size ++;
			}
			else{
				prev_label_index  ++;
				graph_mining->label_fast[prev_label_index].label 	= graph_mining->label_tab[i].label;
				graph_mining->label_fast[prev_label_index].offset 	= i;
				graph_mining->label_fast[prev_label_index].size 	= 1;
			}
		}
		graph_mining->label_tab[i].node->mining_ptr = (void*)(graph_mining->label_tab + i);
		graph_mining->label_tab[i].fastAccess_index = prev_label_index;
	}

	return 0;
}

int32_t graphMining_search_frequent_edge(struct graphMining* graph_mining, struct frequentKPath* frequent_k_path){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t  		index;
	uint32_t 		nb_occurence;

	frequent_k_path->nb_path 	= 0;
	frequent_k_path->length 	= 2;
	frequent_k_path->paths 		= (struct path*)malloc(sizeof_path(2) * graph_mining->nb_label * graph_mining-> nb_label);
	if (frequent_k_path->paths == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for (i = 0; i < graph_mining->nb_label; i++){
		for (j = 0; j < graph_mining->nb_label; j++){
			index = i * graph_mining->nb_label + j;

			frequent_k_path->paths[index].labels[0] = graph_mining->label_fast[i].label;
			frequent_k_path->paths[index].labels[1] = graph_mining->label_fast[j].label;

			nb_occurence = graphMining_evaluate_path_frequency(graph_mining, frequent_k_path->paths + index, 2);
			if (nb_occurence >= GRAPHMINING_MIN_OCCURENCE){
				if (index != frequent_k_path->nb_path){
					memcpy(frequent_k_path->paths + frequent_k_path->nb_path, frequent_k_path->paths + index, sizeof_path(2));
				}
				frequent_k_path->nb_path ++;

				/* pour le debug */
				printf("\t%c -> %c : %u\n", (char)(frequent_k_path->paths[index].labels[0]), (char)(frequent_k_path->paths[index].labels[1]), nb_occurence);
			}
		}
	}

	/* pour le debug */
	printf("Nb frequent 1-path: %u\n", frequent_k_path->nb_path);

	if (frequent_k_path->nb_path != 0){
		frequent_k_path->paths = (struct path*)realloc(frequent_k_path->paths, sizeof_path(2) * frequent_k_path->nb_path);
		if (frequent_k_path->paths == NULL){
			printf("ERROR: in %s, unable to realloc memory\n", __func__);
			return -1;
		}
	}
	else{
		free(frequent_k_path->paths);
		frequent_k_path->paths = NULL;
	}

	return 0;
}

int32_t graphMining_search_frequent_path(struct graphMining* graph_mining, uint32_t k){
	struct frequentKPath 	next_frequent_k_path;

	if (k == 1){
		if(graphMining_search_frequent_edge(graph_mining, &next_frequent_k_path)){
			printf("ERROR: in %s, unable to search frequent edge\n", __func__);
			return -1;
		}
	}
	else{
		struct frequentKPath* 	prev_frequent_k_path;
		struct frequentKPath*	frequent_edge;
		uint32_t 				i;
		uint32_t 				j;
		struct candidatKPath* 	ll_root = NULL;
		struct candidatKPath* 	ll_last = NULL;
		struct candidatKPath* 	ll_new = NULL;
		struct candidatKPath** 	ll_result;
		struct candidatKPath* 	ll_current;
		struct candidatKPath* 	ll_delete;
		void* 					tree_root = NULL;

		next_frequent_k_path.length = k + 1;
		next_frequent_k_path.nb_path = 0;

		prev_frequent_k_path = (struct frequentKPath*)array_get(&(graph_mining->frequent_path_array), k - 2);
		frequent_edge = (struct frequentKPath*)array_get(&(graph_mining->frequent_path_array), 0);
		for (i = 0; i < prev_frequent_k_path->nb_path; i++){
			for (j = 0; j < frequent_edge->nb_path; j++, ll_new = NULL){
				if (get_path_value(prev_frequent_k_path->paths, i, k).labels[0] == frequent_edge->paths[j].labels[1]){
					ll_new = (struct candidatKPath*)malloc(sizeof_candidatKPath(k + 1));
					if (ll_new == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
					}
					else{
						ll_new->length = k + 1;
						ll_new->path.labels[0] = frequent_edge->paths[j].labels[0];
						memcpy(ll_new->path.labels + 1, get_path_value(prev_frequent_k_path->paths, i, k).labels, k * sizeof(uint32_t));
					}
				}

				if (get_path_value(prev_frequent_k_path->paths, i, k).labels[k - 1] == frequent_edge->paths[j].labels[0]){
					ll_new = (struct candidatKPath*)malloc(sizeof_candidatKPath(k + 1));
					if (ll_new == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
					}
					else{
						ll_new->length = k + 1;
						memcpy(ll_new->path.labels, get_path_value(prev_frequent_k_path->paths, i, k).labels, k * sizeof(uint32_t));
						ll_new->path.labels[k] = frequent_edge->paths[j].labels[1];
					}
				}

				if (ll_new != NULL){
					ll_result = (struct candidatKPath**)tsearch(ll_new, &tree_root, compare_candidatKPath);
					if (ll_result == NULL){
						printf("ERROR: in %s, tsearch return NULL\n", __func__);
						free(ll_new);
						continue;
					}
					if (*ll_result != ll_new){
						free(ll_new);
					}
					else{
						next_frequent_k_path.nb_path ++;
						if (ll_last == NULL){
							ll_root = ll_new;
							ll_last = ll_new;
							ll_new->prev = NULL;
							ll_new->next = NULL;
						}
						else{
							ll_last->next = ll_new;
							ll_new->prev = ll_last;
							ll_new->next = NULL;
							ll_last = ll_new;
						}
					}
				}
			}
		}

		tdestroy(tree_root, dumb_tsearch_cleanner);

		ll_current = ll_root;
		while(ll_current != NULL){
			uint32_t nb_occurence;

			nb_occurence = graphMining_evaluate_path_frequency(graph_mining, &(ll_current->path), k + 1);
			if (nb_occurence >= GRAPHMINING_MIN_OCCURENCE){
				/* pour le debug */
				for (i = 0; i < k + 1; i++){
					if (i == 0){
						printf("\t%c -> ", (char)(ll_current->path.labels[0]));
					}
					else if (i == k){
						printf("%c : %u\n", (char)(ll_current->path.labels[i]), nb_occurence);
					}
					else{
						printf("%c -> ", (char)(ll_current->path.labels[i]));
					}
				}

				ll_current = ll_current->next;
			}
			else{
				ll_delete = ll_current;

				if (ll_current == ll_root){
					ll_root = ll_current->next;
				}
				else{
					ll_current->prev->next = ll_current->next;
				}

				if (ll_current == ll_last){
					ll_last = ll_current->prev;
				}
				else{
					ll_current->next->prev = ll_current->prev; 
				}

				ll_current = ll_current->next;
				free(ll_delete);
				next_frequent_k_path.nb_path --;
			}
		}

		/* pour le debug */
		printf("Nb frequent %u-path: %u\n", k, next_frequent_k_path.nb_path);

		if (next_frequent_k_path.nb_path > 0){
			next_frequent_k_path.paths = (struct path*)malloc(sizeof_path(k + 1) * next_frequent_k_path.nb_path);
			if (next_frequent_k_path.paths == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				ll_current = ll_root;
				while(ll_current != NULL){
					ll_delete = ll_current;
					ll_current = ll_current->next;
					free(ll_delete);
				}
			}
			else{
				uint32_t offset = 0;

				ll_current = ll_root;
				while(ll_current != NULL){
					ll_delete = ll_current;
					memcpy(get_path_ptr(next_frequent_k_path.paths, offset, k + 1), &(ll_current->path), sizeof_path(k + 1));
					offset ++;
					ll_current = ll_current->next;
					free(ll_delete);
				}
			}
		}
	}

	if (next_frequent_k_path.nb_path > 0){
		if (array_add(&(graph_mining->frequent_path_array), &next_frequent_k_path) != (int32_t)(k - 1)){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
			return -1;
		}

		return graphMining_search_frequent_path(graph_mining, k + 1);
	}

	return 0;
}

int32_t graphMining_search_frequent_2_graph(struct graphMining* graph_mining){
	uint32_t 						nb_frequent_path = 0;
	uint32_t 						nb_frequent_2_graph = 0;
	uint32_t 						sizeof_frequent_2_graph = 0;
	struct frequentPathFastAccess*	frequent_path_fast_access;
	uint32_t 						i;
	uint32_t 						j;
	uint32_t 						offset;
	struct frequentKPath* 			frequent_k_path;
	struct array 					graph_array;
	uint32_t* 						graph_occurrence;
	struct graphMatriceBuffer* 		graph_matrice_buffer;
	struct graphMatrice* 			graph_matrice;

	for (i = 0; i < array_get_length(&(graph_mining->frequent_path_array)); i++){
		frequent_k_path = (struct frequentKPath*)array_get(&(graph_mining)->frequent_path_array, i);
		nb_frequent_path += frequent_k_path->nb_path;
	}

	frequent_path_fast_access = (struct frequentPathFastAccess*)malloc(sizeof(struct frequentPathFastAccess) * nb_frequent_path);
	if (frequent_path_fast_access == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}


	if (array_init(&graph_array, sizeof(struct graphMatrice*))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		free(frequent_path_fast_access);
		return -1;
	}

	for (i = 0, offset = 0; i < array_get_length(&(graph_mining->frequent_path_array)); i++){
		frequent_k_path = (struct frequentKPath*)array_get(&(graph_mining)->frequent_path_array, i);
		for (j = 0; j < frequent_k_path->nb_path; j++){
			frequent_path_fast_access[offset].length = frequent_k_path->length;
			frequent_path_fast_access[offset].path = get_path_ptr(frequent_k_path->paths, j, frequent_k_path->length);

			offset ++;
		}
	}

	for (i = 0; i < nb_frequent_path; i++){
		for (j = 0; j <= i; j++){
			if (graphMining_combine_paths(frequent_path_fast_access + i, frequent_path_fast_access + j, &graph_array)){
				printf("ERROR: in %s, unable to combine path %d and %d\n", __func__, i, j);
			}
		}
	}

	if (array_get_length(&graph_array) > 0){
		graph_occurrence = (uint32_t*)malloc(sizeof(uint32_t) * array_get_length(&graph_array));
		if (graph_occurrence == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
		}
		else{
			for (i = 0; i < array_get_length(&graph_array); i++){
				graph_matrice = *(struct graphMatrice**)array_get(&graph_array, i);
				graph_occurrence[i] = graphMining_evaluate_subgraph_frequency(graph_mining, graph_matrice, 2);
				if (graph_occurrence[i] >= GRAPHMINING_MIN_OCCURENCE){
					nb_frequent_2_graph ++;
					sizeof_frequent_2_graph += sizeof_graphMatrice(2, graph_matrice->nb_node);
				}
			}

			if (nb_frequent_2_graph > 0){
				graph_matrice_buffer = graphMatriceBuffer_create(nb_frequent_2_graph, sizeof_frequent_2_graph);
				if (graph_matrice_buffer != NULL){
					printf("Nb 2 path graph: %u -> frequent: %u\n", array_get_length(&graph_array), nb_frequent_2_graph); /* pour le debug */
					for (i = 0, nb_frequent_2_graph = 0; i < array_get_length(&graph_array); i++){
						if (graph_occurrence[i] >= GRAPHMINING_MIN_OCCURENCE){
							graph_matrice = *(struct graphMatrice**)array_get(&graph_array, i);
							memcpy(graph_matrice_buffer->graph_matrices[nb_frequent_2_graph], graph_matrice, sizeof_graphMatrice(2, graph_matrice->nb_node));
							nb_frequent_2_graph ++;
							if (nb_frequent_2_graph != graph_matrice_buffer->nb_graph_matrice){
								graph_matrice_buffer->graph_matrices[nb_frequent_2_graph] = (struct graphMatrice*)((char*)graph_matrice_buffer->graph_matrices[nb_frequent_2_graph - 1] + sizeof_graphMatrice(2, graph_matrice->nb_node));
							}

							/* pour le debug */
							printf("Occurence: %u -> ", graph_occurrence[i]);
							debug_print_graph(graph_matrice, 2);
						}
					}

					if (array_add(&(graph_mining->frequent_graph_array), &graph_matrice_buffer) < 0){
						printf("ERROR: in %s, unable to add element to array\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to create graphMatriceBuffer structure\n", __func__);
				}
			}
			
			free(graph_occurrence);
		}
	}

	for (i = 0; i < array_get_length(&graph_array); i++){
		free(*(struct graphMatrice**)array_get(&graph_array, i));
	}

	array_clean(&graph_array);
	free(frequent_path_fast_access);

	return 0;
}

uint32_t graphMining_evaluate_path_frequency(struct graphMining* graph_mining, struct path* path, uint32_t length){
	uint32_t 				nb_occurence = 0;
	struct labelFastAccess* label_fast_access;
	uint32_t 				i;
	struct node* 			current_node;

	label_fast_access = (struct labelFastAccess*)bsearch (path->labels, graph_mining->label_fast, graph_mining->nb_label, sizeof(struct labelFastAccess), compare_key_labelFastAccess);
	if (label_fast_access == NULL){
		printf("ERROR: in %s, this case is not supposed to happen: no fast access for the required label\n", __func__);
		return 0;
	}

	for (i = 0; i < label_fast_access->size; i++){
		current_node = graph_mining->label_tab[label_fast_access->offset + i].node;
		nb_occurence += graphMining_count_path(path->labels + 1, current_node, length - 1);
	}

	return nb_occurence;
}

uint32_t graphMining_count_path(uint32_t* labels, struct node* node, uint32_t length){
	uint32_t 		nb_occurence = 0;
	struct edge* 	current_edge;
	struct node* 	child;

	current_edge = node_get_head_edge_src(node);
	while(current_edge != NULL){
		child = edge_get_dst(current_edge);
		if (node_get_mining_ptr(child)->label == labels[0]){
			if (length != 1){
				nb_occurence += graphMining_count_path(labels + 1, child, length - 1);
			}
			else{
				nb_occurence ++;
			}
		}
		current_edge = edge_get_next_src(current_edge);
	}

	return nb_occurence;
}

uint32_t graphMining_evaluate_subgraph_frequency(struct graphMining* graph_mining, struct graphMatrice* graph_matrice, uint32_t nb_path){
	uint32_t 					result = 0;
	struct possibleAssignement* possible_assignement;
	struct node** 				assignement;

	possible_assignement = possibleAssignement_create_init_first(graph_mining, graph_matrice, nb_path);
	if (possible_assignement == NULL){
		printf("ERROR: in %s, unable to create first possible assignement\n", __func__);
	}
	else{
		assignement = (struct node**)alloca(sizeof(struct node*) * graph_matrice->nb_node);
		result = graphMining_count_subgraph(graph_matrice, nb_path, assignement, 0, possible_assignement);
		possibleAssignement_delete(possible_assignement);
	}

	return result;
}

uint32_t graphMining_count_subgraph(struct graphMatrice* graph_matrice, uint32_t nb_path, struct node** assignement, uint32_t nb_assignement, struct possibleAssignement* possible_assignement){
	uint32_t 		i;
	uint32_t 		result = 0;
	uint32_t 		local_result;

	possibleAssignement_update(possible_assignement, graph_matrice, nb_assignement, nb_path);

	#if 0
	if (nb_assignement > 1){
		uint32_t 		j;
		struct edge* 	edge_cursor;
		uint8_t 		match;

		for (i = 0; i < nb_path; i++){
			if (graphMatrice_get_line(graph_matrice, nb_assignement - 1, nb_path)->node[i] > 0){
				for (j = nb_assignement - 1; j > 0; j--){
					if (graphMatrice_get_line(graph_matrice, j - 1, nb_path)->node[i] > 0){
						edge_cursor = node_get_head_edge_dst(assignement[nb_assignement - 1]);
						match = 0;

						while(edge_cursor != NULL){
							if (assignement[j - 1] == edge_get_src(edge_cursor)){
								match = 1;
								break;
							}

							edge_cursor = edge_get_next_dst(edge_cursor);
						}

						if (!match){
							return 0;
						}
						break;
					}
				}
			}
		}
	}
	#endif

	if (nb_assignement == graph_matrice->nb_node){
		return 1;
	}

	for (i = 0; i < possible_assignement->headers[nb_assignement].nb_possible_assignement; i++){
		if (possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i] != NULL){
			struct possibleAssignement* new_possible_assignement;

			new_possible_assignement = possibleAssignement_duplicate(possible_assignement, nb_assignement, possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i]);
			if (new_possible_assignement != NULL){
				assignement[nb_assignement] = possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i];
				local_result = graphMining_count_subgraph(graph_matrice, nb_path, assignement, nb_assignement + 1, new_possible_assignement);
				possibleAssignement_delete(new_possible_assignement);

				if (local_result > 0){
					result += local_result;
				}
				else{
					possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i] = NULL;
					possibleAssignement_update(possible_assignement, graph_matrice, nb_assignement, nb_path);
				}
			}
			else{
				printf("ERROR: in %s, unable to duplicate possible assignement\n", __func__);
			}
		}
	}

	return result;
}

struct possibleAssignement* possibleAssignement_create(uint32_t nb_node, uint32_t nb_assignement){
	struct possibleAssignement* possible_assignement;

	possible_assignement = (struct possibleAssignement*)malloc(sizeof(struct possibleAssignement) + nb_node * sizeof(struct possibleAssignementHeader) + nb_assignement * sizeof(struct node*));
	if (possible_assignement != NULL){
		possible_assignement->nb_node 	= nb_node;
		possible_assignement->headers 	= (struct possibleAssignementHeader*)((char*)possible_assignement + sizeof(struct possibleAssignement));
		possible_assignement->nodes 	= (struct node**)((char*)possible_assignement->headers + nb_node * sizeof(struct possibleAssignementHeader));
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return possible_assignement;
}

struct possibleAssignement* possibleAssignement_create_init_first(struct graphMining* graph_mining, struct graphMatrice* graph_matrice, uint32_t nb_path){
	uint32_t 					nb_assignement = 0;
	uint32_t 					i;
	uint32_t 					j;
	struct labelFastAccess* 	label_fast_access;
	struct possibleAssignement* possible_assignement;

	for	(i = 0; i < graph_matrice->nb_node; i++){
		label_fast_access = (struct labelFastAccess*)bsearch (&(graphMatrice_get_line(graph_matrice, i, nb_path)->label), graph_mining->label_fast, graph_mining->nb_label, sizeof(struct labelFastAccess), compare_key_labelFastAccess);
		if (label_fast_access == NULL){
			printf("ERROR: in %s, this case is not supposed to happen: no fast access for the required label\n", __func__);
			return NULL;
		}
		nb_assignement += label_fast_access->size;
	}

	possible_assignement = possibleAssignement_create(graph_matrice->nb_node, nb_assignement);
	if (possible_assignement != NULL){
		possible_assignement->headers[0].node_offset = 0;

		for	(i = 0; i < graph_matrice->nb_node; i++){
			label_fast_access = (struct labelFastAccess*)bsearch (&(graphMatrice_get_line(graph_matrice, i, nb_path)->label), graph_mining->label_fast, graph_mining->nb_label, sizeof(struct labelFastAccess), compare_key_labelFastAccess);
			possible_assignement->headers[i].nb_possible_assignement = label_fast_access->size;
			if (i != graph_matrice->nb_node - 1){
				possible_assignement->headers[i + 1].node_offset = possible_assignement->headers[i].node_offset + label_fast_access->size;
			}

			for (j = 0; j < label_fast_access->size; j++){
				possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] = graph_mining->label_tab[label_fast_access->offset + j].node;
			}
		}
	}

	return possible_assignement;
}

struct possibleAssignement* possibleAssignement_duplicate(struct possibleAssignement* possible_assignement, uint32_t new_assignement_index, struct node* new_assignement_value){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					nb_assignement = 0;
	struct possibleAssignement* new_possible_assignement;

	for (i = 0; i < possible_assignement->nb_node; i++){
		if (i != new_assignement_index){
			for (j = 0; j < possible_assignement->headers[i].nb_possible_assignement; j++){
				if (possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] != NULL && possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] != new_assignement_value){
					nb_assignement ++;
				}
			}
		}
		else{
			nb_assignement ++;
		}
	}

	new_possible_assignement = possibleAssignement_create(possible_assignement->nb_node, nb_assignement);
	if (new_possible_assignement != NULL){
		new_possible_assignement->headers[0].node_offset = 0;

		for (i = 0; i < possible_assignement->nb_node; i++){
			nb_assignement = 0;

			if (i != new_assignement_index){
				for (j = 0; j < possible_assignement->headers[i].nb_possible_assignement; j++){
					if (possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] != NULL && possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] != new_assignement_value){
						new_possible_assignement->nodes[new_possible_assignement->headers[i].node_offset + nb_assignement] = possible_assignement->nodes[possible_assignement->headers[i].node_offset + j];
						nb_assignement ++;
					}
				}
			}
			else{
				new_possible_assignement->nodes[new_possible_assignement->headers[i].node_offset + nb_assignement] = new_assignement_value;
				nb_assignement ++;
			}

			new_possible_assignement->headers[i].nb_possible_assignement = nb_assignement;
			if (i != possible_assignement->nb_node - 1){
				new_possible_assignement->headers[i + 1].node_offset = new_possible_assignement->headers[i].node_offset + nb_assignement;
			}
		}
	}

	return new_possible_assignement;
}

void possibleAssignement_update(struct possibleAssignement* possible_assignement, struct graphMatrice* graph_matrice, uint32_t nb_assignement, uint32_t nb_path){
	uint8_t 		match;
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		l;
	uint32_t 		m;
	struct edge* 	edge;

	start:
	for (i = nb_assignement; i < graph_matrice->nb_node; i++){
		for (j = 0; j < possible_assignement->headers[i].nb_possible_assignement; j++){
			if (possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] != NULL){
				for (k = 0; k < nb_path; k++){
					if (graphMatrice_get_line(graph_matrice, i, nb_path)->node[k] > 0){
						for (l = i; l > 0; l--){
							if (graphMatrice_get_line(graph_matrice, l - 1, nb_path)->node[k] > 0){
								match = 0;

								for (m = 0; m < possible_assignement->headers[l - 1].nb_possible_assignement && !match; m++){
									edge = node_get_head_edge_dst(possible_assignement->nodes[possible_assignement->headers[i].node_offset + j]);

									while(edge != NULL){
										if (possible_assignement->nodes[possible_assignement->headers[l - 1].node_offset + m] == edge_get_src(edge)){
											match = 1;
											break;
										}
										edge = edge_get_next_dst(edge);
									}
								}
								if (!match){
									possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] = NULL;
									goto start;
								}
								break;
							}
						}
					}
				}
			}
		}
	}
}

int32_t graphMining_combine_paths(struct frequentPathFastAccess* path_access1, struct frequentPathFastAccess* path_access2, struct array* graph_array){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	uint32_t 				l;
	uint32_t 				m;
	uint32_t* 				merge;
	uint32_t 				nb_merge = 0;
	struct graphMatrice* 	graph_matrice;
	uint32_t 				graph_matrice_length;

	merge = (uint32_t*)alloca(2 * sizeof(uint32_t) * ((path_access1->length < path_access2->length) ? path_access1->length : path_access2->length));

	for (i = 0; i < path_access1->length; i++){
		for (j = 0; j < path_access2->length; j++){
			if (!((i == 0 && j == path_access2->length - 1) || (i == path_access1->length - 1 && j == 0)) && path_access1->path->labels[i] == path_access2->path->labels[j]){
				merge[0] = i;
				merge[1] = j;

				nb_merge = 1;
				k = i + 1;
				l = j + 1;

				do{
					while (k < path_access1->length && l < path_access2->length){
						while(l < path_access2->length && path_access1->path->labels[k] != path_access2->path->labels[l]){
							l ++;
						}

						if (l < path_access2->length){
							if (k == merge[2 * (nb_merge - 1)] + 1 && l == merge[2 * (nb_merge - 1) + 1] + 1){
								l ++;
							}
							else{
								merge[2 * nb_merge    ] = k;
								merge[2 * nb_merge + 1] = l;

								nb_merge ++;
								k ++;
								l ++;
							}
						}
						else if (k < path_access1->length){
							k ++;
							l = merge[2 * (nb_merge - 1) + 1] + 1;
						}
					}

					graph_matrice = graphMining_create_graphMatrice_from_path(path_access1, path_access2, merge, nb_merge);
					if (graph_matrice != NULL){
						graph_matrice_length = array_get_length(graph_array);

						for (m = 0; m <= graph_matrice_length; m++){
							if (m == graph_matrice_length){
								if (array_add(graph_array, &graph_matrice) < 0){
									printf("ERROR: in %s, unable to add element to array\n", __func__);
									free(graph_matrice);
								}
							}
							else{
								if (graphMining_are_graphMatrice_isomorphic(graph_matrice, *(struct graphMatrice**)array_get(graph_array, m), 2)){
									free(graph_matrice);
									break;
								}
							}
						}
					}
					else{
						printf("ERROR: in %s, unable to create graphMatrice\n", __func__);
					}

					nb_merge --;
					k = merge[2 * nb_merge] + 1;
					l = merge[2 * nb_merge];

				} while(nb_merge != 0);
			}
		}
	}

	return 0;
}

struct graphMatrice* graphMining_create_graphMatrice_from_path(struct frequentPathFastAccess* path_access1, struct frequentPathFastAccess* path_access2, uint32_t* merge, uint32_t nb_merge){
	struct graphMatrice* 		graph_matrice;
	uint32_t 					i;
	uint32_t					j;
	uint32_t					merge_index;
	uint32_t 					line_index;
	struct graphMatriceLine* 	line;

	graph_matrice = (struct graphMatrice*)calloc(sizeof_graphMatrice(2, path_access1->length + path_access2->length - nb_merge), 1);
	if (graph_matrice != NULL){
		graph_matrice->nb_node = path_access1->length + path_access2->length - nb_merge;

		for (i = 0, j = 0, merge_index = 0, line_index = 0; i < path_access1->length; i++){
			if (merge_index < nb_merge && i == merge[2 * merge_index]){
				while (j < merge[2 * merge_index + 1]){
					line = graphMatrice_get_line(graph_matrice, line_index, 2);

					line->label = path_access2->path->labels[j];
					line->node[1] = j + 1;

					line_index ++;
					j ++;
				}

				line = graphMatrice_get_line(graph_matrice, line_index, 2);

				line->label = path_access2->path->labels[j];
				line->node[0] = i + 1;
				line->node[1] = j + 1;

				line_index ++;
				j ++;
				merge_index ++;
			}
			else{
				line = graphMatrice_get_line(graph_matrice, line_index, 2);

				line->label = path_access1->path->labels[i];
				line->node[0] = i + 1;

				line_index ++;
			}
		}

		for (; j < path_access2->length; j++){
			line = graphMatrice_get_line(graph_matrice, line_index, 2);

			line->label = path_access2->path->labels[j];
			line->node[1] = j + 1;

			line_index ++;
		}

		/* pour le debug */
		if (merge_index != nb_merge){
			printf("Incorrect value at the end for merge_index: %u vs %u\n", merge_index, nb_merge);
		}
		if (graph_matrice->nb_node != line_index){
			printf("Incorrect value at the end for line_index: %u vs %u\n", graph_matrice->nb_node, line_index);
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return graph_matrice;
}

int32_t graphMining_are_graphMatrice_isomorphic(struct graphMatrice* m1, struct graphMatrice* m2, uint32_t nb_path){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	k;
	uint32_t 	m;
	uint16_t* 	matching;
	uint32_t 	matching_offset;
	uint32_t 	nb_node;
	uint8_t* 	taken;
	uint32_t 	multiplicity1;
	uint32_t 	multiplicity2;

	if (m1->nb_node != m2->nb_node){
		return 0;
	}
	else{
		nb_node = m1->nb_node ;
	}

	/* here we can add other heuristic to speed up the process */

	matching = (uint16_t*)alloca(sizeof(uint16_t) * 2 * nb_node);
	taken = (uint8_t*)alloca(sizeof(uint8_t) * nb_node);
	memset(taken, 0, sizeof(uint8_t) * nb_node);

	for (j = 0; j < nb_node; j++){
		if (graphMatrice_get_line(m1, 0, nb_path)->label == graphMatrice_get_line(m2, j, nb_path)->label){

			for (m = 0, multiplicity1 = 0; m < nb_path; m++){
				if (graphMatrice_get_line(m1, 0, nb_path)->node[m]){
					multiplicity1 ++;
				}	
			}

			for (m = 0, multiplicity2 = 0; m < nb_path; m++){
				if (graphMatrice_get_line(m2, j, nb_path)->node[m]){
					multiplicity2 ++;
				}	
			}

			if (multiplicity1 == multiplicity2){
				matching[0] = 0;
				matching[1] = j;
				taken[j] = 1;

				matching_offset = 1;
				i = 1;
				k = 0;

				while(i < nb_node && k < nb_node){
					while (k < nb_node){
						if (taken[k] == 0 && graphMatrice_get_line(m1, i, nb_path)->label == graphMatrice_get_line(m2, k, nb_path)->label){
							
							for (m = 0, multiplicity1 = 0; m < nb_path; m++){
								if (graphMatrice_get_line(m1, i, nb_path)->node[m]){
									multiplicity1 ++;
								}	
							}

							for (m = 0, multiplicity2 = 0; m < nb_path; m++){
								if (graphMatrice_get_line(m2, k, nb_path)->node[m]){
									multiplicity2 ++;
								}	
							}

							if (multiplicity1 == multiplicity2){
								/* This is not correct we need to make further test */

								matching[2 * matching_offset    ] = i;
								matching[2 * matching_offset + 1] = k;
								taken[k] = 1;

								matching_offset ++;
								i ++;
								k = 0;

								break;
							}
							else{
								k ++;
							}
						}
						else{
							k ++;
						}
					}
					if (k == nb_node){
						if (matching_offset > 1){
							matching_offset --;
							taken[matching[2 * matching_offset + 1]] = 0;
							i = matching[2 * matching_offset];
							k = matching[2 * matching_offset + 1] + 1;
						}
						else{
							break;
						}
					}
				}

				if (matching_offset != nb_node){
					matching_offset = 0;
					taken[j] = 0;
				}
				else{
					/* pour le debug */
					debug_print_isomorphism(m1, m2, nb_path);
					return 1;
				}
			}
		}
	}

	/* for debug we can print both graphMatrice if they are isomorphic */
	/* every path are found they intersectect in the right place */

	return 0;
}

struct graphMatriceBuffer* graphMatriceBuffer_create(uint32_t nb_graph_matrice, uint32_t graph_matrice_tot_size){
	struct graphMatriceBuffer* graph_matrice_buffer;

	graph_matrice_buffer = (struct graphMatriceBuffer*)malloc(sizeof(struct graphMatriceBuffer) + nb_graph_matrice * sizeof(struct graphMatrice*) + graph_matrice_tot_size);
	if (graph_matrice_buffer != NULL){
		graph_matrice_buffer->nb_graph_matrice = nb_graph_matrice;
		graph_matrice_buffer->graph_matrices = (struct graphMatrice**)((char*)graph_matrice_buffer + sizeof(struct graphMatriceBuffer));
		graph_matrice_buffer->graph_matrices[0] = (struct graphMatrice*)((char*)graph_matrice_buffer->graph_matrices + nb_graph_matrice * sizeof(struct graphMatrice*));
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return graph_matrice_buffer;
}

int32_t graphMining_mine(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct graphMining* 	graph_mining;
	int32_t 				status = -1;

	graph_mining = graphMining_create();
	if (graph_mining == NULL){
		printf("ERROR: in %s, unable to create graph mining structure\n", __func__);
		return -1;
	}

	if (graphMining_init_label_tab(graph, graph_mining, node_get_label)){
		printf("ERROR: in %s, unable to init label tab\n", __func__);
		goto exit;
	}

	if (graphMining_init_label_fast(graph, graph_mining)){
		printf("ERROR: in %s, unable to init label fast\n", __func__);
		goto exit;
	}

	/* pour le debug */
	printf("\n*** PHASE 1 ***\n");

	if (graphMining_search_frequent_path(graph_mining, 1)){
		printf("ERROR: in %s, unable to search frequent path\n", __func__);
		goto exit;
	}

	/* pour le debug */
	printf("\n*** PHASE 2 ***\n");

	if (graphMining_search_frequent_2_graph(graph_mining)){
		printf("ERROR: in %s, unable to search frequent 2 graph\n", __func__);
		goto exit;
	}

	/* pour le debug */
	printf("\n*** PHASE 3 ***\n");

	/* a completer */

	status = 0;

	exit:
	graphMining_delete(graph_mining);
	return status;
}

void graphMining_clean(struct graphMining* graph_mining){
	uint32_t i;

	if (graph_mining->label_tab != NULL){
		free(graph_mining->label_tab);
		graph_mining->label_tab = NULL;
	}
	if (graph_mining->label_fast != NULL){
		free(graph_mining->label_fast);
		graph_mining->label_fast = NULL;
	}

	for (i = 0; i < array_get_length(&(graph_mining->frequent_path_array)); i++){
		frequentKPath_clean((struct frequentKPath*)array_get(&(graph_mining->frequent_path_array), i));
	}
	array_clean(&(graph_mining->frequent_path_array));

	for (i = 0; i < array_get_length(&(graph_mining->frequent_graph_array)); i++){
		graphMatriceBuffer_delete(*(struct graphMatriceBuffer**)array_get(&(graph_mining->frequent_graph_array), i));
	}
	array_clean(&(graph_mining->frequent_graph_array));
}

/* ===================================================================== */
/* Compare routine 														 */
/* ===================================================================== */

int32_t compare_labelTabItem(const void* arg1, const void* arg2){
	struct labelTabItem* label_item1 = (struct labelTabItem*)arg1;
	struct labelTabItem* label_item2 = (struct labelTabItem*)arg2;

	if (label_item1->label < label_item2->label){
		return -1;
	}
	else if (label_item1->label > label_item2->label){
		return 1;
	}
	else{
		return 0;
	}
}

int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2){
	uint32_t 				key = *(uint32_t*)arg1;
	struct labelFastAccess* fast_access = (struct labelFastAccess*)arg2;

	if (key < fast_access->label){
		return -1;
	}
	else if (key > fast_access->label){
		return 1;
	}
	else{
		return 0;
	}
}

int32_t compare_candidatKPath(const void* arg1, const void* arg2){
	struct candidatKPath* candidat1 = (struct candidatKPath*)arg1;
	struct candidatKPath* candidat2 = (struct candidatKPath*)arg2;
	uint32_t i;

	if (candidat1->length < candidat2->length){
		return -1;
	}
	else if (candidat1->length > candidat2->length){
		return 1;
	}
	else{
		for (i = 0; i < candidat1->length; i++){
			if (candidat1->path.labels[i] < candidat2->path.labels[i]){
				return -1;
			}
			else if (candidat1->path.labels[i] > candidat2->path.labels[i]){
				return 1;
			}
		}
	}

	return 0;
}

/* ===================================================================== */
/* Printing routine 													 */
/* ===================================================================== */

void debug_print_path(struct path* path, uint32_t length){
	uint32_t i;

	for (i = 0; i < length; i++){
		if (i == 0){
			printf("\tl(%u): %c -> ", length, (char)(path->labels[0]));
		}
		else if (i == length - 1){
			printf("%c\n", (char)(path->labels[i]));
		}
		else{
			printf("%c -> ", (char)(path->labels[i]));
		}
	}
}

void debug_print_graph(struct graphMatrice* graph_matrice, uint32_t nb_path){
	uint32_t 					i;
	uint32_t 					j;
	struct graphMatriceLine* 	line;

	printf("GraphMatrice (node=%u, path=%u)\n", graph_matrice->nb_node, nb_path);
	for (i = 0; i < graph_matrice->nb_node; i++){
		line = graphMatrice_get_line(graph_matrice, i, nb_path);
		printf("%c | ", (char)line->label);
		for (j = 0; j < nb_path; j++){
			if (j == nb_path - 1){
				printf("\t%u\n", line->node[j]);
			}
			else if (j == 0){
				printf("%u", line->node[0]);
			}
			else{
				printf("\t%u", line->node[j]);
			}
		}
	}
	printf("\n");
}

void debug_print_isomorphism(struct graphMatrice* m1, struct graphMatrice* m2, uint32_t nb_path){
	uint32_t 					i;
	uint32_t 					j;
	struct graphMatriceLine* 	line;

	printf("GraphMatrice isomorphism (node=%u, path=%u)\n", m1->nb_node, nb_path);
	for (i = 0; i < m1->nb_node; i++){
		line = graphMatrice_get_line(m1, i, nb_path);
		printf("%c | ", (char)line->label);
		for (j = 0; j < nb_path; j++){
			if (j == nb_path - 1){
				printf("\t%u\t\t", line->node[j]);
			}
			else if (j == 0){
				printf("%u", line->node[0]);
			}
			else{
				printf("\t%u", line->node[j]);
			}
		}

		line = graphMatrice_get_line(m2, i, nb_path);
		printf("%c | ", (char)line->label);
		for (j = 0; j < nb_path; j++){
			if (j == nb_path - 1){
				printf("\t%u\n", line->node[j]);
			}
			else if (j == 0){
				printf("%u", line->node[0]);
			}
			else{
				printf("\t%u", line->node[j]);
			}
		}
	}
	printf("\n");
}
