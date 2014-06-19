#include <stdlib.h>
#include <stdio.h>

#include "subGraphIsomorphism.h"

static struct labelTab* graphIso_create_label_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
static struct labelFastAccess* graphIso_create_label_fast(struct graph* graph, struct labelTab* label_tab, uint32_t* nb_label);

int32_t compare_labelTabItem(const void* arg1, const void* arg2);
int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2);

struct possibleAssignementHeader{
	uint32_t 	nb_possible_assignement;
	uint32_t 	node_offset;
};

struct possibleAssignement{
	uint32_t 								nb_node;
	struct possibleAssignementHeader* 		headers;
	struct node** 							nodes;
};

static struct possibleAssignement* possibleAssignement_create(uint32_t nb_node, uint32_t nb_assignement);
static struct possibleAssignement* possibleAssignement_create_init_first(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle);
static struct possibleAssignement* possibleAssignement_duplicate(struct possibleAssignement* possible_assignement, uint32_t new_assignement_index, struct node* new_assignement_value);
static void possibleAssignement_update(struct possibleAssignement* possible_assignement, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignement);

#define possibleAssignement_delete(possible_assignement) free(possible_assignement);

static uint32_t graphIso_recursive_search(struct subGraphIsoHandle* sub_graph_handle, struct node** assignement, uint32_t nb_assignement, struct possibleAssignement* possible_assignement, struct array* assignement_array);

/* ===================================================================== */
/* Handle routines 														 */
/* ===================================================================== */

static struct labelTab* graphIso_create_label_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct node* 		node_cursor;
	uint32_t 			i;
	struct labelTab* 	label_tab;

	label_tab = (struct labelTab*)malloc(sizeof(struct labelTab) * graph->nb_node);
	if (label_tab == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		node_cursor = graph_get_head_node(graph);
		i = 0;
		while(node_cursor != NULL){
			label_tab[i].label = node_get_label(node_cursor);
			label_tab[i].node = node_cursor;
			node_cursor->ptr = label_tab + i;

			node_cursor = node_get_next(node_cursor);
			i++;
		}
	}

	return label_tab;
}

static struct labelFastAccess* graphIso_create_label_fast(struct graph* graph, struct labelTab* label_tab, uint32_t* nb_label){
	uint32_t 					i;
	uint32_t 					prev_label_value;
	uint32_t 					prev_label_index;
	struct labelFastAccess* 	label_fast;

	qsort(label_tab, graph->nb_node, sizeof(struct labelTab), compare_labelTabItem);

	for (i = 1, *nb_label = 1, prev_label_value = label_tab[0].label; i < graph->nb_node; i++){
		if (label_tab[i].label != prev_label_value){
			*nb_label += 1;
			prev_label_value = label_tab[i].label;
		}
	}

	label_fast = (struct labelFastAccess*)malloc(sizeof(struct labelFastAccess) * (*nb_label));
	if (label_fast == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for (i = 0; i < graph->nb_node; i++){
			if (i == 0){
				label_fast[0].label 	= label_tab[0].label;
				label_fast[0].offset 	= 0;
				label_fast[0].size 		= 1;
				prev_label_index = 0;
			}
			else{
				if (label_tab[i].label == label_fast[prev_label_index].label){
					label_fast[prev_label_index].size ++;
				}
				else{
					prev_label_index  ++;
					label_fast[prev_label_index].label 		= label_tab[i].label;
					label_fast[prev_label_index].offset 	= i;
					label_fast[prev_label_index].size 		= 1;
				}
			}
			label_tab[i].fastAccess_index = prev_label_index;
		}
	}

	return label_fast;
}

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct labelTab* 			label_tab;
	struct labelFastAccess* 	label_fast;
	uint32_t 					nb_label;
	struct graphIsoHandle* 		handle;

	handle = (struct graphIsoHandle*)malloc(sizeof(struct graphIsoHandle));
	if (handle != NULL){
		label_tab = graphIso_create_label_tab(graph, node_get_label);
		if (label_tab != NULL){
			label_fast = graphIso_create_label_fast(graph, label_tab, &nb_label);
			if (label_fast != NULL){
				handle->graph 		= graph;
				handle->nb_label 	= nb_label;
				handle->label_fast 	= label_fast;
				handle->label_tab 	= label_tab;
			}
			else{
				printf("ERROR: in %s, unable to create labelFastAccess\n", __func__);
				free(label_tab);
				free(handle);
				handle = NULL;
			}
		}
		else{
			printf("ERROR: in %s, unable to create labelTab\n", __func__);
			free(handle);
			handle = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return handle;
}

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	struct possibleAssignement* possible_assignement;
	struct node** 				assignement;
	struct array*				assignement_array = NULL;

	if (sub_graph_handle->graph->nb_node > 0){
		assignement_array = array_create(sizeof(struct node*) * sub_graph_handle->graph->nb_node);
		if (assignement_array == NULL){
			printf("ERROR: in %s, unable to create array\n", __func__);
		}
		else{
			possible_assignement = possibleAssignement_create_init_first(graph_handle, sub_graph_handle);
			if (possible_assignement == NULL){
				printf("ERROR: in %s, unable to create first possible assignement\n", __func__);
			}
			else{
				assignement = (struct node**)alloca(sizeof(struct node*) * sub_graph_handle->graph->nb_node);
				graphIso_recursive_search(sub_graph_handle, assignement, 0, possible_assignement, assignement_array);
				possibleAssignement_delete(possible_assignement);
			}
		}
	}

	return assignement_array;
}

static uint32_t graphIso_recursive_search(struct subGraphIsoHandle* sub_graph_handle, struct node** assignement, uint32_t nb_assignement, struct possibleAssignement* possible_assignement, struct array* assignement_array){
	uint32_t 		i;
	uint32_t 		result = 0;
	uint32_t 		local_result;

	possibleAssignement_update(possible_assignement, sub_graph_handle, nb_assignement);

	if (nb_assignement == sub_graph_handle->graph->nb_node){
		if (array_add(assignement_array, assignement) < 0){
			printf("ERROR: in %s, unable to add assignement to array\n", __func__);
		}
		return 1;
	}

	for (i = 0; i < possible_assignement->headers[nb_assignement].nb_possible_assignement; i++){
		if (possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i] != NULL){
			struct possibleAssignement* new_possible_assignement;

			new_possible_assignement = possibleAssignement_duplicate(possible_assignement, nb_assignement, possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i]);
			if (new_possible_assignement != NULL){
				assignement[nb_assignement] = possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i];
				local_result = graphIso_recursive_search(sub_graph_handle, assignement, nb_assignement + 1, new_possible_assignement, assignement_array);
				possibleAssignement_delete(new_possible_assignement);

				if (local_result > 0){
					result += local_result;
				}
				else{
					possible_assignement->nodes[possible_assignement->headers[nb_assignement].node_offset + i] = NULL;
					possibleAssignement_update(possible_assignement, sub_graph_handle, nb_assignement);
				}
			}
			else{
				printf("ERROR: in %s, unable to duplicate possible assignement\n", __func__);
			}
		}
	}

	return result;
}

struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct labelTab* 			label_tab;
	struct subGraphIsoHandle* 	handle;

	handle = (struct subGraphIsoHandle*)malloc(sizeof(struct subGraphIsoHandle));
	if (handle != NULL){
		label_tab = graphIso_create_label_tab(graph, node_get_label);
		if (label_tab != NULL){
			handle->graph 		= graph;
			handle->label_tab 	= label_tab;
		}
		else{
			printf("ERROR: in %s, unable to create labelTab\n", __func__);
			free(handle);
			handle = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return handle;
}

void graphIso_delete_graph_handle(struct graphIsoHandle* handle){
	free(handle->label_fast);
	free(handle->label_tab);
	free(handle);
}

void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle){
	free(handle->label_tab);
	free(handle);
}

/* ===================================================================== */
/* Possible assignement routines 										 */
/* ===================================================================== */

static struct possibleAssignement* possibleAssignement_create(uint32_t nb_node, uint32_t nb_assignement){
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

static struct possibleAssignement* possibleAssignement_create_init_first(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	uint32_t 					nb_assignement = 0;
	uint32_t 					i;
	uint32_t 					j;
	struct labelFastAccess* 	label_fast_access;
	struct possibleAssignement* possible_assignement;

	for	(i = 0; i < sub_graph_handle->graph->nb_node; i++){
		label_fast_access = (struct labelFastAccess*)bsearch(&(sub_graph_handle->label_tab[i].label), graph_handle->label_fast, graph_handle->nb_label, sizeof(struct labelFastAccess), compare_key_labelFastAccess);
		if (label_fast_access == NULL){
			return NULL;
		}
		nb_assignement += label_fast_access->size;
	}

	possible_assignement = possibleAssignement_create(sub_graph_handle->graph->nb_node, nb_assignement);
	if (possible_assignement != NULL){
		possible_assignement->headers[0].node_offset = 0;

		for	(i = 0; i < sub_graph_handle->graph->nb_node; i++){
			label_fast_access = (struct labelFastAccess*)bsearch (&(sub_graph_handle->label_tab[i].label), graph_handle->label_fast, graph_handle->nb_label, sizeof(struct labelFastAccess), compare_key_labelFastAccess);
			possible_assignement->headers[i].nb_possible_assignement = label_fast_access->size;
			if (i != sub_graph_handle->graph->nb_node - 1){
				possible_assignement->headers[i + 1].node_offset = possible_assignement->headers[i].node_offset + label_fast_access->size;
			}

			for (j = 0; j < label_fast_access->size; j++){
				possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] = graph_handle->label_tab[label_fast_access->offset + j].node;
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to create possibleAssignement structure\n", __func__);
	}

	return possible_assignement;
}

static struct possibleAssignement* possibleAssignement_duplicate(struct possibleAssignement* possible_assignement, uint32_t new_assignement_index, struct node* new_assignement_value){
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

static void possibleAssignement_update(struct possibleAssignement* possible_assignement, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignement){
	uint8_t 		match;
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		l;
	struct edge* 	sub_graph_edge;
	struct edge* 	graph_edge;

	start:
	for (i = nb_assignement; i < sub_graph_handle->graph->nb_node; i++){
		for (j = 0; j < possible_assignement->headers[i].nb_possible_assignement; j++){
			if (possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] != NULL){

				sub_graph_edge = node_get_head_edge_src(sub_graph_handle->label_tab[i].node);
				while(sub_graph_edge != NULL){
					k = (struct labelTab*)edge_get_dst(sub_graph_edge)->ptr - sub_graph_handle->label_tab;

					for (l = 0, match = 0; l < possible_assignement->headers[k].nb_possible_assignement && !match; l++){
						if (possible_assignement->nodes[possible_assignement->headers[k].node_offset + l] != NULL){
							graph_edge = node_get_head_edge_src(possible_assignement->nodes[possible_assignement->headers[i].node_offset + j]);
							
							while(graph_edge != NULL){
								if (possible_assignement->nodes[possible_assignement->headers[k].node_offset + l] == edge_get_dst(graph_edge)){
									match = 1;
									break;
								}

								graph_edge = edge_get_next_src(graph_edge);
							}
						}
					}
					if (!match){
						possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] = NULL;
						goto start;
					}

					sub_graph_edge = edge_get_next_src(sub_graph_edge);
				}

				sub_graph_edge = node_get_head_edge_dst(sub_graph_handle->label_tab[i].node);
				while(sub_graph_edge != NULL){
					k = (struct labelTab*)edge_get_src(sub_graph_edge)->ptr - sub_graph_handle->label_tab;

					for (l = 0, match = 0; l < possible_assignement->headers[k].nb_possible_assignement && !match; l++){
						if (possible_assignement->nodes[possible_assignement->headers[k].node_offset + l] != NULL){
							graph_edge = node_get_head_edge_dst(possible_assignement->nodes[possible_assignement->headers[i].node_offset + j]);
							
							while(graph_edge != NULL){
								if (possible_assignement->nodes[possible_assignement->headers[k].node_offset + l] == edge_get_src(graph_edge)){
									match = 1;
									break;
								}

								graph_edge = edge_get_next_dst(graph_edge);
							}
						}
					}
					if (!match){
						possible_assignement->nodes[possible_assignement->headers[i].node_offset + j] = NULL;
						goto start;
					}

					sub_graph_edge = edge_get_next_dst(sub_graph_edge);
				}
			}
		}
	}
}

/* ===================================================================== */
/* Compare routines 													 */
/* ===================================================================== */

int32_t compare_labelTabItem(const void* arg1, const void* arg2){
	struct labelTab* label_item1 = (struct labelTab*)arg1;
	struct labelTab* label_item2 = (struct labelTab*)arg2;

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