#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "graphMining.h"

int32_t compare_labelTabItem(const void* arg1, const void* arg2);
int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2);
int32_t compare_candidatKPath(const void* arg1, const void* arg2);

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
	graph_mining->nb_label 			= 0;
	graph_mining->label_tab 			= NULL;
	graph_mining->label_fast 			= NULL;

	if (array_init(&(graph_mining->frequent_path_array), sizeof(struct frequentKPath))){
		printf("ERROR: in %s, unable to init array\n", __func__);
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

	if (graphMining_search_frequent_path(graph_mining, 1)){
		printf("ERROR: in %s, unable to search frequent path\n", __func__);
		goto exit;
	}

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
