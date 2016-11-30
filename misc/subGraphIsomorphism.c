#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "subGraphIsomorphism.h"
#include "base.h"

#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
#include "dijkstra.h"
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
#include "graphLayer.h"
#endif

static struct nodeTab* graphIso_create_node_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
static struct labelTab* graphIso_create_label_tab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
static struct labelTab** graphIso_create_connectivity_mapping(struct labelTab* label_tab, uint32_t nb_label);
#endif
static struct labelFastAccess* graphIso_create_label_fast(struct graph* graph, struct labelTab* label_tab, uint32_t* nb_label);
static struct edgeTab* graphIso_create_edge_tab(struct graph* graph, uint32_t(*edge_get_label)(struct edge*), struct nodeTab* node_tab);

static int32_t compare_labelTabItem_label(const void* arg1, const void* arg2);
static int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2);
#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
static int32_t compare_labelTabPtr_connectivity(const void* arg1, const void* arg2);
#endif

struct possibleAssignmentHeader{
	uint32_t nb_possible_assignment;
	uint32_t node_offset;
	uint32_t assigned;
};

struct possibleAssignment{
	uint32_t 							nb_node;
	struct possibleAssignmentHeader* 	headers;
	uint32_t* 							nodes;
	uint32_t*							stacked_size;
	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	uint32_t* 							node_order;
	#endif
	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	uint64_t 							seed;
	#endif
};

static struct possibleAssignment* possibleAssignment_create(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error);
#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
static int32_t possibleAssignment_duplicate(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value);
#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
static int32_t possibleAssignment_duplicate_light(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value);
#endif
#else
static int32_t possibleAssignment_duplicate(struct graphIsoHandle* graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value);
#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
static int32_t possibleAssignment_duplicate_light(struct graphIsoHandle* graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value);
#endif
#endif
#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
static int32_t possibleAssignment_check(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#endif
static int32_t possibleAssignment_update(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment);
static void possibleAssignment_save_state(struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
static void possibleAssignment_mark_collision_layer_src_dst(uint64_t seed, struct node* node_layer);
static void possibleAssignment_mark_collision_layer_dst(uint64_t seed, struct node* node_layer);
#define possibleAssignment_is_collision(possible_assignment, node_master) (*((uint64_t*)node_get_data(node_get_layer_counterpart(node_master))) == (possible_assignment)->seed)
#define possibleAssignment_clean_layer(node_master) *((uint64_t*)node_get_data(node_get_layer_counterpart(node_master))) = 0
#endif

#define possibleAssignment_delete(possible_assignment) free(possible_assignment);

static uint32_t graphIso_recursive_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct node** assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array);

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
#define node_order(possible_assignment, index) ((possible_assignment)->node_order[(index)])
static void graphIso_choose_next_node(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#else
#define node_order(possible_assignment, index) (index)
#define restore_node_order(possible_assignment, index)
#endif

#define node_offset(possible_assignment, index) ((possible_assignment)->headers[node_order((possible_assignment), (index))].node_offset)
#define node_assigned(possible_assignment, index) ((possible_assignment)->headers[node_order((possible_assignment), (index))].assigned)
#define node_nb_possible_assignment(possible_assignment, index) ((possible_assignment)->headers[node_order((possible_assignment), (index))].nb_possible_assignment)
#define node_index(possible_assignment, index, pos) ((possible_assignment)->nodes[(possible_assignment)->headers[node_order((possible_assignment), (index))].node_offset + (pos)])

#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
#define node_connectivity(sub_graph_handle, possible_assignment, index) ((sub_graph_handle)->node_tab[node_order(possible_assignment, index)].connectivity)
#else
#define node_connectivity(sub_graph_handle, possible_assignment, index) ((sub_graph_handle)->node_tab[node_order(possible_assignment, index)].node->nb_edge_src + (sub_graph_handle)->node_tab[node_order(possible_assignment, index)].node->nb_edge_dst)
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
		log_err("unable to allocate memory");
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

	if ((label_tab = (struct labelTab*)malloc(sizeof(struct labelTab) * graph->nb_node)) != NULL){
		for(node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
			label_tab[i].label 			= node_get_label(node_cursor);
			label_tab[i].node 			= node_cursor;
			#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
			label_tab[i].connectivity 	= node_cursor->nb_edge_src + node_cursor->nb_edge_dst;
			#endif
			#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
			label_tab[i].index 			= i;
			#endif
			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			if (node_has_layer_counterpart(node_cursor)){
				possibleAssignment_clean_layer(node_cursor);
			}
			#endif
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return label_tab;
}

#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
static struct labelTab** graphIso_create_connectivity_mapping(struct labelTab* label_tab, uint32_t nb_node){
	uint32_t 			i;
	struct labelTab** 	connectivity_mapping;

	connectivity_mapping = (struct labelTab**)malloc(sizeof(struct labelTab*) * nb_node);
	if (connectivity_mapping == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for(i = 0; i < nb_node; i++){
		connectivity_mapping[i] = label_tab + i;
	}
	qsort(connectivity_mapping, nb_node, sizeof(struct labelTab*), compare_labelTabPtr_connectivity);

	return connectivity_mapping;
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

	if ((label_fast = (struct labelFastAccess*)malloc(sizeof(struct labelFastAccess) * (*nb_label))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	label_fast[0].label 	= label_tab[0].label;
	label_fast[0].offset 	= 0;
	label_fast[0].size 		= 1;
	prev_label_index = 0;

	for (i = 1; i < graph->nb_node; i++){
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

	return label_fast;
}

static struct edgeTab* graphIso_create_edge_tab(struct graph* graph, uint32_t(*edge_get_label)(struct edge*), struct nodeTab* node_tab){
	struct edgeTab* edge_tab;
	uint32_t 		i = 0;
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;

	edge_tab = (struct edgeTab*)malloc(sizeof(struct edgeTab) * graph->nb_edge);
	if (edge_tab == NULL){
		log_err("unable to allocate memory");
	}
	else{
		for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				edge_tab[i].label 	= edge_get_label(edge_cursor);
				edge_tab[i].src 	= (uint32_t)((struct nodeTab*)(node_cursor->ptr) - node_tab);
				edge_tab[i].dst 	= (uint32_t)((struct nodeTab*)(edge_get_dst(edge_cursor)->ptr) - node_tab);
				i ++;
			}
		}
	}

	return edge_tab;
}

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	uint32_t 				nb_label;
	struct graphIsoHandle* 	handle;

	if (graph->nb_node == 0){
		log_err("unable to create graphHandle for empty graph");
		return NULL;
	}

	if ((handle = (struct graphIsoHandle*)calloc(1, sizeof(struct graphIsoHandle))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	if ((handle->dst = (uint32_t**)calloc(graph->nb_node, sizeof(uint32_t*))) == NULL){
		log_err("unable to allocate memory");
		goto error;
	}
	#endif

	if ((handle->label_tab = graphIso_create_label_tab(graph, node_get_label)) == NULL){
		log_err("unable to create labelTab");
		goto error;
	}

	if ((handle->label_fast = graphIso_create_label_fast(graph, handle->label_tab, &nb_label)) == NULL){
		log_err("unable to create labelFastAccess");
		goto error;
	}

	handle->graph 			= graph;
	handle->nb_label 		= nb_label;
	handle->edge_get_label 	= edge_get_label;

	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	if ((handle->connectivity_mapping = graphIso_create_connectivity_mapping(handle->label_tab, graph->nb_node)) == NULL){
		log_err("unable to create connectivity mapping");
		goto error;
	}
	#endif

	return handle;

	error:
	if (handle->label_fast != NULL){
		free(handle->label_fast);
	}
	if (handle->label_tab != NULL){
		free(handle->label_tab);
	}
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	if (handle->dst != NULL){
		free(handle->dst);
	}
	#endif
	free(handle);

	return NULL;
}

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	struct possibleAssignment* 	possible_assignment;
	struct node** 				assignment;
	struct array*				assignment_array;
	uint8_t 					error;

	if (sub_graph_handle->graph->nb_node == 0){
		return NULL;
	}

	assignment_array = array_create(sizeof(struct node*) * sub_graph_handle->graph->nb_node);
	if (assignment_array == NULL){
		log_err("unable to create array");
		return NULL;
	}

	if ((possible_assignment = possibleAssignment_create(graph_handle, sub_graph_handle, &error)) == NULL){
		if (error){
			log_err("unable to create first possible assignment");
		}
		return assignment_array;
	}

	assignment = (struct node**)alloca(sizeof(struct node*) * sub_graph_handle->graph->nb_node);
	graphIso_recursive_search(graph_handle, sub_graph_handle, assignment, 0, possible_assignment, assignment_array);
	possibleAssignment_delete(possible_assignment);

	return assignment_array;
}

static uint32_t graphIso_recursive_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct node** assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array){
	uint32_t i;
	uint32_t result 						= 0;
	uint32_t local_nb_possible_assignment;
	uint32_t local_node_offset;
	uint32_t nb_assigned 					= 0;

	#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
	for ( ; nb_assignment + nb_assigned < sub_graph_handle->graph->nb_node - 1; nb_assigned ++){
		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
		graphIso_choose_next_node(sub_graph_handle, possible_assignment, nb_assignment + nb_assigned);
		#endif

		if (node_nb_possible_assignment(possible_assignment, nb_assignment + nb_assigned) > 1){
			break;
		}
		else if (possibleAssignment_check(graph_handle, sub_graph_handle, possible_assignment, nb_assignment + nb_assigned)){
			goto quit;
		}

		#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
		possible_assignment->seed ++;
		if (node_has_layer_counterpart(graph_handle->label_tab[node_index(possible_assignment, nb_assignment + nb_assigned, 0)].node)){
			possibleAssignment_mark_collision_layer_src_dst(possible_assignment->seed, node_get_layer_counterpart(graph_handle->label_tab[node_index(possible_assignment, nb_assignment + nb_assigned, 0)].node));
		}
		#endif

		#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
		if (!possibleAssignment_duplicate_light(graph_handle, sub_graph_handle, possible_assignment, nb_assignment + nb_assigned, node_index(possible_assignment, nb_assignment + nb_assigned, 0))){
		#else
		if (!possibleAssignment_duplicate_light(graph_handle, possible_assignment, nb_assignment + nb_assigned, node_index(possible_assignment, nb_assignment + nb_assigned, 0))){
		#endif
			node_assigned(possible_assignment, nb_assignment + nb_assigned) = 1;
			assignment[node_order(possible_assignment, nb_assignment + nb_assigned)] = graph_handle->label_tab[node_index(possible_assignment, nb_assignment + nb_assigned, 0)].node;
		}
		else{
			goto quit;
		}
	}
	#endif

	if (possibleAssignment_update(graph_handle, sub_graph_handle, possible_assignment)){
		#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
		goto quit;
		#else
		return 0;
		#endif
	}

	if (nb_assignment + nb_assigned == sub_graph_handle->graph->nb_node - 1){
		for (i = 0; i < node_nb_possible_assignment(possible_assignment, nb_assignment + nb_assigned); i++){
			assignment[node_order(possible_assignment, nb_assignment + nb_assigned)] = graph_handle->label_tab[node_index(possible_assignment, nb_assignment + nb_assigned, i)].node;
			if (array_add(assignment_array, assignment) < 0){
				log_err("unable to add assignment to array");
			}
			result ++;
		}
	}
	else{
		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1 && SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP != 1
		graphIso_choose_next_node(sub_graph_handle, possible_assignment, nb_assignment + nb_assigned);
		#endif

		node_assigned(possible_assignment, nb_assignment + nb_assigned) = 1;

		possibleAssignment_save_state(possible_assignment, nb_assignment + nb_assigned);

		local_nb_possible_assignment = node_nb_possible_assignment(possible_assignment, nb_assignment + nb_assigned);
		local_node_offset = node_offset(possible_assignment, nb_assignment + nb_assigned);

		for (i = 0; i < local_nb_possible_assignment; i++){
			node_offset(possible_assignment, nb_assignment + nb_assigned) = local_node_offset + i;

			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			possible_assignment->seed ++;
			if (node_has_layer_counterpart(graph_handle->label_tab[possible_assignment->nodes[local_node_offset + i]].node)){
				possibleAssignment_mark_collision_layer_src_dst(possible_assignment->seed, node_get_layer_counterpart(graph_handle->label_tab[possible_assignment->nodes[local_node_offset + i]].node));
			}
			#endif

			#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
			if (!possibleAssignment_duplicate(graph_handle, sub_graph_handle, possible_assignment, nb_assignment + nb_assigned, possible_assignment->nodes[local_node_offset + i])){
			#else
			if (!possibleAssignment_duplicate(graph_handle, possible_assignment, nb_assignment + nb_assigned, possible_assignment->nodes[local_node_offset + i])){
			#endif
				assignment[node_order(possible_assignment, nb_assignment + nb_assigned)] = graph_handle->label_tab[possible_assignment->nodes[local_node_offset + i]].node;
				result += graphIso_recursive_search(graph_handle, sub_graph_handle, assignment, nb_assignment + nb_assigned + 1, possible_assignment, assignment_array);
			}
		}

		node_offset(possible_assignment, nb_assignment + nb_assigned) = local_node_offset;
		node_assigned(possible_assignment, nb_assignment + nb_assigned) = 0;
	}

	#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
	quit:

	for ( ; nb_assigned; ){
		nb_assigned --;
		node_assigned(possible_assignment, nb_assignment + nb_assigned) = 0;
	}
	#endif

	return result;
}

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static void graphIso_choose_next_node(struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint32_t index;
	uint32_t i;
	uint32_t min_nb_assignment;
	uint32_t max_connectivity;
	uint32_t tmp;

	min_nb_assignment = node_nb_possible_assignment(possible_assignment, nb_assignment);
	max_connectivity = node_connectivity(sub_graph_handle, possible_assignment, nb_assignment);
	index = nb_assignment;

	for (i = nb_assignment + 1; i < sub_graph_handle->graph->nb_node; i++){
		if (node_nb_possible_assignment(possible_assignment, i) < min_nb_assignment){
			min_nb_assignment = node_nb_possible_assignment(possible_assignment, i);
			max_connectivity = node_connectivity(sub_graph_handle, possible_assignment, i);
			index = i;
		}
		else if (node_nb_possible_assignment(possible_assignment, i) == min_nb_assignment){
			if (max_connectivity < node_connectivity(sub_graph_handle, possible_assignment, i)){
				min_nb_assignment = node_nb_possible_assignment(possible_assignment, i);
				max_connectivity = node_connectivity(sub_graph_handle, possible_assignment, i);
				index = i;
			}
		}
	}

	tmp = possible_assignment->node_order[index];
	possible_assignment->node_order[index] = possible_assignment->node_order[nb_assignment];
	possible_assignment->node_order[nb_assignment] = tmp;
}
#endif

struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	struct subGraphIsoHandle* 	handle;
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	uint32_t 					i;
	struct node* 				node_cursor;
	#endif

	if ((handle = (struct subGraphIsoHandle*)calloc(1, sizeof(struct subGraphIsoHandle))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	if ((handle->dst = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_node * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		goto error;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL && i < graph->nb_node; node_cursor = node_get_next(node_cursor), i++){
		if (dijkstra_dst(graph, node_cursor, handle->dst + (i * graph->nb_node))){
			log_err("unable to compute graph dst (Dijkstra)");
		}
	}
	#endif

	if ((handle->node_tab = graphIso_create_node_tab(graph, node_get_label)) == NULL){
		log_err("unable to create labelTab");
		goto error;
	}

	if ((handle->edge_tab = graphIso_create_edge_tab(graph, edge_get_label, handle->node_tab)) == NULL){
		log_err("unable to create edgeTab");
		goto error;
	}

	handle->graph = graph;

	return handle;

	error:
	if (handle->node_tab != NULL){
		free(handle->node_tab);
	}
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	if (handle->dst != NULL){
		free(handle->dst);
	}
	#endif
	free(handle);

	return NULL;
}

void graphIso_delete_graph_handle(struct graphIsoHandle* handle){
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	uint32_t i;
	#endif

	free(handle->label_fast);
	free(handle->label_tab);
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	free(handle->connectivity_mapping);
	#endif
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	for (i = 0; i < handle->graph->nb_node; i++){
		if (handle->dst[i] != NULL){
			free(handle->dst[i]);
		}
	}
	free(handle->dst);
	#endif
	free(handle);
}

void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle){
	free(handle->edge_tab);
	#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
	free(handle->dst);
	#endif
	free(handle->node_tab);
	free(handle);
}

/* ===================================================================== */
/* Possible assignment routines 										 */
/* ===================================================================== */

struct graphNodeList{
	uint32_t offset;
	uint32_t size;
};

static struct possibleAssignment* possibleAssignment_create(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error){
	uint32_t 					nb_assignment = 0;
	uint32_t 					i;
	uint32_t 					j;
	struct labelFastAccess* 	label_fast_access;
	struct possibleAssignment* 	possible_assignment = NULL;
	struct graphNodeList* 		graph_node_list;

	if (error != NULL){
		*error = 0;
	}

	graph_node_list = (struct graphNodeList*)malloc(sizeof(struct graphNodeList) * sub_graph_handle->graph->nb_node);
	if (graph_node_list == NULL){
		log_err("unable to allocate memory");
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
				if (graph_handle->label_tab[label_fast_access->offset + j].connectivity < sub_graph_handle->node_tab[i].connectivity){
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
				if (graph_handle->connectivity_mapping[j]->connectivity < sub_graph_handle->node_tab[i].connectivity){
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

	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	possible_assignment = (struct possibleAssignment*)malloc(sizeof(struct possibleAssignment) + sub_graph_handle->graph->nb_node * sizeof(struct possibleAssignmentHeader) + nb_assignment * sizeof(struct node*) + sub_graph_handle->graph->nb_node * sub_graph_handle->graph->nb_node * sizeof(uint32_t) + sub_graph_handle->graph->nb_node * sizeof(uint32_t));
	#else
	possible_assignment = (struct possibleAssignment*)malloc(sizeof(struct possibleAssignment) + sub_graph_handle->graph->nb_node * sizeof(struct possibleAssignmentHeader) + nb_assignment * sizeof(struct node*) + sub_graph_handle->graph->nb_node * sub_graph_handle->graph->nb_node * sizeof(uint32_t));
	#endif
	if (possible_assignment != NULL){
		possible_assignment->nb_node 		= sub_graph_handle->graph->nb_node;
		possible_assignment->headers 		= (struct possibleAssignmentHeader*)(possible_assignment + 1);
		possible_assignment->nodes 			= (uint32_t*)(possible_assignment->headers + sub_graph_handle->graph->nb_node);
		possible_assignment->stacked_size 	= (uint32_t*)(possible_assignment->nodes + nb_assignment);

		#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
		possible_assignment->node_order 	= (uint32_t*)(possible_assignment->stacked_size + sub_graph_handle->graph->nb_node * sub_graph_handle->graph->nb_node);

		for (i = 0; i < sub_graph_handle->graph->nb_node; i++){
			possible_assignment->node_order[i] = i;
		}
		#endif

		#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
		possible_assignment->seed = 1;
		#endif

		possible_assignment->headers[0].node_offset = 0;

		for	(i = 0; i < sub_graph_handle->graph->nb_node; i++){
			possible_assignment->headers[i].nb_possible_assignment = graph_node_list[i].size;
			possible_assignment->headers[i].assigned = 0;

			if (i != sub_graph_handle->graph->nb_node - 1){
				possible_assignment->headers[i + 1].node_offset = possible_assignment->headers[i].node_offset + graph_node_list[i].size;
			}

			if (sub_graph_handle->node_tab[i].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
				for (j = 0; j < graph_node_list[i].size; j++){
					possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = graph_node_list[i].offset + j;
				}
			}
			else{
				#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
				for (j = 0; j < graph_node_list[i].size; j++){
					possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = (uint32_t)(graph_handle->connectivity_mapping[j] - graph_handle->label_tab);
				}
				#else
				for (j = 0; j < graph_node_list[i].size; j++){
					possible_assignment->nodes[possible_assignment->headers[i].node_offset + j] = j;
				}
				#endif
			}
		}
	}
	else{
		log_err("unable to allocate memory");
		if (error != NULL){
			*error = 1;
		}
	}

	exit:
	free(graph_node_list);

	return possible_assignment;
}

#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
static int32_t possibleAssignment_duplicate(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	tmp;
	uint32_t 	size;
	uint32_t* 	possible_assignment_local_ptr;

	node_nb_possible_assignment(possible_assignment, nb_assignment) = 1;
	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		node_nb_possible_assignment(possible_assignment, i) = possible_assignment->stacked_size[node_order(possible_assignment, nb_assignment) * possible_assignment->nb_node + node_order(possible_assignment, i)];
	}

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		if (sub_graph_handle->dst[sub_graph_handle->graph->nb_node * node_order(possible_assignment, nb_assignment) + node_order(possible_assignment, i)] != DIJKSTRA_INVALID_DST){
			size = node_nb_possible_assignment(possible_assignment, i);
			possible_assignment_local_ptr = possible_assignment->nodes + node_offset(possible_assignment, i);

			for (j = 0; j < size; ){
				if (possible_assignment_local_ptr[j] == new_assignment_value){
					size --;
					possible_assignment_local_ptr[j] = possible_assignment_local_ptr[size];
					possible_assignment_local_ptr[size] = new_assignment_value;
				}
				#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
				else if (node_has_layer_counterpart(graph_handle->label_tab[possible_assignment_local_ptr[j]].node) && possibleAssignment_is_collision(possible_assignment, graph_handle->label_tab[possible_assignment_local_ptr[j]].node)){
					size --;
					tmp = possible_assignment_local_ptr[size];
					possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
					possible_assignment_local_ptr[j] = tmp;
				}
				#endif
				else{
					uint32_t index_k = (uint32_t)(graph_handle->label_tab[new_assignment_value].index);
					uint32_t index_i = (uint32_t)(graph_handle->label_tab[possible_assignment_local_ptr[j]].index);

					if (graph_handle->dst[index_k] == NULL){
						graph_handle->dst[index_k] = (uint32_t*)malloc(sizeof(uint32_t) * graph_handle->graph->nb_node);
						if (graph_handle->dst[index_k] == NULL){
							log_err("unable to allocate memory");
							return -1;
						}
						if (dijkstra_dst(graph_handle->graph, graph_handle->label_tab[new_assignment_value].node, graph_handle->dst[index_k])){
							log_err("unable to compute graph dst (Dijkstra)");
							return -1;
						}
					}

					if (sub_graph_handle->dst[sub_graph_handle->graph->nb_node * node_order(possible_assignment, nb_assignment) + node_order(possible_assignment, i)] >= graph_handle->dst[index_k][index_i]){
						j ++;
					}
					else{
						size --;
						tmp = possible_assignment_local_ptr[size];
						possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
						possible_assignment_local_ptr[j] = tmp;
					}
				}
			}
			if (!size){
				return -1;
			}
			node_nb_possible_assignment(possible_assignment, i) = size;

		}
		else{
			log_err("this case is not supposed to happen, subgraph is not connected");
		}
	}

	return 0;
}
#else
static int32_t possibleAssignment_duplicate(struct graphIsoHandle* graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	size;
	uint32_t* 	possible_assignment_local_ptr;
	uint32_t 	tmp;

	node_nb_possible_assignment(possible_assignment, nb_assignment) = 1;

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		node_nb_possible_assignment(possible_assignment, i) = possible_assignment->stacked_size[node_order(possible_assignment, nb_assignment) * possible_assignment->nb_node + node_order(possible_assignment, i)];
	}

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		size = node_nb_possible_assignment(possible_assignment, i);
		possible_assignment_local_ptr = possible_assignment->nodes + node_offset(possible_assignment, i);

		for (j = 0; j < size; ){
			if (possible_assignment_local_ptr[j] == new_assignment_value){
				if (!(--size)){
					return -1;
				}
				possible_assignment_local_ptr[j] = possible_assignment_local_ptr[size];
				possible_assignment_local_ptr[size] = new_assignment_value;
				break;
			}
			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			else if (node_has_layer_counterpart(graph_handle->label_tab[possible_assignment_local_ptr[j]].node) && possibleAssignment_is_collision(possible_assignment, graph_handle->label_tab[possible_assignment_local_ptr[j]].node)){
				if (!(--size)){
					return -1;
				}
				tmp = possible_assignment_local_ptr[j];
				possible_assignment_local_ptr[j] = possible_assignment_local_ptr[size];
				possible_assignment_local_ptr[size] = tmp;
			}
			#endif
			else{
				j++;
			}
		}
		node_nb_possible_assignment(possible_assignment, i) = size;
	}

	return 0;
}
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
#if SUBGRAPHISOMORPHISM_OPTIM_MIN_DST == 1
static int32_t possibleAssignment_duplicate_light(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	tmp;
	uint32_t 	size;
	uint32_t* 	possible_assignment_local_ptr;

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		if (sub_graph_handle->dst[sub_graph_handle->graph->nb_node * node_order(possible_assignment, nb_assignment) + node_order(possible_assignment, i)] != DIJKSTRA_INVALID_DST){
			size = node_nb_possible_assignment(possible_assignment, i);
			possible_assignment_local_ptr = possible_assignment->nodes + node_offset(possible_assignment, i);

			for (j = 0; j < size; ){
				if (possible_assignment_local_ptr[j] == new_assignment_value){
					size --;
					possible_assignment_local_ptr[j] = possible_assignment_local_ptr[size];
					possible_assignment_local_ptr[size] = new_assignment_value;
				}
				#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
				else if (node_has_layer_counterpart(graph_handle->label_tab[possible_assignment_local_ptr[j]].node) && possibleAssignment_is_collision(possible_assignment, graph_handle->label_tab[possible_assignment_local_ptr[j]].node)){
					size --;
					tmp = possible_assignment_local_ptr[size];
					possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
					possible_assignment_local_ptr[j] = tmp;
				}
				#endif
				else{
					uint32_t index_k = (uint32_t)(graph_handle->label_tab[new_assignment_value].index);
					uint32_t index_i = (uint32_t)(graph_handle->label_tab[possible_assignment_local_ptr[j]].index);

					if (graph_handle->dst[index_k] == NULL || sub_graph_handle->dst[sub_graph_handle->graph->nb_node * node_order(possible_assignment, nb_assignment) + node_order(possible_assignment, i)] >= graph_handle->dst[index_k][index_i]){
						j ++;
					}
					else{
						size --;
						tmp = possible_assignment_local_ptr[size];
						possible_assignment_local_ptr[size] = possible_assignment_local_ptr[j];
						possible_assignment_local_ptr[j] = tmp;
					}
				}
			}
			if (!size){
				return -1;
			}
			node_nb_possible_assignment(possible_assignment, i) = size;

		}
		else{
			log_err("this case is not supposed to happen, subgraph is not connected");
		}
	}

	return 0;
}
#else
static int32_t possibleAssignment_duplicate_light(struct graphIsoHandle* graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment, uint32_t new_assignment_value){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	size;
	uint32_t* 	possible_assignment_local_ptr;
	uint32_t 	tmp;

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		size = node_nb_possible_assignment(possible_assignment, i);
		possible_assignment_local_ptr = possible_assignment->nodes + node_offset(possible_assignment, i);

		for (j = 0; j < size; ){
			if (possible_assignment_local_ptr[j] == new_assignment_value){
				if (!(--size)){
					return -1;
				}
				possible_assignment_local_ptr[j] = possible_assignment_local_ptr[size];
				possible_assignment_local_ptr[size] = new_assignment_value;
			}
			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			else if (node_has_layer_counterpart(graph_handle->label_tab[possible_assignment_local_ptr[j]].node) && possibleAssignment_is_collision(possible_assignment, graph_handle->label_tab[possible_assignment_local_ptr[j]].node)){
				if (!(--size)){
					return -1;
				}
				tmp = possible_assignment_local_ptr[j];
				possible_assignment_local_ptr[j] = possible_assignment_local_ptr[size];
				possible_assignment_local_ptr[size] = tmp;
			}
			#endif
			else{
				j++;
			}
		}
		node_nb_possible_assignment(possible_assignment, i) = size;
	}

	return 0;
}
#endif
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
static int32_t possibleAssignment_check(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint32_t 		i;
	uint32_t 		j;
	struct edge* 	edge;
	uint32_t 		src;
	uint32_t 		dst;

	for (i = 0; i < sub_graph_handle->graph->nb_edge; i++){
		src = sub_graph_handle->edge_tab[i].src;
		dst = sub_graph_handle->edge_tab[i].dst;

		if (src == node_order(possible_assignment, nb_assignment)){
			for (j = 0; j < possible_assignment->headers[dst].nb_possible_assignment; j++){
				for (edge = node_get_head_edge_src(graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[src].node_offset]].node); edge != NULL; edge = edge_get_next_src(edge)){
					if (sub_graph_handle->edge_tab[i].label == graph_handle->edge_get_label(edge) && graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[dst].node_offset + j]].node == edge_get_dst(edge)){
						goto next1;
					}
				}
			}
			return -1;
		}

		next1:;

		if (dst == node_order(possible_assignment, nb_assignment)){
			for (j = 0; j < possible_assignment->headers[src].nb_possible_assignment; j++){
				for (edge = node_get_head_edge_dst(graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[dst].node_offset]].node); edge != NULL; edge = edge_get_next_dst(edge)){
					if (sub_graph_handle->edge_tab[i].label == graph_handle->edge_get_label(edge) && graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[src].node_offset + j]].node == edge_get_src(edge)){
						goto next2;
					}
				}
			}
			return -1;
		}

		next2:;
	}

	return 0;
}
#endif

static int32_t possibleAssignment_update(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, struct possibleAssignment* possible_assignment){
	uint8_t 		match;
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		l;
	uint8_t 		restart;
	struct edge* 	edge;
	uint32_t 		src;
	uint32_t 		dst;

	do{
		restart = 0;

		for (i = 0; i < sub_graph_handle->graph->nb_edge; i++){
			src = sub_graph_handle->edge_tab[i].src;
			dst = sub_graph_handle->edge_tab[i].dst;

			if (!possible_assignment->headers[src].assigned){
				for (j = 0; j < possible_assignment->headers[src].nb_possible_assignment; ){
					for (edge = node_get_head_edge_src(graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[src].node_offset + j]].node), match = 0; (uint32_t)edge & (match + 0xffffffff); edge = edge_get_next_src(edge)){
						if (sub_graph_handle->edge_tab[i].label == graph_handle->edge_get_label(edge)){

							for (l = 0; l < possible_assignment->headers[dst].nb_possible_assignment; l++){
								if (graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[dst].node_offset + l]].node == edge_get_dst(edge)){
									match = 1;
									break;
								}
							}
						}
					}

					if (!match){
						uint32_t tmp;

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
					for (edge = node_get_head_edge_dst(graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[dst].node_offset + j]].node), match = 0; (uint32_t)edge & (match + 0xffffffff); edge = edge_get_next_dst(edge)){
						if (sub_graph_handle->edge_tab[i].label == graph_handle->edge_get_label(edge)){

							for (l = 0; l < possible_assignment->headers[src].nb_possible_assignment; l++){
								if (graph_handle->label_tab[possible_assignment->nodes[possible_assignment->headers[src].node_offset + l]].node == edge_get_src(edge)){
									match = 1;
									break;
								}
							}
						}
					}

					if (!match){
						uint32_t tmp;

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
	} while (restart);

	return 0;
}


static void possibleAssignment_save_state(struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint32_t i;

	for (i = nb_assignment + 1; i < possible_assignment->nb_node; i++){
		possible_assignment->stacked_size[node_order(possible_assignment, nb_assignment) * possible_assignment->nb_node + node_order(possible_assignment, i)] = node_nb_possible_assignment(possible_assignment, i);
	}
}

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
static void possibleAssignment_mark_collision_layer_src_dst(uint64_t seed, struct node* node_layer){
	struct edge* 	edge_cursor;
	uint64_t* 		data;

	data = (uint64_t*)node_get_data(node_layer);
	if (*data == seed){
		return;
	}

	*data = seed;

	for (edge_cursor = node_get_head_edge_dst(node_layer); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		possibleAssignment_mark_collision_layer_dst(seed, edge_get_src(edge_cursor));
	}
	for (edge_cursor = node_get_head_edge_src(node_layer); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		possibleAssignment_mark_collision_layer_src_dst(seed, edge_get_dst(edge_cursor));
	}

}

static void possibleAssignment_mark_collision_layer_dst(uint64_t seed, struct node* node_layer){
	struct edge* 	edge_cursor;
	uint64_t* 		data;

	data = (uint64_t*)node_get_data(node_layer);
	if (*data == seed){
		return;
	}

	*data = seed;

	for (edge_cursor = node_get_head_edge_dst(node_layer); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		possibleAssignment_mark_collision_layer_dst(seed, edge_get_src(edge_cursor));
	}
}
#endif


/* ===================================================================== */
/* Compare routines 													 */
/* ===================================================================== */

static int32_t compare_labelTabItem_label(const void* arg1, const void* arg2){
	const struct labelTab* label_item1 = (const struct labelTab*)arg1;
	const struct labelTab* label_item2 = (const struct labelTab*)arg2;

	if (label_item1->label < label_item2->label){
		return -1;
	}
	else if (label_item1->label > label_item2->label){
		return 1;
	}
	#if SUBGRAPHISOMORPHISM_OPTIM_CONNECTIVITY == 1
	if (label_item1->connectivity > label_item2->connectivity){
		return -1;
	}
	else if (label_item1->connectivity < label_item2->connectivity){
		return 1;
	}
	#endif
	else{
		return 0;
	}
}

static int32_t compare_key_labelFastAccess(const void* arg1, const void* arg2){
	uint32_t 						key = *(const uint32_t*)arg1;
	const struct labelFastAccess* 	fast_access = (const struct labelFastAccess*)arg2;

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
static int32_t compare_labelTabPtr_connectivity(const void* arg1, const void* arg2){
	struct labelTab const* label_item1 = *(struct labelTab* const*)arg1;
	struct labelTab const* label_item2 = *(struct labelTab* const*)arg2;

	if (label_item1->connectivity > label_item2->connectivity){
		return -1;
	}
	else if (label_item1->connectivity < label_item2->connectivity){
		return 1;
	}
	else{
		return 0;
	}
}
#endif
