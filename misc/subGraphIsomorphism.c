#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "subGraphIsomorphism.h"

#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
#include "dijkstra.h"
#endif

static struct nodeTab* graphIso_create_node_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
static struct labelTab* graphIso_create_label_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
static struct connectivityTab* graphIso_create_connectivity_tab(struct graph* graph);
#endif
static struct labelFastAccess* graphIso_create_label_fast(struct graph* graph, struct labelTab* label_tab, uint32_t* nb_label);
static struct edgeTab* graphIso_create_edge_tab(struct graph* graph, uint32_t(*edge_get_label)(struct edge*), struct nodeTab* node_tab);

int32_t compare_labelTabItem_label(const void* arg1, const void* arg2);
int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2);
#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
int32_t compare_labelTabItem_connectivity(const void* arg1, const void* arg2);
int32_t compare_connectivityTabItem(const void* arg1, const void* arg2);
#endif

struct possibleAssignmentHeader{
	uint32_t 	nb_possible_assignment;
	uint32_t 	node_offset;
	uint8_t 	assigned;
};

struct possibleAssignment{
	uint32_t 							nb_node;
	struct possibleAssignmentHeader* 	headers;
	struct node** 						nodes;
	uint32_t*							stacked_size;
};

static struct possibleAssignment* possibleAssignment_create(uint32_t nb_node, uint32_t nb_assignment);
static struct possibleAssignment* possibleAssignment_create_init_first(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error);
#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
static struct possibleAssignment* possibleAssignment_duplicate(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, struct node* new_assignment_value, uint8_t* error);
#elif SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static struct possibleAssignment* possibleAssignment_duplicate(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, struct node* new_assignment_value, uint8_t* error);
#else
static struct possibleAssignment* possibleAssignment_duplicate(struct possibleAssignment* possible_assignment, uint32_t nb_assignment, struct node* new_assignment_value, uint8_t* error);
#endif
static int32_t possibleAssignment_update(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment);
#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static void possibleAssignment_save_state(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#else
static void possibleAssignment_save_state(struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#endif

#define possibleAssignment_delete(possible_assignment) free(possible_assignment);

static uint32_t graphIso_recursive_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct node** assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array);

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
#define get_node_order(sub_graph_handle, index) ((sub_graph_handle)->node_order[index])

static void graphIso_choose_next_node(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t* index_);

#else
#define get_node_order(sub_graph_handle, index) (index)
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
#define get_node_connectivity(sub_graph_handle, index) ((sub_graph_handle)->node_tab[get_node_order(sub_graph_handle, index)].connectivity)
#else
#define get_node_connectivity(sub_graph_handle, index) ((sub_graph_handle)->node_tab[get_node_order(sub_graph_handle, index)].node->nb_edge_src + (sub_graph_handle)->node_tab[get_node_order(sub_graph_handle, index)].node->nb_edge_dst)
#endif

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
			#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
			node_tab[i].connectivity 	= node_cursor->nb_edge_src + node_cursor->nb_edge_dst;
			#endif
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
					#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
					qsort(label_tab + label_fast[prev_label_index].offset, label_fast[prev_label_index].size, sizeof(struct labelTab), compare_labelTabItem_connectivity);
					#endif

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

static struct edgeTab* graphIso_create_edge_tab(struct graph* graph, uint32_t(*edge_get_label)(struct edge*), struct nodeTab* node_tab){
	struct edgeTab* edge_tab;
	uint32_t 		i = 0;
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;

	edge_tab = (struct edgeTab*)malloc(sizeof(struct edgeTab) * graph->nb_edge);
	if (edge_tab == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				edge_tab[i].label 	= edge_get_label(edge_cursor);
				edge_tab[i].src 	= (struct nodeTab*)(node_cursor->ptr) - node_tab;
				edge_tab[i].dst 	= (struct nodeTab*)(edge_get_dst(edge_cursor)->ptr) - node_tab;
				i ++;
			}
		}
	}

	return edge_tab;
}

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	struct labelTab* 			label_tab;
	struct labelFastAccess* 	label_fast;
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	struct connectivityTab* 	connectivity_tab;
	#endif
	uint32_t 					nb_label;
	struct graphIsoHandle* 		handle;
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	uint32_t 					i;
	struct node* 				node_cursor;
	#endif

	handle = (struct graphIsoHandle*)malloc(sizeof(struct graphIsoHandle));
	if (handle == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	handle->dst = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_node * graph->nb_node);
	if (handle->dst == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(handle);
		return NULL;
	}
			
	for (node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL && i < graph->nb_node; node_cursor = node_get_next(node_cursor), i++){
		if (dijkstra_dst(graph, node_cursor, handle->dst + (i * graph->nb_node))){
			printf("ERROR: in %s, unable to compute graph dst (Dijkstra)\n", __func__);
		}
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL && i < graph->nb_node; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = (void*)i;
	}
	#endif

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
		#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
		free(handle->dst);
		#endif
		free(handle);
		handle = NULL;
	}

	return handle;
}

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	struct possibleAssignment* 	possible_assignment;
	struct node** 				assignment;
	struct array*				assignment_array = NULL;
	uint8_t 					error;
	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	uint32_t 					i;
	#endif

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
				#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
				for (i = 0; i < sub_graph_handle->graph->nb_node; i++){
					sub_graph_handle->node_order[i] = i;
				}
				#endif

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
	uint8_t 		error;
	uint32_t 		local_nb_possible_assignment;
	uint32_t 		local_node_offset;
	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	uint32_t 		index;
	uint32_t 		tmp;
	#endif

	if (possibleAssignment_update(graph_handle, sub_graph_handle, possible_assignment)){
		return 0;
	}

	if (nb_assignment < sub_graph_handle->graph->nb_node - 1){
		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
		graphIso_choose_next_node(sub_graph_handle, possible_assignment, nb_assignment, &index);
		#endif

		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
		possibleAssignment_save_state(sub_graph_handle, possible_assignment, nb_assignment);
		#else
		possibleAssignment_save_state(possible_assignment, nb_assignment);
		#endif

		local_nb_possible_assignment = possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].nb_possible_assignment;
		local_node_offset = possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].node_offset;

		possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].assigned = 1;
		for (i = 0; i < local_nb_possible_assignment; i++){
			struct possibleAssignment* new_possible_assignment;

			possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].node_offset = local_node_offset + i;

			#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
			new_possible_assignment = possibleAssignment_duplicate(graph_handle, sub_graph_handle, possible_assignment, nb_assignment, possible_assignment->nodes[local_node_offset + i], &error);
			#elif SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
			new_possible_assignment = possibleAssignment_duplicate(sub_graph_handle, possible_assignment, nb_assignment, possible_assignment->nodes[local_node_offset + i], &error);
			#else
			new_possible_assignment = possibleAssignment_duplicate(possible_assignment, nb_assignment, possible_assignment->nodes[local_node_offset + i], &error);
			#endif
			if (new_possible_assignment != NULL){
				assignment[get_node_order(sub_graph_handle, nb_assignment)] = possible_assignment->nodes[local_node_offset + i];
				result += graphIso_recursive_search(graph_handle, sub_graph_handle, assignment, nb_assignment + 1, new_possible_assignment, assignment_array);

				possible_assignment->stacked_size[sub_graph_handle->graph->nb_node * nb_assignment + nb_assignment] --;
			}
			else if (error){
				printf("ERROR: in %s, unable to duplicate possible assignment\n", __func__);
			}
		}

		possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].node_offset = local_node_offset;
		possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].assigned = 0;

		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
		tmp = sub_graph_handle->node_order[nb_assignment];
		sub_graph_handle->node_order[nb_assignment] = sub_graph_handle->node_order[index];
		sub_graph_handle->node_order[index] = tmp;
		#endif
	}
	else{
		for (i = 0; i < possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].nb_possible_assignment; i++){
			if (possible_assignment->nodes[possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].node_offset + i] != NULL){

				assignment[get_node_order(sub_graph_handle, nb_assignment)] = possible_assignment->nodes[possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].node_offset + i];
				if (array_add(assignment_array, assignment) < 0){
					printf("ERROR: in %s, unable to add assignment to array\n", __func__);
				}
				result ++;
			}
		}
	}

	return result;
}

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static void graphIso_choose_next_node(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t* index_){
	uint32_t index;
	uint32_t i;
	uint32_t tmp;
	uint32_t min_nb_assignement;
	uint32_t max_connectivity;

	min_nb_assignement = possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].nb_possible_assignment;
	max_connectivity = get_node_connectivity(sub_graph_handle, nb_assignment);
	index = nb_assignment;

	for (i = nb_assignment + 1; i < sub_graph_handle->graph->nb_node; i++){
		if (possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment < min_nb_assignement){
			min_nb_assignement = possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment;
			max_connectivity = get_node_connectivity(sub_graph_handle, i);
			index = i;
		}
		else if (possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment == min_nb_assignement){
			if (max_connectivity < get_node_connectivity(sub_graph_handle, i)){
				min_nb_assignement = possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment;
				max_connectivity = get_node_connectivity(sub_graph_handle, i);
				index = i;
			}
		}
	}

	tmp = sub_graph_handle->node_order[nb_assignment];
	sub_graph_handle->node_order[nb_assignment] = sub_graph_handle->node_order[index];
	sub_graph_handle->node_order[index] = tmp;
	*index_ = index;
}
#endif

struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	struct subGraphIsoHandle* 	handle;
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	uint32_t 					i;
	struct node* 				node_cursor;
	#endif

	handle = (struct subGraphIsoHandle*)malloc(sizeof(struct subGraphIsoHandle));
	if (handle == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}
		
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	handle->dst = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_node * graph->nb_node);
	if (handle->dst == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(handle);
		return NULL;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL && i < graph->nb_node; node_cursor = node_get_next(node_cursor), i++){
		if (dijkstra_dst(graph, node_cursor, handle->dst + (i * graph->nb_node))){
			printf("ERROR: in %s, unable to compute graph dst (Dijkstra)\n", __func__);
		}
	}
	#endif

	handle->node_tab = graphIso_create_node_tab(graph, node_get_label);
	if (handle->node_tab == NULL){
		printf("ERROR: in %s, unable to create labelTab\n", __func__);
		#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
		free(handle->dst);
		#endif
		free(handle);
		return NULL;
	}
		
	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	handle->node_order = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_node);
	if (handle->node_order == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(handle->node_tab);
		#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
		free(handle->dst);
		#endif
		free(handle);
		return NULL;
	}
	#endif

	handle->edge_tab = graphIso_create_edge_tab(graph, edge_get_label, handle->node_tab);
	if (handle->edge_tab == NULL){
		printf("ERROR: in %s, unable to create edgeTab\n", __func__);
		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
		free(handle->node_order);
		#endif
		free(handle->node_tab);
		#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
		free(handle->dst);
		#endif
		free(handle);
		return NULL;
	}

	handle->graph = graph;
	
	return handle;
}

void graphIso_delete_graph_handle(struct graphIsoHandle* handle){
	free(handle->label_fast);
	free(handle->label_tab);
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	free(handle->connectivity_tab);
	#endif
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	free(handle->dst);
	#endif
	free(handle);
}

void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle){
	free(handle->edge_tab);
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	free(handle->dst);
	#endif
	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	free(handle->node_order);
	#endif
	free(handle->node_tab);
	free(handle);
}

/* ===================================================================== */
/* Possible assignment routines 										 */
/* ===================================================================== */

static struct possibleAssignment* possibleAssignment_create(uint32_t nb_node, uint32_t nb_assignment){
	struct possibleAssignment* possible_assignment;

	possible_assignment = (struct possibleAssignment*)malloc(sizeof(struct possibleAssignment) + nb_node * sizeof(struct possibleAssignmentHeader) + nb_assignment * sizeof(struct node*) + nb_node * nb_node * sizeof(uint32_t));
	if (possible_assignment != NULL){
		possible_assignment->nb_node 		= nb_node;
		possible_assignment->headers 		= (struct possibleAssignmentHeader*)((char*)possible_assignment + sizeof(struct possibleAssignment));
		possible_assignment->nodes 			= (struct node**)((char*)possible_assignment->headers + nb_node * sizeof(struct possibleAssignmentHeader));
		possible_assignment->stacked_size 	= (uint32_t*)((char*)possible_assignment->nodes + nb_assignment * sizeof(struct node*));
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

			#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
			for (j = 0; j < label_fast_access->size; j++){
				if (graph_handle->label_tab[label_fast_access->offset + j].node->nb_edge_src + graph_handle->label_tab[label_fast_access->offset + j].node->nb_edge_dst < sub_graph_handle->node_tab[i].connectivity){
					break;
				}
			}
			#else
			j = label_fast_access->size;
			#endif

			nb_assignment += j;
			graph_node_list[i].offset = label_fast_access->offset;
			graph_node_list[i].size = j;
		}
		else{
			#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
			for (j = 0; j < graph_handle->graph->nb_node; j++){
				if (graph_handle->connectivity_tab[j].connectivity < sub_graph_handle->node_tab[i].connectivity){
					break;
				}
			}
			#else
			j = graph_handle->graph->nb_node;
			#endif

			nb_assignment += j;
			graph_node_list[i].offset = 0;
			graph_node_list[i].size = j;
		}
	}

	possible_assignment = possibleAssignment_create(sub_graph_handle->graph->nb_node, nb_assignment);
	if (possible_assignment != NULL){
		possible_assignment->headers[0].node_offset = 0;

		for	(i = 0; i < sub_graph_handle->graph->nb_node; i++){
			possible_assignment->headers[i].nb_possible_assignment = graph_node_list[i].size;
			possible_assignment->headers[i].assigned = 0;

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
#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
static struct possibleAssignment* possibleAssignment_duplicate(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, struct node* new_assignment_value, uint8_t* error){
	uint32_t 					i;
	uint32_t 					j;
	struct node* 				tmp;
	uint32_t 					size;
	struct node** 				possible_assignment_local_ptr;
	struct possibleAssignment* 	new_possible_assignment = possible_assignment;

	*error = 0;

	possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].nb_possible_assignment = 1;
	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment = possible_assignment->stacked_size[nb_assignment * possible_assignment->nb_node + i];
	}

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		if (sub_graph_handle->dst[sub_graph_handle->graph->nb_node * get_node_order(sub_graph_handle, nb_assignment) + get_node_order(sub_graph_handle, i)] != DIJKSTRA_INVALID_DST){
			j = 0;
			size = possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment;
			possible_assignment_local_ptr = possible_assignment->nodes + possible_assignment->headers[get_node_order(sub_graph_handle, i)].node_offset;

			while(j < size){
				if (possible_assignment_local_ptr[j] != new_assignment_value){
					uint32_t index_k = (uint32_t)(new_assignment_value->ptr);
					uint32_t index_i = (uint32_t)(possible_assignment_local_ptr[j]->ptr);

					if (sub_graph_handle->dst[sub_graph_handle->graph->nb_node * get_node_order(sub_graph_handle, nb_assignment) + get_node_order(sub_graph_handle, i)] >= graph_handle->dst[graph_handle->graph->nb_node * index_k + index_i]){
						j ++;
					}
					else{
						size --;
						tmp = possible_assignment_local_ptr[size];
						possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
						possible_assignment_local_ptr[j] = tmp;
					}
				}
				else{
					size --;
					tmp = possible_assignment_local_ptr[size];
					possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
					possible_assignment_local_ptr[j] = tmp;
				}
			}
			possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment = size;
			if (!size){
				return NULL;
			}
		}
		else{
			printf("ERROR: in %s, this case is not supposed to happen, subgrah is not connected\n", __func__);
		}
	}

	return new_possible_assignment;
}
#else
#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static struct possibleAssignment* possibleAssignment_duplicate(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, struct node* new_assignment_value, uint8_t* error){
#else
static struct possibleAssignment* possibleAssignment_duplicate(struct possibleAssignment* possible_assignment, uint32_t nb_assignment, struct node* new_assignment_value, uint8_t* error){
#endif
	uint32_t 					i;
	uint32_t 					j;
	struct node* 				tmp;
	uint32_t 					size;
	struct node** 				possible_assignment_local_ptr;
	struct possibleAssignment* 	new_possible_assignment = possible_assignment;

	*error = 0;

	possible_assignment->headers[get_node_order(sub_graph_handle, nb_assignment)].nb_possible_assignment = 1;
	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment = possible_assignment->stacked_size[nb_assignment * possible_assignment->nb_node + i];
	}

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		j = 0;
		size = possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment;
		possible_assignment_local_ptr = possible_assignment->nodes + possible_assignment->headers[get_node_order(sub_graph_handle, i)].node_offset;

		while(j < size){
			if (possible_assignment_local_ptr[j] != new_assignment_value){
				j ++;
			}
			else{
				size --;
				tmp = possible_assignment_local_ptr[size];
				possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
				possible_assignment_local_ptr[j] = tmp;
			}
		}
		possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment = size;
		if (!size){
			return NULL;
		}
	}

	return new_possible_assignment;
}
#endif

static int32_t possibleAssignment_update(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment){
	uint8_t 		match;
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		l;
	uint8_t 		restart = 1;
	struct edge* 	edge;
	uint32_t 		src;
	uint32_t 		dst;

	while(restart){
		restart = 0;
		
		for (i = 0; i < sub_graph_handle->graph->nb_edge; i++){
			src = sub_graph_handle->edge_tab[i].src;
			dst = sub_graph_handle->edge_tab[i].dst;

			if (!possible_assignment->headers[src].assigned){
				for (j = 0; j < possible_assignment->headers[src].nb_possible_assignment;){
					for (edge = node_get_head_edge_src(possible_assignment->nodes[possible_assignment->headers[src].node_offset + j]), match = 0; (uint32_t)edge & (match + 0xffffffff); edge = edge_get_next_src(edge)){
						if (sub_graph_handle->edge_tab[i].label == graph_handle->edge_get_label(edge)){

							for (l = 0; l < possible_assignment->headers[dst].nb_possible_assignment; l++){
								if (possible_assignment->nodes[possible_assignment->headers[dst].node_offset + l] == edge_get_dst(edge)){
									match = 1;
									break;
								}
							}
						}
					}

					if (!match){
						struct node* tmp;

						tmp = possible_assignment->nodes[possible_assignment->headers[src].node_offset + possible_assignment->headers[src].nb_possible_assignment - 1];
						possible_assignment->nodes[possible_assignment->headers[src].node_offset + possible_assignment->headers[src].nb_possible_assignment - 1] = possible_assignment->nodes[possible_assignment->headers[src].node_offset + j];
						possible_assignment->nodes[possible_assignment->headers[src].node_offset + j] = tmp;

						possible_assignment->headers[src].nb_possible_assignment --;
						restart = 1;
					}

					j += match;
				}

				if (possible_assignment->headers[src].nb_possible_assignment == 0){
					return -1;
				}
			}
			if (!possible_assignment->headers[dst].assigned){
				for (j = 0; j < possible_assignment->headers[dst].nb_possible_assignment;){
					for (edge = node_get_head_edge_dst(possible_assignment->nodes[possible_assignment->headers[dst].node_offset + j]), match = 0; (uint32_t)edge & (match + 0xffffffff); edge = edge_get_next_dst(edge)){
						if (sub_graph_handle->edge_tab[i].label == graph_handle->edge_get_label(edge)){

							for (l = 0; l < possible_assignment->headers[src].nb_possible_assignment; l++){
								if (possible_assignment->nodes[possible_assignment->headers[src].node_offset + l] == edge_get_src(edge)){
									match = 1;
									break;
								}
							}
						}
					}

					if (!match){
						struct node* tmp;

						tmp = possible_assignment->nodes[possible_assignment->headers[dst].node_offset + possible_assignment->headers[dst].nb_possible_assignment - 1];
						possible_assignment->nodes[possible_assignment->headers[dst].node_offset + possible_assignment->headers[dst].nb_possible_assignment - 1] = possible_assignment->nodes[possible_assignment->headers[dst].node_offset + j];
						possible_assignment->nodes[possible_assignment->headers[dst].node_offset + j] = tmp;

						possible_assignment->headers[dst].nb_possible_assignment --;
						restart = 1;
					}
					
					j += match;
				}

				if (possible_assignment->headers[dst].nb_possible_assignment == 0){
					return -1;
				}
			}
		}
	}

	return 0;
}

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static void possibleAssignment_save_state(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
#else
static void possibleAssignment_save_state(struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
#endif
	uint32_t i;

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		possible_assignment->stacked_size[nb_assignment * possible_assignment->nb_node + i] = possible_assignment->headers[get_node_order(sub_graph_handle, i)].nb_possible_assignment;
	}
}


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
int32_t compare_labelTabItem_connectivity(const void* arg1, const void* arg2){
	struct labelTab* label_item1 = (struct labelTab*)arg1;
	struct labelTab* label_item2 = (struct labelTab*)arg2;

	uint32_t connectivity1 = label_item1->node->nb_edge_src + label_item1->node->nb_edge_dst;
	uint32_t connectivity2 = label_item2->node->nb_edge_src + label_item2->node->nb_edge_dst;

	if (connectivity1 > connectivity2){
		return -1;
	}
	else if (connectivity1 < connectivity2){
		return 1;
	}
	else{
		return 0;
	}
}

int32_t compare_connectivityTabItem(const void* arg1, const void* arg2){
	struct connectivityTab* connectivity_item1 = (struct connectivityTab*)arg1;
	struct connectivityTab* connectivity_item2 = (struct connectivityTab*)arg2;

	if (connectivity_item1->connectivity > connectivity_item2->connectivity){
		return -1;
	}
	else if (connectivity_item1->connectivity < connectivity_item2->connectivity){
		return 1;
	}
	else{
		return 0;
	}
}
#endif