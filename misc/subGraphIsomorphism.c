#include <stdlib.h>
#include <stdio.h>

#include "subGraphIsomorphism.h"

static struct nodeTab* graphIso_create_node_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
static struct labelTab* graphIso_create_label_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
static struct connectivityTab* graphIso_create_connectivity_tab(struct graph* graph);
#endif
static struct labelFastAccess* graphIso_create_label_fast(struct graph* graph, struct labelTab* label_tab, uint32_t* nb_label);

int32_t compare_labelTabItem_label(const void* arg1, const void* arg2);
int32_t compare_labelTabItem_connectivity(const void* arg1, const void* arg2);
int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2);
#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
int32_t compare_connectivityTabItem(const void* arg1, const void* arg2);
#endif

struct possibleAssignmentHeader{
	uint32_t 	nb_possible_assignment;
	uint32_t 	node_offset;
};

struct possibleAssignment{
	uint32_t 								nb_node;
	struct possibleAssignmentHeader* 		headers;
	struct node** 							nodes;
};

static struct possibleAssignment* possibleAssignment_create(uint32_t nb_node, uint32_t nb_assignment);
static struct possibleAssignment* possibleAssignment_create_init_first(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error);
static struct possibleAssignment* possibleAssignment_duplicate(struct possibleAssignment* possible_assignment, uint32_t new_assignment_index, struct node* new_assignment_value, uint8_t* error);
static void possibleAssignment_update(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#if SUBGRAPHISOMORPHISM_OPTIM_FAST_ERROR == 1
static uint32_t possibleAssignment_is_empty(struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#endif

#define possibleAssignment_delete(possible_assignment) free(possible_assignment);

static uint32_t graphIso_recursive_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct node** assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array);

/* ===================================================================== */
/* Handle routines 														 */
/* ===================================================================== */

static struct nodeTab* graphIso_create_node_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct node* 		node_cursor;
	uint32_t 			i;
	struct nodeTab* 	node_tab;

	node_tab = (struct nodeTab*)malloc(sizeof(struct nodeTab) * graph->nb_node);
	if (node_tab == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for(node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
			node_tab[i].label 			= node_get_label(node_cursor);
			node_tab[i].connectivity 	= node_cursor->nb_edge_src + node_cursor->nb_edge_dst;
			node_tab[i].node 			= node_cursor;
			node_cursor->ptr = node_tab + i;
		}
	}

	return node_tab;
}

static struct labelTab* graphIso_create_label_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct node* 		node_cursor;
	uint32_t 			i;
	struct labelTab* 	label_tab;

	label_tab = (struct labelTab*)malloc(sizeof(struct labelTab) * graph->nb_node);
	if (label_tab == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for(node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
			label_tab[i].label 	= node_get_label(node_cursor);
			label_tab[i].node 	= node_cursor;
		}
	}

	return label_tab;
}

#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
static struct connectivityTab* graphIso_create_connectivity_tab(struct graph* graph){
	struct node* 			node_cursor;
	uint32_t 				i;
	struct connectivityTab* connectivity_tab;

	connectivity_tab = (struct connectivityTab*)malloc(sizeof(struct connectivityTab) * graph->nb_node);
	if (connectivity_tab == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for(node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
			connectivity_tab[i].connectivity 	= node_cursor->nb_edge_src + node_cursor->nb_edge_dst;
			connectivity_tab[i].node 			= node_cursor;
		}
	}
	qsort(connectivity_tab, graph->nb_node, sizeof(struct connectivityTab), compare_connectivityTabItem);

	return connectivity_tab;
}
#endif

static struct labelFastAccess* graphIso_create_label_fast(struct graph* graph, struct labelTab* label_tab, uint32_t* nb_label){
	uint32_t 					i;
	uint32_t 					prev_label_value;
	uint32_t 					prev_label_index;
	struct labelFastAccess* 	label_fast;

	qsort(label_tab, graph->nb_node, sizeof(struct labelTab), compare_labelTabItem_label);

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
					qsort(label_tab + label_fast[prev_label_index].offset, label_fast[prev_label_index].size, sizeof(struct labelTab), compare_labelTabItem_connectivity);

					prev_label_index  ++;
					label_fast[prev_label_index].label 		= label_tab[i].label;
					label_fast[prev_label_index].offset 	= i;
					label_fast[prev_label_index].size 		= 1;
				}
			}
		}
	}

	return label_fast;
}

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	struct labelTab* 			label_tab;
	struct labelFastAccess* 	label_fast;
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	struct connectivityTab* 	connectivity_tab;
	#endif
	uint32_t 					nb_label;
	struct graphIsoHandle* 		handle;

	handle = (struct graphIsoHandle*)malloc(sizeof(struct graphIsoHandle));
	if (handle != NULL){
		label_tab = graphIso_create_label_tab(graph, node_get_label);
		#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
		connectivity_tab = graphIso_create_connectivity_tab(graph);
		if (label_tab != NULL && connectivity_tab != NULL){
		#else
		if (label_tab != NULL){
		#endif
			label_fast = graphIso_create_label_fast(graph, label_tab, &nb_label);
			if (label_fast != NULL){
				handle->graph 				= graph;
				handle->nb_label 			= nb_label;
				handle->label_fast 			= label_fast;
				handle->label_tab 			= label_tab;
				#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
				handle->connectivity_tab 	= connectivity_tab;
				#endif
				handle->edge_get_label 		= edge_get_label;
			}
			else{
				printf("ERROR: in %s, unable to create labelFastAccess\n", __func__);
				free(label_tab);
				#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
				free(connectivity_tab);
				#endif
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
	struct possibleAssignment* 	possible_assignment;
	struct node** 				assignment;
	struct array*				assignment_array = NULL;
	uint8_t 					error;

	if (sub_graph_handle->graph->nb_node > 0){
		assignment_array = array_create(sizeof(struct node*) * sub_graph_handle->graph->nb_node);
		if (assignment_array == NULL){
			printf("ERROR: in %s, unable to create array\n", __func__);
		}
		else{
			possible_assignment = possibleAssignment_create_init_first(graph_handle, sub_graph_handle, &error);
			if (possible_assignment == NULL){
				if (error){
					printf("ERROR: in %s, unable to create first possible assignment\n", __func__);
				}
			}
			else{
				assignment = (struct node**)alloca(sizeof(struct node*) * sub_graph_handle->graph->nb_node);
				graphIso_recursive_search(graph_handle, sub_graph_handle, assignment, 0, possible_assignment, assignment_array);
				possibleAssignment_delete(possible_assignment);
			}
		}
	}

	return assignment_array;
}

static uint32_t graphIso_recursive_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct node** assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array){
	uint32_t 		i;
	uint32_t 		result = 0;
	uint32_t 		local_result;
	uint8_t 		error;

	possibleAssignment_update(graph_handle, sub_graph_handle, possible_assignment, nb_assignment);

	#if SUBGRAPHISOMORPHISM_OPTIM_FAST_ERROR == 1
	if (possibleAssignment_is_empty(possible_assignment, nb_assignment)){
		return 0;
	}
	#endif

	if (nb_assignment == sub_graph_handle->graph->nb_node){
		if (array_add(assignment_array, assignment) < 0){
			printf("ERROR: in %s, unable to add assignment to array\n", __func__);
		}
		return 1;
	}

	for (i = 0; i < possible_assignment->headers[nb_assignment].nb_possible_assignment; i++){
		if (possible_assignment->nodes[possible_assignment->headers[nb_assignment].node_offset + i] != NULL){
			struct possibleAssignment* new_possible_assignment;

			new_possible_assignment = possibleAssignment_duplicate(possible_assignment, nb_assignment, possible_assignment->nodes[possible_assignment->headers[nb_assignment].node_offset + i], &error);
			if (new_possible_assignment != NULL){
				assignment[nb_assignment] = possible_assignment->nodes[possible_assignment->headers[nb_assignment].node_offset + i];
				local_result = graphIso_recursive_search(graph_handle, sub_graph_handle, assignment, nb_assignment + 1, new_possible_assignment, assignment_array);
				possibleAssignment_delete(new_possible_assignment);

				if (local_result > 0){
					result += local_result;
				}
				else{
					possible_assignment->nodes[possible_assignment->headers[nb_assignment].node_offset + i] = NULL;
					possibleAssignment_update(graph_handle, sub_graph_handle, possible_assignment, nb_assignment);
				}
			}
			else if (error){
				printf("ERROR: in %s, unable to duplicate possible assignment\n", __func__);
			}
		}
	}

	return result;
}

struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	struct nodeTab* 			node_tab;
	struct subGraphIsoHandle* 	handle;

	handle = (struct subGraphIsoHandle*)malloc(sizeof(struct subGraphIsoHandle));
	if (handle != NULL){
		node_tab = graphIso_create_node_tab(graph, node_get_label);
		if (node_tab != NULL){
			handle->graph 			= graph;
			handle->node_tab 		= node_tab;
			handle->edge_get_label 	= edge_get_label;
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
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	free(handle->connectivity_tab);
	#endif
	free(handle);
}

void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle){
	free(handle->node_tab);
	free(handle);
}

/* ===================================================================== */
/* Possible assignment routines 										 */
/* ===================================================================== */

static struct possibleAssignment* possibleAssignment_create(uint32_t nb_node, uint32_t nb_assignment){
	struct possibleAssignment* possible_assignment;

	possible_assignment = (struct possibleAssignment*)malloc(sizeof(struct possibleAssignment) + nb_node * sizeof(struct possibleAssignmentHeader) + nb_assignment * sizeof(struct node*));
	if (possible_assignment != NULL){
		possible_assignment->nb_node 	= nb_node;
		possible_assignment->headers 	= (struct possibleAssignmentHeader*)((char*)possible_assignment + sizeof(struct possibleAssignment));
		possible_assignment->nodes 		= (struct node**)((char*)possible_assignment->headers + nb_node * sizeof(struct possibleAssignmentHeader));
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return possible_assignment;
}

struct graphNodeList{
	uint32_t offset;
	uint32_t size;
};

static struct possibleAssignment* possibleAssignment_create_init_first(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error){
	uint32_t 					nb_assignment = 0;
	uint32_t 					i;
	uint32_t 					j;
	struct labelFastAccess* 	label_fast_access;
	struct possibleAssignment* possible_assignment = NULL;
	struct graphNodeList* 		graph_node_list;

	if (error != NULL){
		*error = 0;
	}

	graph_node_list = (struct graphNodeList*)malloc(sizeof(struct graphNodeList) * sub_graph_handle->graph->nb_node);
	if (graph_node_list == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	for	(i = 0; i < sub_graph_handle->graph->nb_node; i++){
		if (sub_graph_handle->node_tab[i].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
			label_fast_access = (struct labelFastAccess*)bsearch(&(sub_graph_handle->node_tab[i].label), graph_handle->label_fast, graph_handle->nb_label, sizeof(struct labelFastAccess), compare_key_labelFastAccess);
			if (label_fast_access == NULL){
				goto exit;
			}

			for (j = 0; j < label_fast_access->size; j++){
				if (graph_handle->label_tab[label_fast_access->offset + j].node->nb_edge_src + graph_handle->label_tab[label_fast_access->offset + j].node->nb_edge_dst >= sub_graph_handle->node_tab[i].connectivity){
					break;
				} 
			}

			nb_assignment += label_fast_access->size - j;
			graph_node_list[i].offset = label_fast_access->offset + j;
			graph_node_list[i].size = label_fast_access->size - j;
		}
		else{
			#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
			for (j = 0; j < graph_handle->graph->nb_node; j++){
				if (graph_handle->connectivity_tab[j].connectivity >= sub_graph_handle->node_tab[i].connectivity){
					break;
				}
			}

			nb_assignment += graph_handle->graph->nb_node - j;
			graph_node_list[i].offset = j;
			graph_node_list[i].size = graph_handle->graph->nb_node - j;
			#else
			nb_assignment += graph_handle->graph->nb_node;
			graph_node_list[i].offset = 0;
			graph_node_list[i].size = graph_handle->graph->nb_node;
			#endif
		}
	}

	possible_assignment = possibleAssignment_create(sub_graph_handle->graph->nb_node, nb_assignment);
	if (possible_assignment != NULL){
		possible_assignment->headers[0].node_offset = 0;

		for	(i = 0; i < sub_graph_handle->graph->nb_node; i++){
			possible_assignment->headers[i].nb_possible_assignment = graph_node_list[i].size;
			if (i != sub_graph_handle->graph->nb_node - 1){
				possible_assignment->headers[i + 1].node_offset = possible_assignment->headers[i].node_offset + graph_node_list[i].size;
			}

			if (sub_graph_handle->node_tab[i].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
				for (j = 0; j < graph_node_list[i].size; j++){
					possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = graph_handle->label_tab[graph_node_list[i].offset + j].node;
				}
			}
			else{
				#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
				for (j = 0; j < graph_node_list[i].size; j++){
					possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = graph_handle->connectivity_tab[graph_node_list[i].offset + j].node;
				}
				#else
				for (j = 0; j < graph_node_list[i].size; j++){
					possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = graph_handle->label_tab[j].node;
				}
				#endif
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to create possibleAssignment structure\n", __func__);
		if (error != NULL){
			*error = 1;
		}
	}

	exit:
	free(graph_node_list);

	return possible_assignment;
}

static struct possibleAssignment* possibleAssignment_duplicate(struct possibleAssignment* possible_assignment, uint32_t new_assignment_index, struct node* new_assignment_value, uint8_t* error){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					local_nb_assigment;
	uint32_t 					nb_assignment = 0;
	struct possibleAssignment* 	new_possible_assignment;

	if (error != NULL){
		*error = 0;
	}

	for (i = 0; i < possible_assignment->nb_node; i++){
		if (i != new_assignment_index){
			for (j = 0, local_nb_assigment = 0; j < possible_assignment->headers[i].nb_possible_assignment; j++){
				if (possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] != NULL && possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] != new_assignment_value){
					local_nb_assigment ++;
				}
			}
			#if SUBGRAPHISOMORPHISM_OPTIM_FAST_ERROR == 1
			if (local_nb_assigment > 0){
				nb_assignment += local_nb_assigment;
			}
			else{
				return NULL;
			}
			#else
			nb_assignment += local_nb_assigment;
			#endif
		}
		else{
			nb_assignment ++;
		}
	}

	new_possible_assignment = possibleAssignment_create(possible_assignment->nb_node, nb_assignment);
	if (new_possible_assignment != NULL){
		new_possible_assignment->headers[0].node_offset = 0;

		for (i = 0; i < possible_assignment->nb_node; i++){
			nb_assignment = 0;

			if (i != new_assignment_index){
				for (j = 0; j < possible_assignment->headers[i].nb_possible_assignment; j++){
					if (possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] != NULL && possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] != new_assignment_value){
						new_possible_assignment->nodes[new_possible_assignment->headers[i].node_offset + nb_assignment] = possible_assignment->nodes[possible_assignment->headers[i].node_offset + j];
						nb_assignment ++;
					}
				}
			}
			else{
				new_possible_assignment->nodes[new_possible_assignment->headers[i].node_offset + nb_assignment] = new_assignment_value;
				nb_assignment ++;
			}

			new_possible_assignment->headers[i].nb_possible_assignment = nb_assignment;
			if (i != possible_assignment->nb_node - 1){
				new_possible_assignment->headers[i + 1].node_offset = new_possible_assignment->headers[i].node_offset + nb_assignment;
			}
		}
	}
	else{
		if (error != NULL){
			*error = 1;
		}
	}

	return new_possible_assignment;
}

static void possibleAssignment_update(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint8_t 		match;
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	uint32_t 		l;
	struct edge* 	sub_graph_edge;
	struct edge* 	graph_edge;
	uint8_t 		restart = 1;

	while(restart){
		restart = 0;

		for (i = nb_assignment; i < sub_graph_handle->graph->nb_node; i++){
			for (j = 0; j < possible_assignment->headers[i].nb_possible_assignment; j++){
				if (possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] != NULL){

					sub_graph_edge = node_get_head_edge_src(sub_graph_handle->node_tab[i].node);
					while(sub_graph_edge != NULL){
						k = (struct nodeTab*)edge_get_dst(sub_graph_edge)->ptr - sub_graph_handle->node_tab;

						for (l = 0, match = 0; l < possible_assignment->headers[k].nb_possible_assignment && !match; l++){
							if (possible_assignment->nodes[possible_assignment->headers[k].node_offset + l] != NULL){
								graph_edge = node_get_head_edge_src(possible_assignment->nodes[possible_assignment->headers[i].node_offset + j]);
								
								while(graph_edge != NULL){
									if (possible_assignment->nodes[possible_assignment->headers[k].node_offset + l] == edge_get_dst(graph_edge)){
										if (sub_graph_handle->edge_get_label(sub_graph_edge) == graph_handle->edge_get_label(graph_edge)){
											match = 1;
											break;
										}
									}

									graph_edge = edge_get_next_src(graph_edge);
								}
							}
						}
						if (!match){
							possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = NULL;
							restart = 1;
							goto next;
						}

						sub_graph_edge = edge_get_next_src(sub_graph_edge);
					}

					sub_graph_edge = node_get_head_edge_dst(sub_graph_handle->node_tab[i].node);
					while(sub_graph_edge != NULL){
						k = (struct nodeTab*)edge_get_src(sub_graph_edge)->ptr - sub_graph_handle->node_tab;

						for (l = 0, match = 0; l < possible_assignment->headers[k].nb_possible_assignment && !match; l++){
							if (possible_assignment->nodes[possible_assignment->headers[k].node_offset + l] != NULL){
								graph_edge = node_get_head_edge_dst(possible_assignment->nodes[possible_assignment->headers[i].node_offset + j]);
								
								while(graph_edge != NULL){
									if (possible_assignment->nodes[possible_assignment->headers[k].node_offset + l] == edge_get_src(graph_edge)){
										if (sub_graph_handle->edge_get_label(sub_graph_edge) == graph_handle->edge_get_label(graph_edge)){
											match = 1;
											break;
										}
									}

									graph_edge = edge_get_next_dst(graph_edge);
								}
							}
						}
						if (!match){
							possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = NULL;
							restart = 1;
							goto next;
						}

						sub_graph_edge = edge_get_next_dst(sub_graph_edge);
					}
				}
				next:;
			}
		}
	}
}

#if SUBGRAPHISOMORPHISM_OPTIM_FAST_ERROR == 1
static uint32_t possibleAssignment_is_empty(struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint32_t i;

	for(i = nb_assignment; i < possible_assignment->nb_node; i++){
		if (possible_assignment->headers[i].nb_possible_assignment == 0){
			return 1;
		}
	}

	return 0;
}
#endif

/* ===================================================================== */
/* Compare routines 													 */
/* ===================================================================== */

int32_t compare_labelTabItem_label(const void* arg1, const void* arg2){
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

int32_t compare_labelTabItem_connectivity(const void* arg1, const void* arg2){
	struct labelTab* label_item1 = (struct labelTab*)arg1;
	struct labelTab* label_item2 = (struct labelTab*)arg2;

	uint32_t connectivity1 = label_item1->node->nb_edge_src + label_item1->node->nb_edge_dst;
	uint32_t connectivity2 = label_item2->node->nb_edge_src + label_item2->node->nb_edge_dst;

	if (connectivity1 < connectivity2){
		return -1;
	}
	else if (connectivity1 > connectivity2){
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

#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
int32_t compare_connectivityTabItem(const void* arg1, const void* arg2){
	struct connectivityTab* connectivity_item1 = (struct connectivityTab*)arg1;
	struct connectivityTab* connectivity_item2 = (struct connectivityTab*)arg2;

	if (connectivity_item1->connectivity < connectivity_item2->connectivity){
		return -1;
	}
	else if (connectivity_item1->connectivity > connectivity_item2->connectivity){
		return 1;
	}
	else{
		return 0;
	}
}
#endif