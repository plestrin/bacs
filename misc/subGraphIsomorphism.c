#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "subGraphIsomorphism.h"
#include "base.h"

// #if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
// #include "graphLayer.h"
// #endif

static struct nodeTab* graphIso_create_nodeTab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
static struct subNodeTab* graphIso_create_subNodeTab(struct graph* graph, uint32_t(*node_get_label)(struct node*));
static struct edgeTab* graphIso_create_edgeTab(struct graph* graph, uint32_t(*edge_get_label)(struct edge*));

static uint32_t* graphIso_create_src_fast(struct edgeTab* edge_tab, uint32_t nb_edge, struct subNodeTab* sub_node_tab);
static uint32_t* graphIso_create_dst_fast(struct edgeTab* edge_tab, uint32_t nb_edge, struct subNodeTab* sub_node_tab);

static void* bsearch_r(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*, void*), void* arg);

struct compareArgument{
	struct nodeTab* node_tab[2];
	struct edgeTab* edge_tab[2];
};

static int32_t compare_edgeTab_src(const void* arg1, const void* arg2, void* arg);
static int32_t compare_edgeTab_dst(const void* arg1, const void* arg2, void* arg);

struct searchArgument{
	struct subNodeTab* 	sub_node_tab;
	struct nodeTab* 	node_tab;
	struct edgeTab* 	edge_tab[2];
};

static int32_t search_edgeTab_src(const void* key, const void* el, void* arg);
static int32_t search_edgeTab_dst(const void* key, const void* el, void* arg);

static int32_t compare_fast_src(const void* arg1, const void* arg2, void* arg);
static int32_t compare_fast_dst(const void* arg1, const void* arg2, void* arg);

#define graphIsoHandle_get_nb_edge(graph_handle) ((graph_handle)->graph->nb_edge)
#define graphIsoHandle_edge_get_src(graph_handle, edge_index) ((graph_handle)->node_tab[(graph_handle)->edge_tab[(edge_index)].src])
#define graphIsoHandle_edge_get_dst(graph_handle, edge_index) ((graph_handle)->node_tab[(graph_handle)->edge_tab[(edge_index)].dst])

#define subGraphIsoHandle_get_nb_node(sub_graph_handle) ((sub_graph_handle)->graph->nb_node)
#define subGraphIsoHandle_get_nb_edge(sub_graph_handle) ((sub_graph_handle)->graph->nb_edge)
#define subGraphIsoHandle_edge_get_src(sub_graph_handle, edge_index) ((sub_graph_handle)->sub_node_tab[(sub_graph_handle)->edge_tab[(edge_index)].src])
#define subGraphIsoHandle_edge_get_dst(sub_graph_handle, edge_index) ((sub_graph_handle)->sub_node_tab[(sub_graph_handle)->edge_tab[(edge_index)].dst])

struct nodeMeta{
	uint32_t restart_ctr;
	#if SUBGRAPHISOMORPHISM_OPTIM_MARK_ASSIGNED == 1
	uint32_t assigned;
	#endif
};

struct edgeMeta{
	uint32_t offset;
	uint32_t size;
};

struct possibleAssignment{
	uint32_t 			nb_node;
	uint32_t 			nb_edge;
	struct nodeMeta* 	node_meta_buffer;
	struct edgeMeta* 	edge_meta_buffer;
	uint32_t* 			assignment_buffer;
	uint32_t*			save_size;
	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	uint32_t* 			order;
	#endif
	/*#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	uint64_t 			seed;
	#endif*/
};

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
#define possibleAssignment_get_size(nb_node, nb_edge, nb_possible_assignment) (sizeof(struct possibleAssignment) + (nb_node) * sizeof(struct nodeMeta) + (nb_edge) * sizeof(struct edgeMeta) + (nb_possible_assignment) * sizeof(uint32_t) + (nb_edge) * (nb_edge) * sizeof(uint32_t) + (nb_node) * sizeof(uint32_t) + (nb_edge) * sizeof(uint32_t))
#else
#define possibleAssignment_get_size(nb_node, nb_edge, nb_possible_assignment) (sizeof(struct possibleAssignment) + (nb_node) * sizeof(struct nodeMeta) + (nb_edge) * sizeof(struct edgeMeta) + (nb_possible_assignment) * sizeof(uint32_t) + (nb_edge) * (nb_edge) * sizeof(uint32_t) + (nb_node) * sizeof(uint32_t))
#endif

static struct possibleAssignment* possibleAssignment_create(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error);
#if SUBGRAPHISOMORPHISM_OPTIM_LAZY_DUP == 1
static int32_t possibleAssignment_duplicate_lazy(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value);
#define possibleAssignment_duplicate possibleAssignment_duplicate_lazy
#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
static int32_t possibleAssignment_duplicate_light_lazy(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value);
#define possibleAssignment_duplicate_light possibleAssignment_duplicate_light_lazy
#endif
#else
static int32_t possibleAssignment_duplicate(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value);
#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
static int32_t possibleAssignment_duplicate_light(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value);
#endif
#endif

static void possibleAssignment_save(struct possibleAssignment* possible_assignment, uint32_t nb_assignment);

static int32_t possibleAssignment_global_update_first(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle);
#if SUBGRAPHISOMORPHISM_OPTIM_GLOBAL == 1
static int32_t possibleAssignment_global_update(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, void* assignment, uint32_t nb_assignment, uint32_t new_value);
#define possibleAssignment_update possibleAssignment_global_update
#else
static int32_t possibleAssignment_local_update(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, void* assignment, uint32_t nb_assignment, uint32_t new_value);
#define possibleAssignment_update possibleAssignment_local_update
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT == 1
static void possibleAssignment_compact(struct possibleAssignment* possible_assignment);
#else
#define possibleAssignment_compact(possible_assignment)
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static void possibleAssignment_choose_next(struct possibleAssignment* possible_assignment, uint32_t nb_assignment);
#define possibleAssignment_order(possible_assignment, i) ((possible_assignment)->order[(i)])
#else
#define possibleAssignment_choose_next(possible_assignment, nb_assignment)
#define possibleAssignment_order(possible_assignment, i) (i)
#endif

#define possibleAssignment_size(possible_assignment, i) ((possible_assignment)->edge_meta_buffer[possibleAssignment_order(possible_assignment, i)].size)
#define possibleAssignment_offset(possible_assignment, i) ((possible_assignment)->edge_meta_buffer[possibleAssignment_order(possible_assignment, i)].offset)
#define possibleAssignment_assignment(possible_assignment, i, j) ((possible_assignment)->assignment_buffer[possibleAssignment_offset(possible_assignment, i) + (j)])
#define possibleAssignment_save_size(possible_assignment, i, j) ((possible_assignment)->save_size[(i) * (possible_assignment)->nb_edge + possibleAssignment_order(possible_assignment, j)])

#if SUBGRAPHISOMORPHISM_OPTIM_MARK_ASSIGNED == 1
#define possibleAssignment_is_assigned(possible_assignment, i) ((possible_assignment)->node_meta_buffer[(i)].assigned != 0xffffffff)
#else
#define possibleAssignment_is_assigned(possible_assignment, i) 0
#endif

#define possibleAssignment_delete(possible_assignment) free(possible_assignment)

static void graphIso_assign_recursive_edge(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, void* assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array);

/* ===================================================================== */
/* Handle routines 														 */
/* ===================================================================== */

static struct nodeTab* graphIso_create_nodeTab(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct node* 		node_cursor;
	uint32_t 			i;
	struct nodeTab* 	node_tab;

	if ((node_tab = (struct nodeTab*)malloc(sizeof(struct nodeTab) * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for(node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_tab[i].label 	= node_get_label(node_cursor);
		node_tab[i].node 	= node_cursor;
		node_cursor->ptr = (void*)i;
	}

	return node_tab;
}

static struct subNodeTab* graphIso_create_subNodeTab(struct graph* graph, uint32_t(*node_get_label)(struct node*)){
	struct node* 		node_cursor;
	uint32_t 			i;
	struct subNodeTab* 	sub_node_tab;

	if ((sub_node_tab = (struct subNodeTab*)malloc(sizeof(struct subNodeTab) * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for(node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		sub_node_tab[i].label 	= node_get_label(node_cursor);
		sub_node_tab[i].node 	= node_cursor;
		sub_node_tab[i].off_src = 0;
		sub_node_tab[i].nb_src 	= 0;
		sub_node_tab[i].off_dst = 0;
		sub_node_tab[i].nb_dst 	= 0;
		node_cursor->ptr = (void*)i;
	}

	return sub_node_tab;
}

static struct edgeTab* graphIso_create_edgeTab(struct graph* graph, uint32_t(*edge_get_label)(struct edge*)){
	struct edgeTab* edge_tab;
	uint32_t 		i;
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;

	if ((edge_tab = (struct edgeTab*)malloc(sizeof(struct edgeTab) * graph->nb_edge)) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			edge_tab[i].label 	= edge_get_label(edge_cursor);
			edge_tab[i].edge 	= edge_cursor;
			edge_tab[i].src 	= (uint32_t)(node_cursor->ptr);
			edge_tab[i].dst 	= (uint32_t)(edge_get_dst(edge_cursor)->ptr);
			i ++;
		}
	}

	return edge_tab;
}

static uint32_t* graphIso_create_src_fast(struct edgeTab* edge_tab, uint32_t nb_edge, struct subNodeTab* sub_node_tab){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t* 	result;

	if ((result = (uint32_t*)malloc(sizeof(uint32_t) * nb_edge)) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (i = 0; i < nb_edge; i++){
		result[i] = i;
	}

	qsort_r(result, nb_edge, sizeof(uint32_t), compare_fast_src, edge_tab);

	for (i = 0; i < nb_edge; i += j){
		for (j = 1; i + j < nb_edge; j++){
			if (edge_tab[result[i]].src != edge_tab[result[i + j]].src){
				break;
			}
		}
		sub_node_tab[edge_tab[result[i]].src].off_src = i;
		sub_node_tab[edge_tab[result[i]].src].nb_src = j;
	}

	return result;
}

static uint32_t* graphIso_create_dst_fast(struct edgeTab* edge_tab, uint32_t nb_edge, struct subNodeTab* sub_node_tab){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t* 	result;

	if ((result = (uint32_t*)malloc(sizeof(uint32_t) * nb_edge)) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (i = 0; i < nb_edge; i++){
		result[i] = i;
	}

	qsort_r(result, nb_edge, sizeof(uint32_t), compare_fast_dst, edge_tab);

	for (i = 0; i < nb_edge; i += j){
		for (j = 1; i + j < nb_edge; j++){
			if (edge_tab[result[i]].dst != edge_tab[result[i + j]].dst){
				break;
			}
		}
		sub_node_tab[edge_tab[result[i]].dst].off_dst = i;
		sub_node_tab[edge_tab[result[i]].dst].nb_dst = j;
	}

	return result;
}

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	uint32_t 				i;
	struct node* 			node_cursor;
	struct graphIsoHandle* 	handle;
	struct compareArgument 	cmp_arg;
	void** 					save_ptr = NULL;

	if (graph->nb_node == 0){
		log_err("unable to create graphIsoHandle for empty graph");
		return NULL;
	}

	if ((handle = (struct graphIsoHandle*)calloc(1, sizeof(struct graphIsoHandle))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	if ((save_ptr = (void**)malloc(sizeof(void*) * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		goto error;
	}

	for (i = 0, node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		save_ptr[i] = node_cursor->ptr;
	}

	if ((handle->node_tab = graphIso_create_nodeTab(graph, node_get_label)) == NULL){
		log_err("unable to create nodeTab");
		goto error;
	}

	if ((handle->edge_tab = graphIso_create_edgeTab(graph, edge_get_label)) == NULL){
		log_err("unable to create edgeTab");
		goto error;
	}

	for (i = 0, node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = save_ptr[i];
	}

	free(save_ptr);
	save_ptr = NULL;

	handle->src_edge_mapping = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_edge);
	handle->dst_edge_mapping = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_edge);

	for (i = 0; i < graph->nb_edge; i++){
		handle->src_edge_mapping[i] = i;
		handle->dst_edge_mapping[i] = i;
	}

	if (handle->src_edge_mapping == NULL || handle->dst_edge_mapping == NULL){
		log_err("unable to allocate memory");
		goto error;
	}

	cmp_arg.node_tab[0] = handle->node_tab;
	cmp_arg.node_tab[1] = handle->node_tab;
	cmp_arg.edge_tab[0] = handle->edge_tab;
	cmp_arg.edge_tab[1] = handle->edge_tab;

	qsort_r(handle->src_edge_mapping, graph->nb_edge, sizeof(uint32_t), compare_edgeTab_src, &cmp_arg);
	qsort_r(handle->dst_edge_mapping, graph->nb_edge, sizeof(uint32_t), compare_edgeTab_dst, &cmp_arg);

	handle->graph = graph;

	return handle;

	error:
	if (handle->src_edge_mapping != NULL){
		free(handle->src_edge_mapping);
	}
	if (handle->dst_edge_mapping != NULL){
		free(handle->dst_edge_mapping);
	}
	if (handle->edge_tab != NULL){
		free(handle->edge_tab);
	}
	if (handle->node_tab != NULL){
		free(handle->node_tab);
	}
	if (save_ptr != NULL){
		free(save_ptr);
	}
	free(handle);

	return NULL;
}

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	struct possibleAssignment* 	possible_assignment;
	void* 						assignment;
	struct array*				assignment_array;
	uint8_t 					error;

	if ((assignment_array = array_create(assignment_get_size(subGraphIsoHandle_get_nb_node(sub_graph_handle), subGraphIsoHandle_get_nb_edge(sub_graph_handle)))) == NULL){
		log_err("unable to create array");
		return NULL;
	}

	if ((assignment = malloc(assignment_get_size(subGraphIsoHandle_get_nb_node(sub_graph_handle), subGraphIsoHandle_get_nb_edge(sub_graph_handle)))) == NULL){
		log_err("unable to allocate memory");
		return assignment_array;
	}

	if ((possible_assignment = possibleAssignment_create(graph_handle, sub_graph_handle, &error)) == NULL){
		if (error){
			log_err("unable to create first possible assignment");
		}
		free(assignment);
		return assignment_array;
	}

	if (!possibleAssignment_global_update_first(possible_assignment, graph_handle, sub_graph_handle)){
		graphIso_assign_recursive_edge(graph_handle, sub_graph_handle, assignment, 0, possible_assignment, assignment_array);
	}

	possibleAssignment_delete(possible_assignment);
	free(assignment);

	return assignment_array;
}

static void graphIso_assign_recursive_edge(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, void* assignment, uint32_t nb_assignment, struct possibleAssignment* possible_assignment, struct array* assignment_array){
	uint32_t i;
	uint32_t new_value;
	uint32_t size;

	if (nb_assignment == subGraphIsoHandle_get_nb_edge(sub_graph_handle)){
		/* check that every nodes have been affected - later */
		if (array_add(assignment_array, assignment) < 0){
			log_err("unable to add element to array");
		}
		return;
	}

	possibleAssignment_choose_next(possible_assignment, nb_assignment);

	size = possibleAssignment_size(possible_assignment, nb_assignment);
	#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1
	if (size == 1){
		new_value = possibleAssignment_assignment(possible_assignment, nb_assignment, 0);

		if (possibleAssignment_duplicate_light(possible_assignment, graph_handle, sub_graph_handle, nb_assignment, new_value)){
			return;
		}

		if (!possibleAssignment_update(possible_assignment, graph_handle, sub_graph_handle, assignment, nb_assignment, new_value)){
			graphIso_assign_recursive_edge(graph_handle, sub_graph_handle, assignment, nb_assignment + 1, possible_assignment, assignment_array);
		}
		return;
	}
	#endif

	possibleAssignment_save(possible_assignment, nb_assignment);

	size = possibleAssignment_size(possible_assignment, nb_assignment);
	possibleAssignment_offset(possible_assignment, nb_assignment) += size;
	possibleAssignment_size(possible_assignment, nb_assignment) = 1;

	for (i = 0; i < size; i++){
		possibleAssignment_offset(possible_assignment, nb_assignment)--;
		new_value = possibleAssignment_assignment(possible_assignment, nb_assignment, 0);

		if (possibleAssignment_duplicate(possible_assignment, graph_handle, sub_graph_handle, nb_assignment, new_value)){
			continue;
		}

		if (!possibleAssignment_update(possible_assignment, graph_handle, sub_graph_handle, assignment, nb_assignment, new_value)){
			graphIso_assign_recursive_edge(graph_handle, sub_graph_handle, assignment, nb_assignment + 1, possible_assignment, assignment_array);
		}
	}

	return;
}

#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
static void possibleAssignment_choose_next(struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint32_t i;
	uint32_t j;
	uint32_t min_size;
	uint32_t tmp;

	min_size = possibleAssignment_size(possible_assignment, nb_assignment);
	j = nb_assignment;

	for (i = nb_assignment + 1; i < possible_assignment->nb_edge; i++){
		if (possibleAssignment_size(possible_assignment, i) < min_size){
			min_size = possibleAssignment_size(possible_assignment, i);
			j = i;
		}
	}

	tmp = possible_assignment->order[j];
	possible_assignment->order[j] = possible_assignment->order[nb_assignment];
	possible_assignment->order[nb_assignment] = tmp;
}
#endif

struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*)){
	struct subGraphIsoHandle* handle;

	if (graph->nb_node == 0){
		log_err("unable to create subGraphIsoHandle for empty graph");
		return NULL;
	}

	if ((handle = (struct subGraphIsoHandle*)calloc(1, sizeof(struct subGraphIsoHandle))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	if ((handle->sub_node_tab = graphIso_create_subNodeTab(graph, node_get_label)) == NULL){
		log_err("unable to create nodeTab");
		goto error;
	}

	if ((handle->edge_tab = graphIso_create_edgeTab(graph, edge_get_label)) == NULL){
		log_err("unable to create edgeTab");
		goto error;
	}

	if ((handle->src_fast = graphIso_create_src_fast(handle->edge_tab, graph->nb_edge, handle->sub_node_tab)) == NULL){
		log_err("unable to create src fast mapping");
		goto error;
	}

	if ((handle->dst_fast = graphIso_create_dst_fast(handle->edge_tab, graph->nb_edge, handle->sub_node_tab)) == NULL){
		log_err("unable to create src fast mapping");
		goto error;
	}

	handle->graph = graph;

	return handle;

	error:
	if (handle->src_fast != NULL){
		free(handle->src_fast);
	}
	if (handle->dst_fast != NULL){
		free(handle->dst_fast);
	}
	if (handle->edge_tab != NULL){
		free(handle->edge_tab);
	}
	if (handle->sub_node_tab != NULL){
		free(handle->sub_node_tab);
	}
	free(handle);

	return NULL;
}

void graphIso_delete_graph_handle(struct graphIsoHandle* graph_handle){
	free(graph_handle->edge_tab);
	free(graph_handle->node_tab);
	free(graph_handle->src_edge_mapping);
	free(graph_handle->dst_edge_mapping);
	free(graph_handle);
}

void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* sub_graph_handle){
	free(sub_graph_handle->edge_tab);
	free(sub_graph_handle->sub_node_tab);
	free(sub_graph_handle->src_fast);
	free(sub_graph_handle->dst_fast);
	free(sub_graph_handle);
}

/* ===================================================================== */
/* Possible assignment routines 										 */
/* ===================================================================== */

static struct possibleAssignment* possibleAssignment_create(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint8_t* error){
	struct edgeLocator{
		uint32_t 	offset;
		uint32_t 	size;
		uint32_t* 	mapping;
	};

	uint32_t 					nb_possible_assignment;
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	struct searchArgument 		srch_arg;
	uint32_t* 					entry;
	struct possibleAssignment* 	possible_assignment;
	struct edgeLocator* 		edge_loc_buffer;
	char* 						bld_ptr;
	int32_t(*func)(const void*, const void*, void*key);

	*error = 0;

	srch_arg.sub_node_tab 	= sub_graph_handle->sub_node_tab;
	srch_arg.node_tab 		= graph_handle->node_tab;
	srch_arg.edge_tab[0] 	= sub_graph_handle->edge_tab;
	srch_arg.edge_tab[1] 	= graph_handle->edge_tab;

	if ((edge_loc_buffer = (struct edgeLocator*)malloc(sizeof(struct edgeLocator) * subGraphIsoHandle_get_nb_edge(sub_graph_handle))) == NULL){
		log_err("unable to allocate memory");
		*error = 1;
		return NULL;
	}

	for	(i = 0, nb_possible_assignment = 0; i < subGraphIsoHandle_get_nb_edge(sub_graph_handle); i++){
		if (subGraphIsoHandle_edge_get_src(sub_graph_handle, i).label == SUBGRAPHISOMORPHISM_JOKER_LABEL){
			edge_loc_buffer[i].mapping = graph_handle->dst_edge_mapping;
			func = search_edgeTab_dst;
		}
		else{
			edge_loc_buffer[i].mapping = graph_handle->src_edge_mapping;
			func = search_edgeTab_src;
		}

		if ((entry = (uint32_t*)bsearch_r(sub_graph_handle->edge_tab + i, edge_loc_buffer[i].mapping, graphIsoHandle_get_nb_edge(graph_handle), sizeof(uint32_t), func, &srch_arg)) == NULL){
			free(edge_loc_buffer);
			return NULL;
		}

		for (j = entry - edge_loc_buffer[i].mapping; j > 0; j--){
			if (func(sub_graph_handle->edge_tab + i, edge_loc_buffer[i].mapping + j - 1, &srch_arg)){
				break;
			}
		}

		edge_loc_buffer[i].offset = j;

		for (j = (entry - edge_loc_buffer[i].mapping) + 1; j < graphIsoHandle_get_nb_edge(graph_handle); j++){
			if (func(sub_graph_handle->edge_tab + i, edge_loc_buffer[i].mapping + j, &srch_arg)){
				break;
			}
		}

		edge_loc_buffer[i].size = j - edge_loc_buffer[i].offset;
		nb_possible_assignment += edge_loc_buffer[i].size;
	}

	if ((possible_assignment = (struct possibleAssignment*)malloc(possibleAssignment_get_size(subGraphIsoHandle_get_nb_node(sub_graph_handle), subGraphIsoHandle_get_nb_edge(sub_graph_handle), nb_possible_assignment))) == NULL){
		log_err("unable to allocate memory");
		free(edge_loc_buffer);
		*error = 1;
		return NULL;
	}

	possible_assignment->nb_node 			= subGraphIsoHandle_get_nb_node(sub_graph_handle);
	possible_assignment->nb_edge 			= subGraphIsoHandle_get_nb_edge(sub_graph_handle);

	bld_ptr = (void*)(possible_assignment + 1);

	possible_assignment->node_meta_buffer 	= (struct nodeMeta*)bld_ptr;
	bld_ptr = (void*)(possible_assignment->node_meta_buffer + subGraphIsoHandle_get_nb_node(sub_graph_handle));

	possible_assignment->edge_meta_buffer 	= (struct edgeMeta*)bld_ptr;
	bld_ptr = (void*)(possible_assignment->edge_meta_buffer + subGraphIsoHandle_get_nb_edge(sub_graph_handle));

	possible_assignment->assignment_buffer 	= (uint32_t*)bld_ptr;
	bld_ptr = (void*)(possible_assignment->assignment_buffer + nb_possible_assignment);

	possible_assignment->save_size 			= (uint32_t*)bld_ptr;
	bld_ptr = (void*)(possible_assignment->save_size + subGraphIsoHandle_get_nb_edge(sub_graph_handle) * subGraphIsoHandle_get_nb_edge(sub_graph_handle));

	#if SUBGRAPHISOMORPHISM_OPTIM_SORT == 1
	possible_assignment->order 				= (uint32_t*)bld_ptr;

	for (i = 0; i < subGraphIsoHandle_get_nb_edge(sub_graph_handle); i++){
		possible_assignment->order[i] = i;
	}
	#endif

// 		#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
// 		possible_assignment->seed = 1;
// 		#endif

	for	(i = 0, k = 0; i < subGraphIsoHandle_get_nb_edge(sub_graph_handle); i++, k += j){
		possible_assignment->edge_meta_buffer[i].offset = k;
		possible_assignment->edge_meta_buffer[i].size 	= edge_loc_buffer[i].size;

		for (j = 0; j < edge_loc_buffer[i].size; j++){
			possible_assignment->assignment_buffer[k + j] = edge_loc_buffer[i].mapping[edge_loc_buffer[i].offset + j];
		}
	}

	free(edge_loc_buffer);

	return possible_assignment;
}

#if SUBGRAPHISOMORPHISM_OPTIM_LAZY_DUP == 1

static int32_t possibleAssignment_duplicate_lazy(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value){
	uint32_t i;

	for (i = 0; i < nb_assignment; i++){
		if (possibleAssignment_assignment(possible_assignment, i, 0) == new_value){
			return -1;
		}

		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].src != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src){
			if (graphIsoHandle_edge_get_src(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_src(graph_handle, new_value).node){
				return -1;
			}
		}
		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].dst != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src){
			if (graphIsoHandle_edge_get_dst(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_src(graph_handle, new_value).node){
				return -1;
			}
		}
		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].src != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst){
			if (graphIsoHandle_edge_get_src(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_dst(graph_handle, new_value).node){
				return -1;
			}
		}
		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].dst != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst){
			if (graphIsoHandle_edge_get_dst(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_dst(graph_handle, new_value).node){
				return -1;
			}
		}
	}

	for (i = nb_assignment + 1; i < possible_assignment->nb_edge; i++){
		possibleAssignment_size(possible_assignment, i) = possibleAssignment_save_size(possible_assignment, nb_assignment, i);
	}

	return 0;
}

#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1

static int32_t possibleAssignment_duplicate_light_lazy(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value){
	uint32_t i;

	for (i = 0; i < nb_assignment; i++){
		if (possibleAssignment_assignment(possible_assignment, i, 0) == new_value){
			return -1;
		}

		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].src != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src){
			if (graphIsoHandle_edge_get_src(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_src(graph_handle, new_value).node){
				return -1;
			}
		}
		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].dst != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src){
			if (graphIsoHandle_edge_get_dst(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_src(graph_handle, new_value).node){
				return -1;
			}
		}
		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].src != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst){
			if (graphIsoHandle_edge_get_src(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_dst(graph_handle, new_value).node){
				return -1;
			}
		}
		if (sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].dst != sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst){
			if (graphIsoHandle_edge_get_dst(graph_handle, possibleAssignment_assignment(possible_assignment, i, 0)).node == graphIsoHandle_edge_get_dst(graph_handle, new_value).node){
				return -1;
			}
		}
	}

	return 0;
}

#endif

#else

static int32_t possibleAssignment_duplicate(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		size;
	uint32_t 		offset;
	struct node* 	node_new_src;
	struct node* 	node_new_dst;
	struct node* 	node_possible_src;
	struct node* 	node_possible_dst;
	uint32_t 		idx_new_src;
	uint32_t 		idx_new_dst;
	uint32_t 		idx_possible_src;
	uint32_t 		idx_possible_dst;
	uint32_t 		tmp;

	node_new_src = graphIsoHandle_edge_get_src(graph_handle, new_value).node;
	node_new_dst = graphIsoHandle_edge_get_dst(graph_handle, new_value).node;

	idx_new_src = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src;
	idx_new_dst = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst;

	for (i = nb_assignment + 1; i < possible_assignment->nb_edge; i++){
		size = possibleAssignment_save_size(possible_assignment, nb_assignment, i);
		offset = possibleAssignment_offset(possible_assignment, i);

		idx_possible_src = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].src;
		idx_possible_dst = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].dst;

		for (j = 0; j < size; ){
			if (possible_assignment->assignment_buffer[offset + j] == new_value){
				goto remove;
			}

			node_possible_src = graphIsoHandle_edge_get_src(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;
			node_possible_dst = graphIsoHandle_edge_get_dst(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;

			if (node_new_src == node_possible_src && idx_new_src != idx_possible_src){
				goto remove;
			}
			else if (node_new_src == node_possible_dst && idx_new_src != idx_possible_dst){
				goto remove;
			}
			else if (node_new_dst == node_possible_src && idx_new_dst != idx_possible_src){
				goto remove;
			}
			else if (node_new_dst == node_possible_dst && idx_new_dst != idx_possible_dst){
				goto remove;
			}

			j++;

			continue;

			remove:

			if (!(--size)){
				return -1;
			}

			tmp = possible_assignment->assignment_buffer[offset + j];
			possible_assignment->assignment_buffer[offset + j] = possible_assignment->assignment_buffer[offset + size];
			possible_assignment->assignment_buffer[offset + size] = tmp;
		}
		possibleAssignment_size(possible_assignment, i) = size;
	}

	return 0;
}

#if SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP == 1

static int32_t possibleAssignment_duplicate_light(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, uint32_t nb_assignment, uint32_t new_value){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		size;
	uint32_t 		offset;
	struct node* 	node_new_src;
	struct node* 	node_new_dst;
	struct node* 	node_possible_src;
	struct node* 	node_possible_dst;
	uint32_t 		idx_new_src;
	uint32_t 		idx_new_dst;
	uint32_t 		idx_possible_src;
	uint32_t 		idx_possible_dst;
	uint32_t 		tmp;

	node_new_src = graphIsoHandle_edge_get_src(graph_handle, new_value).node;
	node_new_dst = graphIsoHandle_edge_get_dst(graph_handle, new_value).node;

	idx_new_src = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src;
	idx_new_dst = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst;

	for (i = nb_assignment + 1; i < possible_assignment->nb_edge; i++){
		size = possibleAssignment_size(possible_assignment, i);
		offset = possibleAssignment_offset(possible_assignment, i);

		idx_possible_src = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].src;
		idx_possible_dst = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, i)].dst;

		for (j = 0; j < size; ){
			if (possible_assignment->assignment_buffer[offset + j] == new_value){
				goto remove;
			}

			node_possible_src = graphIsoHandle_edge_get_src(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;
			node_possible_dst = graphIsoHandle_edge_get_dst(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;

			if (node_new_src == node_possible_src && idx_new_src != idx_possible_src){
				goto remove;
			}
			else if (node_new_src == node_possible_dst && idx_new_src != idx_possible_dst){
				goto remove;
			}
			else if (node_new_dst == node_possible_src && idx_new_dst != idx_possible_src){
				goto remove;
			}
			else if (node_new_dst == node_possible_dst && idx_new_dst != idx_possible_dst){
				goto remove;
			}

			j++;

			continue;

			remove:

			if (!(--size)){
				return -1;
			}

			tmp = possible_assignment->assignment_buffer[offset + j];
			possible_assignment->assignment_buffer[offset + j] = possible_assignment->assignment_buffer[offset + size];
			possible_assignment->assignment_buffer[offset + size] = tmp;
		}
		possibleAssignment_size(possible_assignment, i) = size;
	}

	return 0;
}

#endif

#endif

static void possibleAssignment_save(struct possibleAssignment* possible_assignment, uint32_t nb_assignment){
	uint32_t i;

	for (i = nb_assignment + 1; i < possible_assignment->nb_edge; i++){
		possibleAssignment_save_size(possible_assignment, nb_assignment, i) = possibleAssignment_size(possible_assignment, i);
	}
}

#define __generic_fill_restart_buffer_first__() 																													\
	if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].src].restart_ctr < restart_ctr){ 													\
		possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].src].restart_ctr = restart_ctr; 													\
		restart_idx_in[nb_restart_in ++] = sub_graph_handle->edge_tab[edge1].src; 																					\
	} 																																								\
	if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].dst].restart_ctr < restart_ctr){ 													\
		possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].dst].restart_ctr = restart_ctr; 													\
		restart_idx_in[nb_restart_in ++] = sub_graph_handle->edge_tab[edge1].dst; 																					\
	}

#define __generic_update_first__() 																																	\
	if (m == possible_assignment->edge_meta_buffer[edge2].size){ 																									\
		uint32_t __off__; 																																			\
																																									\
		if (!(--possible_assignment->edge_meta_buffer[edge1].size)){ 																								\
			return -1; 																																				\
		} 																																							\
																																									\
		__off__ = possible_assignment->edge_meta_buffer[edge1].offset; 																								\
		possible_assignment->assignment_buffer[__off__ + l] = possible_assignment->assignment_buffer[__off__ + possible_assignment->edge_meta_buffer[edge1].size]; 	\
																																									\
		__generic_fill_restart_buffer_first__() 																													\
	} 																																								\
	else{ 																																							\
		l ++; 																																						\
	}

static int32_t possibleAssignment_global_update_first(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	k;
	uint32_t 	l;
	uint32_t 	m;
	uint32_t 	edge1;
	uint32_t 	edge2;
	uint32_t 	assign1;
	uint32_t 	assign2;
	uint32_t 	nb_restart_in;
	uint32_t 	nb_restart_ou;
	uint32_t 	restart_ctr;
	uint32_t* 	restart_idx_in;
	uint32_t* 	restart_idx_ou;
	uint32_t* 	restart_idx_tp;

	restart_idx_in = (uint32_t*)alloca(sizeof(uint32_t) * subGraphIsoHandle_get_nb_node(sub_graph_handle) * 2);
	restart_idx_ou = restart_idx_in + subGraphIsoHandle_get_nb_node(sub_graph_handle);

	for (i = 0; i < subGraphIsoHandle_get_nb_node(sub_graph_handle); i++){
		possible_assignment->node_meta_buffer[i].restart_ctr 	= 0;
		#if SUBGRAPHISOMORPHISM_OPTIM_MARK_ASSIGNED == 1
		possible_assignment->node_meta_buffer[i].assigned 		= 0xffffffff;
		#endif
		restart_idx_in[i] = i;
	}

	for (restart_ctr = 1, nb_restart_in = subGraphIsoHandle_get_nb_node(sub_graph_handle); nb_restart_in; restart_ctr++){
		nb_restart_ou = nb_restart_in;
		nb_restart_in = 0;

		restart_idx_tp = restart_idx_in;
		restart_idx_in = restart_idx_ou;
		restart_idx_ou = restart_idx_tp;

		for (i = 0; i < nb_restart_ou; i++){
			for (j = 0; j < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; j++){
				edge1 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + j];

				/* SRC - SRC */
				if (sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src > 1){
					for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; k++){
						if (j == k){
							continue;
						}
						edge2 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + k];

						for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
							assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].src;

							for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
								assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].src;

								if (assign1 == assign2){
									break;
								}
							}

							__generic_update_first__()
						}
					}
				}

				/* SRC - DST */
				for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; k++){
					edge2 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + k];

					for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
						assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].src;

						for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
							assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].dst;

							if (assign1 == assign2){
								break;
							}
						}

						__generic_update_first__()
					}
				}
			}

			for (j = 0; j < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; j++){
				edge1 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + j];

				/* DST - SRC */
				for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; k++){
					edge2 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + k];

					for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
						assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].dst;

						for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
							assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].src;

							if (assign1 == assign2){
								break;
							}
						}

						__generic_update_first__()
					}
				}

				/* DST - DST */
				if (sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst > 1){
					for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; k++){
						if (j == k){
							continue;
						}
						edge2 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + k];

						for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
							assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].dst;

							for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
								assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].dst;

								if (assign1 == assign2){
									break;
								}
							}

							__generic_update_first__()
						}
					}
				}
			}
		}
	}

	possibleAssignment_compact(possible_assignment);

	return 0;
}

#define __generic_fill_restart_buffer__() 																															\
	if (!possibleAssignment_is_assigned(possible_assignment, sub_graph_handle->edge_tab[edge1].src)){ 																\
		if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].src].restart_ctr < restart_ctr){ 												\
			possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].src].restart_ctr = restart_ctr; 												\
			restart_idx_in[nb_restart_in ++] = sub_graph_handle->edge_tab[edge1].src; 																				\
		} 																																							\
	} 																																								\
	if (!possibleAssignment_is_assigned(possible_assignment, sub_graph_handle->edge_tab[edge1].dst)){ 																\
		if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].dst].restart_ctr < restart_ctr){ 												\
			possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].dst].restart_ctr = restart_ctr; 												\
			restart_idx_in[nb_restart_in ++] = sub_graph_handle->edge_tab[edge1].dst; 																				\
		} 																																							\
	}

#define __generic_update__() 																																		\
	if (m == possible_assignment->edge_meta_buffer[edge2].size){ 																									\
		uint32_t __off__; 																																			\
		uint32_t __tmp__; 																																			\
																																									\
		if (!(--possible_assignment->edge_meta_buffer[edge1].size)){ 																								\
			return -1; 																																				\
		} 																																							\
																																									\
		__off__ = possible_assignment->edge_meta_buffer[edge1].offset; 																								\
		__tmp__ = possible_assignment->assignment_buffer[__off__ + l]; 																								\
		possible_assignment->assignment_buffer[__off__ + l] = possible_assignment->assignment_buffer[__off__ + possible_assignment->edge_meta_buffer[edge1].size]; 	\
		possible_assignment->assignment_buffer[__off__ + possible_assignment->edge_meta_buffer[edge1].size] = __tmp__; 												\
																																									\
		__generic_fill_restart_buffer__() 																															\
	} 																																								\
	else{ 																																							\
		l ++; 																																						\
	}

#if SUBGRAPHISOMORPHISM_OPTIM_GLOBAL == 1

static int32_t possibleAssignment_global_update(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, void* assignment, uint32_t nb_assignment, uint32_t new_value){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	k;
	uint32_t 	l;
	uint32_t 	m;
	uint32_t 	edge1;
	uint32_t 	edge2;
	uint32_t 	assign1;
	uint32_t 	assign2;
	uint32_t 	nb_restart_in;
	uint32_t 	nb_restart_ou;
	uint32_t 	restart_ctr;
	uint32_t* 	restart_idx_in;
	uint32_t* 	restart_idx_ou;
	uint32_t* 	restart_idx_tp;

	#if SUBGRAPHISOMORPHISM_OPTIM_MARK_ASSIGNED == 1
	uint32_t 	nb_node_assigned;

	for (i = 0, nb_node_assigned = 0; i < subGraphIsoHandle_get_nb_node(sub_graph_handle); i++){
		if (possible_assignment->node_meta_buffer[i].assigned >= nb_assignment){
			possible_assignment->node_meta_buffer[i].restart_ctr = 0;
			possible_assignment->node_meta_buffer[i].assigned = 0xffffffff;
		}
		else{
			nb_node_assigned ++;
		}
	}

	if (nb_node_assigned == subGraphIsoHandle_get_nb_node(sub_graph_handle)){
		return 0;
	}

	restart_idx_in = (uint32_t*)alloca(sizeof(uint32_t) * (subGraphIsoHandle_get_nb_node(sub_graph_handle) - nb_node_assigned) * 2);
	restart_idx_ou = restart_idx_in + (subGraphIsoHandle_get_nb_node(sub_graph_handle) - nb_node_assigned);

	{
		uint32_t __index__;

		nb_restart_in = 0;

		__index__ = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src;
		if (!possibleAssignment_is_assigned(possible_assignment, __index__)){
			restart_idx_in[nb_restart_in ++] = __index__;
			assignment_node(assignment, __index__) = graphIsoHandle_edge_get_src(graph_handle, new_value).node;
			possible_assignment->node_meta_buffer[__index__].assigned = nb_assignment;
		}

		__index__ = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst;
		if (!possibleAssignment_is_assigned(possible_assignment, __index__)){
			restart_idx_in[nb_restart_in ++] = __index__;
			assignment_node(assignment, __index__) = graphIsoHandle_edge_get_dst(graph_handle, new_value).node;
			possible_assignment->node_meta_buffer[__index__].assigned = nb_assignment;
		}
	}

	#else

	for (i = 0; i < subGraphIsoHandle_get_nb_node(sub_graph_handle); i++){
		possible_assignment->node_meta_buffer[i].restart_ctr = 0;
	}

	restart_idx_in = (uint32_t*)alloca(sizeof(uint32_t) * subGraphIsoHandle_get_nb_node(sub_graph_handle) * 2);
	restart_idx_ou = restart_idx_in + subGraphIsoHandle_get_nb_node(sub_graph_handle);

	restart_idx_in[0] = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src;
	restart_idx_in[1] = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst;
	nb_restart_in = 2;

	#endif

	for (restart_ctr = 1; nb_restart_in; restart_ctr++){
		nb_restart_ou = nb_restart_in;
		nb_restart_in = 0;

		restart_idx_tp = restart_idx_in;
		restart_idx_in = restart_idx_ou;
		restart_idx_ou = restart_idx_tp;

		for (i = 0; i < nb_restart_ou; i++){
			for (j = 0; j < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; j++){
				edge1 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + j];

				/* SRC - SRC */
				if (sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src > 1){
					for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; k++){
						if (j == k){
							continue;
						}
						edge2 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + k];

						for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
							assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].src;

							for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
								assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].src;

								if (assign1 == assign2){
									break;
								}
							}

							__generic_update__()
						}
					}
				}

				/* SRC - DST */
				for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; k++){
					edge2 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + k];

					for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
						assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].src;

						for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
							assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].dst;

							if (assign1 == assign2){
								break;
							}
						}

						__generic_update__()
					}
				}
			}

			for (j = 0; j < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; j++){
				edge1 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + j];

				/* DST - SRC */
				for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; k++){
					edge2 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + k];

					for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
						assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].dst;

						for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
							assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].src;

							if (assign1 == assign2){
								break;
							}
						}

						__generic_update__()
					}
				}

				/* DST - DST */
				if (sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst > 1){
					for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; k++){
						if (j == k){
							continue;
						}
						edge2 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + k];

						for (l = 0; l < possible_assignment->edge_meta_buffer[edge1].size; ){
							assign1 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge1].offset + l]].dst;

							for (m = 0; m < possible_assignment->edge_meta_buffer[edge2].size; m++){
								assign2 = graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge2].offset + m]].dst;

								if (assign1 == assign2){
									break;
								}
							}

							__generic_update__()
						}
					}
				}
			}
		}
	}

	assignment_edge(assignment, subGraphIsoHandle_get_nb_node(sub_graph_handle), possibleAssignment_order(possible_assignment, nb_assignment)) = graph_handle->edge_tab[new_value].edge;

	#if SUBGRAPHISOMORPHISM_OPTIM_MARK_ASSIGNED != 1
	assignment_node(assignment, sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src) = graphIsoHandle_edge_get_src(graph_handle, new_value).node;
	assignment_node(assignment, sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst) = graphIsoHandle_edge_get_dst(graph_handle, new_value).node;
	#endif

	return 0;
}

#else

static int32_t possibleAssignment_local_update(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, void* assignment, uint32_t nb_assignment, uint32_t new_value){
	uint32_t i;
	uint32_t j;
	uint32_t src;
	uint32_t dst;
	uint32_t it_edge;
	uint32_t size;
	uint32_t tmp;

	src = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src;
	dst = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst;

	/* SRC - SRC */
	for (i = 0; i < sub_graph_handle->sub_node_tab[src].nb_src; i++){
		it_edge = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[src].off_src + i];

		size = possible_assignment->edge_meta_buffer[it_edge].size;
		for (j = 0; j < size; ){
			if (graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j]].src != graph_handle->edge_tab[new_value].src){
				size --;
				tmp = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size] = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j] = tmp;
			}
			else{
				j ++;
			}
		}

		if (!size){
			return -1;
		}

		possible_assignment->edge_meta_buffer[it_edge].size = size;
	}

	/* SRC - DST */
	for (i = 0; i < sub_graph_handle->sub_node_tab[src].nb_dst; i++){
		it_edge = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[src].off_dst + i];

		size = possible_assignment->edge_meta_buffer[it_edge].size;
		for (j = 0; j < size; ){
			if (graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j]].dst != graph_handle->edge_tab[new_value].src){
				size --;
				tmp = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size] = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j] = tmp;
			}
			else{
				j ++;
			}
		}

		if (!size){
			return -1;
		}

		possible_assignment->edge_meta_buffer[it_edge].size = size;
	}

	/* DST - SRC */
	for (i = 0; i < sub_graph_handle->sub_node_tab[dst].nb_src; i++){
		it_edge = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[dst].off_src + i];

		size = possible_assignment->edge_meta_buffer[it_edge].size;
		for (j = 0; j < size; ){
			if (graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j]].src != graph_handle->edge_tab[new_value].dst){
				size --;
				tmp = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size] = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j] = tmp;
			}
			else{
				j ++;
			}
		}

		if (!size){
			return -1;
		}

		possible_assignment->edge_meta_buffer[it_edge].size = size;
	}

	/* DST - DST */
	for (i = 0; i < sub_graph_handle->sub_node_tab[dst].nb_dst; i++){
		it_edge = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[dst].off_dst + i];

		size = possible_assignment->edge_meta_buffer[it_edge].size;
		for (j = 0; j < size; ){
			if (graph_handle->edge_tab[possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j]].dst != graph_handle->edge_tab[new_value].dst){
				size --;
				tmp = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + size] = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j];
				possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[it_edge].offset + j] = tmp;
			}
			else{
				j ++;
			}
		}

		if (!size){
			return -1;
		}

		possible_assignment->edge_meta_buffer[it_edge].size = size;
	}

	assignment_edge(assignment, subGraphIsoHandle_get_nb_node(sub_graph_handle), possibleAssignment_order(possible_assignment, nb_assignment)) = graph_handle->edge_tab[new_value].edge;
	assignment_node(assignment, src) = graphIsoHandle_edge_get_src(graph_handle, new_value).node;
	assignment_node(assignment, dst) = graphIsoHandle_edge_get_dst(graph_handle, new_value).node;

	return 0;
}

#endif

#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT == 1
static void possibleAssignment_compact(struct possibleAssignment* possible_assignment){
	uint32_t i;
	uint32_t j;

	for (i = 1, j = possible_assignment->edge_meta_buffer[0].size; i < possible_assignment->nb_edge; j += possible_assignment->edge_meta_buffer[i].size, i++){
		memmove(possible_assignment->assignment_buffer + j, possible_assignment->assignment_buffer + possible_assignment->edge_meta_buffer[i].offset, sizeof(uint32_t) * possible_assignment->edge_meta_buffer[i].size);
		possible_assignment->edge_meta_buffer[i].offset = j;
	}
}
#endif

// #if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
// static void possibleAssignment_mark_collision_layer_src_dst(uint64_t seed, struct node* node_layer){
// 	struct edge* 	edge_cursor;
// 	uint64_t* 		data;

// 	data = (uint64_t*)node_get_data(node_layer);
// 	if (*data == seed){
// 		return;
// 	}

// 	*data = seed;

// 	for (edge_cursor = node_get_head_edge_dst(node_layer); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
// 		possibleAssignment_mark_collision_layer_dst(seed, edge_get_src(edge_cursor));
// 	}
// 	for (edge_cursor = node_get_head_edge_src(node_layer); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
// 		possibleAssignment_mark_collision_layer_src_dst(seed, edge_get_dst(edge_cursor));
// 	}

// }

// static void possibleAssignment_mark_collision_layer_dst(uint64_t seed, struct node* node_layer){
// 	struct edge* 	edge_cursor;
// 	uint64_t* 		data;

// 	data = (uint64_t*)node_get_data(node_layer);
// 	if (*data == seed){
// 		return;
// 	}

// 	*data = seed;

// 	for (edge_cursor = node_get_head_edge_dst(node_layer); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
// 		possibleAssignment_mark_collision_layer_dst(seed, edge_get_src(edge_cursor));
// 	}
// }
// #endif


// /* ===================================================================== */
// /* Compare routines 													 */
// /* ===================================================================== */

static void* bsearch_r(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*, void*), void* arg){
	int32_t result;
	size_t 	up;
	size_t 	down;
	size_t 	index;

	for (down = 0, up = nmemb; down < up; ){
		index = (down + up) / 2;
		result = compar(key, (const char*)base + index * size, arg);
		if (result < 0){
			up = index;
		}
		else if (result > 0){
			down = index + 1;
		}
		else{
			return (char*)base + index * size;
		}
	}

	return NULL;
}

static int32_t compare_edgeTab_src(const void* arg1, const void* arg2, void* arg){
	uint32_t i1 					= *(const uint32_t*)arg1;
	uint32_t i2 					= *(const uint32_t*)arg2;
	struct compareArgument* cmp_arg = (struct compareArgument*)arg;

	if (cmp_arg->edge_tab[0][i1].label < cmp_arg->edge_tab[1][i2].label){
		return -1;
	}
	else if (cmp_arg->edge_tab[0][i1].label > cmp_arg->edge_tab[1][i2].label){
		return 1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].src].label < cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].src].label){
		return -1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].src].label > cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].src].label){
		return 1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].dst].label < cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].dst].label){
		return -1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].dst].label > cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].dst].label){
		return 1;
	}

	return 0;
}

static int32_t compare_edgeTab_dst(const void* arg1, const void* arg2, void* arg){
	uint32_t i1 					= *(const uint32_t*)arg1;
	uint32_t i2 					= *(const uint32_t*)arg2;
	struct compareArgument* cmp_arg = (struct compareArgument*)arg;

	if (cmp_arg->edge_tab[0][i1].label < cmp_arg->edge_tab[1][i2].label){
		return -1;
	}
	else if (cmp_arg->edge_tab[0][i1].label > cmp_arg->edge_tab[1][i2].label){
		return 1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].dst].label < cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].dst].label){
		return -1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].dst].label > cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].dst].label){
		return 1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].src].label < cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].src].label){
		return -1;
	}
	else if (cmp_arg->node_tab[0][cmp_arg->edge_tab[0][i1].src].label > cmp_arg->node_tab[1][cmp_arg->edge_tab[1][i2].src].label){
		return 1;
	}

	return 0;
}

static int32_t search_edgeTab_src(const void* key, const void* el, void* arg){
	const struct edgeTab* 	edge_tab 	= (const struct edgeTab*)key;
	uint32_t 				i 			= *(const uint32_t*)el;
	struct searchArgument* 	srch_arg 	= (struct searchArgument*)arg;

	if (edge_tab->label < srch_arg->edge_tab[1][i].label){
		return -1;
	}
	else if (edge_tab->label > srch_arg->edge_tab[1][i].label){
		return 1;
	}

	if (srch_arg->sub_node_tab[edge_tab->src].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
		if (srch_arg->sub_node_tab[edge_tab->src].label < srch_arg->node_tab[srch_arg->edge_tab[1][i].src].label){
			return -1;
		}
		else if (srch_arg->sub_node_tab[edge_tab->src].label > srch_arg->node_tab[srch_arg->edge_tab[1][i].src].label){
			return 1;
		}
	}

	if (srch_arg->sub_node_tab[edge_tab->dst].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
		if (srch_arg->sub_node_tab[edge_tab->dst].label < srch_arg->node_tab[srch_arg->edge_tab[1][i].dst].label){
			return -1;
		}
		else if (srch_arg->sub_node_tab[edge_tab->dst].label > srch_arg->node_tab[srch_arg->edge_tab[1][i].dst].label){
			return 1;
		}
	}

	return 0;
}

static int32_t search_edgeTab_dst(const void* key, const void* el, void* arg){
	const struct edgeTab* 	edge_tab 	= (const struct edgeTab*)key;
	uint32_t 				i 			= *(const uint32_t*)el;
	struct searchArgument* 	srch_arg 	= (struct searchArgument*)arg;

	if (edge_tab->label < srch_arg->edge_tab[1][i].label){
		return -1;
	}
	else if (edge_tab->label > srch_arg->edge_tab[1][i].label){
		return 1;
	}

	if (srch_arg->sub_node_tab[edge_tab->dst].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
		if (srch_arg->sub_node_tab[edge_tab->dst].label < srch_arg->node_tab[srch_arg->edge_tab[1][i].dst].label){
			return -1;
		}
		else if (srch_arg->sub_node_tab[edge_tab->dst].label > srch_arg->node_tab[srch_arg->edge_tab[1][i].dst].label){
			return 1;
		}
	}

	if (srch_arg->sub_node_tab[edge_tab->src].label != SUBGRAPHISOMORPHISM_JOKER_LABEL){
		if (srch_arg->sub_node_tab[edge_tab->src].label < srch_arg->node_tab[srch_arg->edge_tab[1][i].src].label){
			return -1;
		}
		else if (srch_arg->sub_node_tab[edge_tab->src].label > srch_arg->node_tab[srch_arg->edge_tab[1][i].src].label){
			return 1;
		}
	}

	return 0;
}

static int32_t compare_fast_src(const void* arg1, const void* arg2, void* arg){
	uint32_t 		i1 = *(const uint32_t*)arg1;
	uint32_t 		i2 = *(const uint32_t*)arg2;
	struct edgeTab* edge_tab = (struct edgeTab*)arg;

	if (edge_tab[i1].src < edge_tab[i2].src){
		return -1;
	}
	if (edge_tab[i1].src > edge_tab[i2].src){
		return 1;
	}
	return 0;
}

static int32_t compare_fast_dst(const void* arg1, const void* arg2, void* arg){
	uint32_t 		i1 = *(const uint32_t*)arg1;
	uint32_t 		i2 = *(const uint32_t*)arg2;
	struct edgeTab* edge_tab = (struct edgeTab*)arg;

	if (edge_tab[i1].dst < edge_tab[i2].dst){
		return -1;
	}
	if (edge_tab[i1].dst > edge_tab[i2].dst){
		return 1;
	}
	return 0;
}
