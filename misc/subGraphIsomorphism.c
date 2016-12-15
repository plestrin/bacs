#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "subGraphIsomorphism.h"
#include "base.h"

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
	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	uint32_t 			seed;
	#endif
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

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
static void assignment_init_layerNodeData(struct edge* edge){
	struct unode* layer_node;

	if (edge->ptr != NULL){
		layer_node = (struct unode*)edge->ptr;
		if (!layer_node->nb_edge){
			edge->ptr = NULL; /* Ideally one should remove this layer_node from the graphLayer */
		}
		else{
			unode_get_layerNodeData(layer_node)->selected = 0xffffffff;
			unode_get_layerNodeData(layer_node)->disabled = 0xffffffff;
		}
	}
}

static void assignment_mark_collision(struct edge* edge, uint32_t seed){
	struct layerNodeData* 	data;
	struct uedge* 			edge_cursor;

	if (edge->ptr != NULL){
		data = unode_get_layerNodeData(edge->ptr);
		if (data->selected > seed){
			data->selected = seed;
			data->disabled = 0xffffffff;

			for (edge_cursor = unode_get_head_edge((struct unode*)edge->ptr); edge_cursor != NULL; edge_cursor = uedge_get_next(edge_cursor)){
				data = unode_get_layerNodeData(uedge_get_endp(edge_cursor));
				if (data->disabled > seed){
					data->disabled = seed;
				}
			}
		}
	}
}

static inline int32_t assignment_is_collision(struct edge* edge, uint32_t seed){
	struct layerNodeData* data;

	if (edge->ptr != NULL){
		data = unode_get_layerNodeData(edge->ptr);

		if (data->disabled <= seed){
			return 1;
		}
		data->disabled = 0xffffffff;
		if (data->selected > seed){
			data->selected = 0xffffffff;
		}
	}

	return 0;
}

void graphIso_prepare_layer(struct graph* graph){
	struct node* node_cursor;
	struct edge* edge_cursor;

	for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			edge_cursor->ptr = NULL;
		}
	}
}

void graphIso_check_layer(struct ugraph* graph_layer){
	struct unode* 	unode_cursor;
	struct uedge* 	uedge_cursor1;
	struct uedge* 	uedge_cursor2;
	uint32_t 		nb_local_edge;

	for (unode_cursor = ugraph_get_head_node(graph_layer); unode_cursor != NULL; unode_cursor = unode_get_next(unode_cursor)){
		for (uedge_cursor1 = unode_get_head_edge(unode_cursor), nb_local_edge = 0; uedge_cursor1 != NULL; uedge_cursor1 = uedge_get_next(uedge_cursor1)){
			nb_local_edge ++;

			if (((struct layerNodeData*)unode_get_data(unode_cursor))->type == LAYERNODE_TYPE_PRIM && ((struct layerNodeData*)unode_get_data(uedge_get_endp(uedge_cursor1)))->type == LAYERNODE_TYPE_PRIM){
				log_err("incorrect edge from PRIM to PRIM");
				continue;
			}

			for (uedge_cursor2 = uedge_get_next(uedge_cursor1); uedge_cursor2 != NULL; uedge_cursor2 = uedge_get_next(uedge_cursor2)){
				if (uedge_get_endp(uedge_cursor1) == uedge_get_endp(uedge_cursor2)){
					log_err("two uedges point to the same endpoint");
				}
			}

			#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1
			if (unode_get_layerNodeData(unode_cursor)->type == LAYERNODE_TYPE_PRIM){
				uint32_t 				i;
				struct layerNodeData* 	data;

				for (i = 0, data = unode_get_layerNodeData(unode_cursor); i < data->nb_edge; i++){
					if (data->edge_buffer[i]->ptr != unode_cursor){
						log_err("edge is in unode buffer but its ptr attribute does not point to unode");
					}
				}
			}
			#endif
		}

		if (nb_local_edge != unode_cursor->nb_edge){
			log_err("incorrect number of edges");
		}
	}
}

void layerNodeData_printDot(void* data, FILE* file){
	struct layerNodeData* layer_node_data = data;

	switch (layer_node_data->type){
		case LAYERNODE_TYPE_PRIM 	: {
			#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1
			fprintf(file, "[label=\"p:%u\"]", layer_node_data->nb_edge);
			#else
			fprintf(file, "[label=\"p\"]");
			#endif
			break;
		}
		case LAYERNODE_TYPE_MACRO 	: {
			fprintf(file, "[label=\"m\"]");
			break;
		}
	}
}

#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1

void layerNodeData_clean_unode(struct unode* unode){
	struct layerNodeData* data;

	data = unode_get_layerNodeData(unode);
	if (data->type == LAYERNODE_TYPE_PRIM && data->edge_buffer != NULL){
		free(data->edge_buffer);
	}
}

#endif

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
		/* check that every node has been assigned - later */
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

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	possible_assignment->seed += size;
	#endif

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	for (i = 0; i < size; i++, possible_assignment->seed--)
	#else
	for (i = 0; i < size; i++)
	#endif
	{
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

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	possible_assignment->seed = 1;
	#endif

	for	(i = 0, k = 0; i < subGraphIsoHandle_get_nb_edge(sub_graph_handle); i++, k += j){
		possible_assignment->edge_meta_buffer[i].offset = k;
		possible_assignment->edge_meta_buffer[i].size 	= edge_loc_buffer[i].size;

		for (j = 0; j < edge_loc_buffer[i].size; j++){
			possible_assignment->assignment_buffer[k + j] = edge_loc_buffer[i].mapping[edge_loc_buffer[i].offset + j];
		}
	}

	free(edge_loc_buffer);

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	for	(i = 0; i < graphIsoHandle_get_nb_edge(graph_handle); i++){
		assignment_init_layerNodeData(graph_handle->edge_tab[i].edge);
	}
	#endif

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


#define __generic_remove__() 																								\
	{ 																														\
		uint32_t __tmp__; 																									\
																															\
		if (!(--size)){ 																									\
			return -1; 																										\
		} 																													\
																															\
		__tmp__ = possible_assignment->assignment_buffer[offset + j]; 														\
		possible_assignment->assignment_buffer[offset + j] = possible_assignment->assignment_buffer[offset + size]; 		\
		possible_assignment->assignment_buffer[offset + size] = __tmp__; 													\
																															\
		continue; 																											\
	}

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

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	assignment_mark_collision(graph_handle->edge_tab[new_value].edge, possible_assignment->seed);
	#endif

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
				__generic_remove__()
			}

			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			if (assignment_is_collision(graph_handle->edge_tab[possible_assignment->assignment_buffer[offset + j]].edge, possible_assignment->seed)){
				__generic_remove__()
			}
			#endif

			node_possible_src = graphIsoHandle_edge_get_src(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;
			node_possible_dst = graphIsoHandle_edge_get_dst(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;

			if (node_new_src == node_possible_src && idx_new_src != idx_possible_src){
				__generic_remove__()
			}
			else if (node_new_src == node_possible_dst && idx_new_src != idx_possible_dst){
				__generic_remove__()
			}
			else if (node_new_dst == node_possible_src && idx_new_dst != idx_possible_src){
				__generic_remove__()
			}
			else if (node_new_dst == node_possible_dst && idx_new_dst != idx_possible_dst){
				__generic_remove__()
			}

			j++;
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

	#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
	assignment_mark_collision(graph_handle->edge_tab[new_value].edge, possible_assignment->seed);
	#endif

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
				__generic_remove__()
			}

			#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
			if (assignment_is_collision(graph_handle->edge_tab[possible_assignment->assignment_buffer[offset + j]].edge, possible_assignment->seed)){
				__generic_remove__()
			}
			#endif

			node_possible_src = graphIsoHandle_edge_get_src(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;
			node_possible_dst = graphIsoHandle_edge_get_dst(graph_handle, possible_assignment->assignment_buffer[offset + j]).node;

			if (node_new_src == node_possible_src && idx_new_src != idx_possible_src){
				__generic_remove__()
			}
			else if (node_new_src == node_possible_dst && idx_new_src != idx_possible_dst){
				__generic_remove__()
			}
			else if (node_new_dst == node_possible_src && idx_new_dst != idx_possible_src){
				__generic_remove__()
			}
			else if (node_new_dst == node_possible_dst && idx_new_dst != idx_possible_dst){
				__generic_remove__()
			}

			j++;
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

static int32_t uint32_t_compare(const void* arg1, const void* arg2){
	return *(const uint32_t*)arg1 - *(const uint32_t*)arg2;
}

static uint32_t uint32_t_remove_duplicate(uint32_t* buffer, uint32_t buffer_size){
	uint32_t i;
	uint32_t j;

	for (i = 1, j = 1; i < buffer_size; i++){
		if (buffer[i - 1] != buffer[i]){
			buffer[j ++] = buffer[i];
		}
	}

	return j;
}

static uint32_t uint32_t_intersect_buffer(uint32_t* dst, uint32_t dst_size, const uint32_t* src, uint32_t src_size){
	uint32_t i;
	uint32_t j;
	uint32_t k;

	for (i = 0, j = 0, k = 0; i < dst_size && j < src_size; ){
		if ((int32_t)(dst[i] - src[j]) < 0){
			i++;
		}
		else if ((int32_t)(dst[i] - src[j]) > 0){
			j++;
		}
		else{
			dst[k ++] = dst[i];
			i ++;
			j ++;
		}
	}

	return k;
}

static uint32_t possibleAssignment_intersect_src_endpoint(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, uint32_t* buffer, uint32_t buffer_size, uint32_t edge){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t* 	local_buffer;
	uint32_t 	local_buffer_size;

	local_buffer_size = possible_assignment->edge_meta_buffer[edge].size;
	local_buffer = (uint32_t*)alloca(local_buffer_size * sizeof(uint32_t));

	for (i = 0, j = possible_assignment->edge_meta_buffer[edge].offset; i < local_buffer_size; i++, j++){
		local_buffer[i] = graph_handle->edge_tab[possible_assignment->assignment_buffer[j]].src;
	}

	qsort(local_buffer, local_buffer_size, sizeof(uint32_t), uint32_t_compare);

	return uint32_t_intersect_buffer(buffer, buffer_size, local_buffer, local_buffer_size);
}

static uint32_t possibleAssignment_intersect_dst_endpoint(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, uint32_t* buffer, uint32_t buffer_size, uint32_t edge){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t* 	local_buffer;
	uint32_t 	local_buffer_size;

	local_buffer_size = possible_assignment->edge_meta_buffer[edge].size;
	local_buffer = (uint32_t*)alloca(local_buffer_size * sizeof(uint32_t));

	for (i = 0, j = possible_assignment->edge_meta_buffer[edge].offset; i < local_buffer_size; i++, j++){
		local_buffer[i] = graph_handle->edge_tab[possible_assignment->assignment_buffer[j]].dst;
	}

	qsort(local_buffer, local_buffer_size, sizeof(uint32_t), uint32_t_compare);

	return uint32_t_intersect_buffer(buffer, buffer_size, local_buffer, local_buffer_size);
}

static uint32_t possibleAssignment_filter_based_on_src(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, uint32_t* buffer, uint32_t buffer_size, uint32_t edge){
	uint32_t i;
	uint32_t j;
	uint32_t size;

	for (i = 0, j = possible_assignment->edge_meta_buffer[edge].offset, size = possible_assignment->edge_meta_buffer[edge].size; i < size; ){
		if (bsearch(&(graph_handle->edge_tab[possible_assignment->assignment_buffer[j]].src), buffer, buffer_size, sizeof(uint32_t), uint32_t_compare) == NULL){
			size --;
			possible_assignment->assignment_buffer[j] = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge].offset + size];
		}
		else{
			i ++;
			j ++;
		}
	}

	return size;
}

static uint32_t possibleAssignment_filter_based_on_dst(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, uint32_t* buffer, uint32_t buffer_size, uint32_t edge){
	uint32_t i;
	uint32_t j;
	uint32_t size;

	for (i = 0, j = possible_assignment->edge_meta_buffer[edge].offset, size = possible_assignment->edge_meta_buffer[edge].size; i < size; ){
		if (bsearch(&(graph_handle->edge_tab[possible_assignment->assignment_buffer[j]].dst), buffer, buffer_size, sizeof(uint32_t), uint32_t_compare) == NULL){
			size --;
			possible_assignment->assignment_buffer[j] = possible_assignment->assignment_buffer[possible_assignment->edge_meta_buffer[edge].offset + size];
		}
		else{
			i ++;
			j ++;
		}
	}

	return size;
}

static int32_t possibleAssignment_fast_compare_node(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle, int32_t index, uint32_t* restart_idx_in, uint32_t* nb_restart_in, uint32_t restart_ctr){
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	k;
	uint32_t* 	buffer;
	uint32_t 	buffer_size;
	uint32_t 	restart;
	uint32_t 	edge;
	uint32_t 	size;

	if (sub_graph_handle->sub_node_tab[index].nb_src + sub_graph_handle->sub_node_tab[index].nb_dst == 1){
		return 0;
	}

	for (i = 0, buffer = NULL; i < sub_graph_handle->sub_node_tab[index].nb_src; i++){
		edge = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[index].off_src + i];

		if (buffer == NULL){
			buffer_size = possible_assignment->edge_meta_buffer[edge].size;
			buffer = (uint32_t*)alloca(buffer_size * sizeof(uint32_t));

			for (j = 0, k = possible_assignment->edge_meta_buffer[edge].offset; j < buffer_size; j++, k++){
				buffer[j] = graph_handle->edge_tab[possible_assignment->assignment_buffer[k]].src;
			}

			qsort(buffer, buffer_size, sizeof(uint32_t), uint32_t_compare);
			buffer_size = uint32_t_remove_duplicate(buffer, buffer_size);
		}
		else{
			if ((buffer_size = possibleAssignment_intersect_src_endpoint(possible_assignment, graph_handle, buffer, buffer_size, edge)) == 0){
				return -1;
			}
		}
	}

	for (i = 0; i < sub_graph_handle->sub_node_tab[index].nb_dst; i++){
		edge = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[index].off_dst + i];

		if (buffer == NULL){
			buffer_size = possible_assignment->edge_meta_buffer[edge].size;
			buffer = (uint32_t*)alloca(buffer_size * sizeof(uint32_t));

			for (j = 0, k = possible_assignment->edge_meta_buffer[edge].offset; j < buffer_size; j++, k++){
				buffer[j] = graph_handle->edge_tab[possible_assignment->assignment_buffer[k]].dst;
			}

			qsort(buffer, buffer_size, sizeof(uint32_t), uint32_t_compare);
			buffer_size = uint32_t_remove_duplicate(buffer, buffer_size);
		}
		else{
			if ((buffer_size = possibleAssignment_intersect_dst_endpoint(possible_assignment, graph_handle, buffer, buffer_size, edge)) == 0){
				return -1;
			}
		}
	}

	for (i = 0, restart = 0; i < sub_graph_handle->sub_node_tab[index].nb_src; i++){
		edge = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[index].off_src + i];
		size = possibleAssignment_filter_based_on_src(possible_assignment, graph_handle, buffer, buffer_size, edge);

		if (size != possible_assignment->edge_meta_buffer[edge].size){
			possible_assignment->edge_meta_buffer[edge].size = size;
			restart = 1;

			if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge].dst].restart_ctr < restart_ctr && sub_graph_handle->sub_node_tab[sub_graph_handle->edge_tab[edge].dst].nb_src + sub_graph_handle->sub_node_tab[sub_graph_handle->edge_tab[edge].dst].nb_dst > 1){
				possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge].dst].restart_ctr = restart_ctr;
				restart_idx_in[*nb_restart_in] = sub_graph_handle->edge_tab[edge].dst;
				*nb_restart_in = *nb_restart_in + 1;
			}
		}
	}

	for (i = 0; i < sub_graph_handle->sub_node_tab[index].nb_dst; i++){
		edge = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[index].off_dst + i];
		size = possibleAssignment_filter_based_on_dst(possible_assignment, graph_handle, buffer, buffer_size, edge);

		if (size != possible_assignment->edge_meta_buffer[edge].size){
			possible_assignment->edge_meta_buffer[edge].size = size;
			restart = 1;

			if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge].src].restart_ctr < restart_ctr && sub_graph_handle->sub_node_tab[sub_graph_handle->edge_tab[edge].src].nb_src + sub_graph_handle->sub_node_tab[sub_graph_handle->edge_tab[edge].src].nb_dst > 1){
				possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge].src].restart_ctr = restart_ctr;
				restart_idx_in[*nb_restart_in] = sub_graph_handle->edge_tab[edge].src;
				*nb_restart_in = *nb_restart_in + 1;
			}
		}
	}

	if (restart && possible_assignment->node_meta_buffer[index].restart_ctr < restart_ctr){
		possible_assignment->node_meta_buffer[index].restart_ctr = restart_ctr;
		restart_idx_in[*nb_restart_in] = index;
		*nb_restart_in = *nb_restart_in + 1;
	}

	return 0;
}


static int32_t possibleAssignment_global_update_first(struct possibleAssignment* possible_assignment, struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle){
	uint32_t 	i;
	uint32_t 	nb_restart_in;
	uint32_t 	nb_restart_ou;
	uint32_t 	restart_ctr;
	uint32_t* 	restart_idx_root;
	uint32_t* 	restart_idx_in;
	uint32_t* 	restart_idx_ou;
	uint32_t* 	restart_idx_tp;

	if ((restart_idx_root = (uint32_t*)malloc(sizeof(uint32_t) * subGraphIsoHandle_get_nb_node(sub_graph_handle) * 2)) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	restart_idx_in = restart_idx_root;
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
			if (possibleAssignment_fast_compare_node(possible_assignment, graph_handle, sub_graph_handle, restart_idx_ou[i], restart_idx_in, &nb_restart_in, restart_ctr)){
				free(restart_idx_root);
				return -1;
			}
		}
	}

	free(restart_idx_root);
	possibleAssignment_compact(possible_assignment);

	return 0;
}

#define __generic_fill_restart_buffer__() 																										\
	if (size1 != possible_assignment->edge_meta_buffer[edge1].size){ 																			\
		possible_assignment->edge_meta_buffer[edge1].size = size1; 																				\
																																				\
		if (!possibleAssignment_is_assigned(possible_assignment, sub_graph_handle->edge_tab[edge1].src)){ 										\
			if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].src].restart_ctr < restart_ctr){ 						\
				possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].src].restart_ctr = restart_ctr; 						\
				restart_idx_in[nb_restart_in ++] = sub_graph_handle->edge_tab[edge1].src; 														\
			} 																																	\
		} 																																		\
		if (!possibleAssignment_is_assigned(possible_assignment, sub_graph_handle->edge_tab[edge1].dst)){ 										\
			if (possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].dst].restart_ctr < restart_ctr){ 						\
				possible_assignment->node_meta_buffer[sub_graph_handle->edge_tab[edge1].dst].restart_ctr = restart_ctr; 						\
				restart_idx_in[nb_restart_in ++] = sub_graph_handle->edge_tab[edge1].dst; 														\
			} 																																	\
		} 																																		\
	}

#define __generic_update__() 																													\
	if (m == possible_assignment->edge_meta_buffer[edge2].size){ 																				\
		uint32_t __off__; 																														\
		uint32_t __tmp__; 																														\
																																				\
		if (!(--size1)){ 																														\
			return -1; 																															\
		} 																																		\
																																				\
		__off__ = possible_assignment->edge_meta_buffer[edge1].offset; 																			\
		__tmp__ = possible_assignment->assignment_buffer[__off__ + l]; 																			\
		possible_assignment->assignment_buffer[__off__ + l] = possible_assignment->assignment_buffer[__off__ + size1]; 							\
		possible_assignment->assignment_buffer[__off__ + size1] = __tmp__; 																		\
	} 																																			\
	else{ 																																		\
		l ++; 																																	\
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
	uint32_t 	size1;
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
		assignment_edge(assignment, subGraphIsoHandle_get_nb_node(sub_graph_handle), possibleAssignment_order(possible_assignment, nb_assignment)) = graph_handle->edge_tab[new_value].edge;
		return 0;
	}

	restart_idx_in = (uint32_t*)alloca(sizeof(uint32_t) * (subGraphIsoHandle_get_nb_node(sub_graph_handle) - nb_node_assigned) * 2);
	restart_idx_ou = restart_idx_in + (subGraphIsoHandle_get_nb_node(sub_graph_handle) - nb_node_assigned);

	{
		uint32_t __index__;

		nb_restart_in = 0;

		__index__ = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].src;
		if (!possibleAssignment_is_assigned(possible_assignment, __index__)){
			if (sub_graph_handle->sub_node_tab[__index__].nb_src + sub_graph_handle->sub_node_tab[__index__].nb_dst > 1){
				restart_idx_in[nb_restart_in ++] = __index__;
			}
			assignment_node(assignment, __index__) = graphIsoHandle_edge_get_src(graph_handle, new_value).node;
			possible_assignment->node_meta_buffer[__index__].assigned = nb_assignment;
		}

		__index__ = sub_graph_handle->edge_tab[possibleAssignment_order(possible_assignment, nb_assignment)].dst;
		if (!possibleAssignment_is_assigned(possible_assignment, __index__)){
			if (sub_graph_handle->sub_node_tab[__index__].nb_src + sub_graph_handle->sub_node_tab[__index__].nb_dst > 1){
				restart_idx_in[nb_restart_in ++] = __index__;
			}
			assignment_node(assignment, __index__) = graphIsoHandle_edge_get_dst(graph_handle, new_value).node;
			possible_assignment->node_meta_buffer[__index__].assigned = nb_assignment;
		}
	}

	#else

	for (i = 0; i < subGraphIsoHandle_get_nb_node(sub_graph_handle); i++){
		possible_assignment->node_meta_buffer[i].restart_ctr = 0;
	}

	restart_idx_in = (uint32_t*)alloca(sizeof(uint32_t) * (subGraphIsoHandle_get_nb_node(sub_graph_handle) - nb_node_assigned) * 2);
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
				size1 = possible_assignment->edge_meta_buffer[edge1].size;

				/* SRC - SRC */
				if (sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src > 1){
					for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; k++){
						if (j == k){
							continue;
						}
						edge2 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + k];

						for (l = 0; l < size1; ){
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

					for (l = 0; l < size1; ){
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

				__generic_fill_restart_buffer__()
			}

			for (j = 0; j < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_dst; j++){
				edge1 = sub_graph_handle->dst_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_dst + j];
				size1 = possible_assignment->edge_meta_buffer[edge1].size;

				/* DST - SRC */
				for (k = 0; k < sub_graph_handle->sub_node_tab[restart_idx_ou[i]].nb_src; k++){
					edge2 = sub_graph_handle->src_fast[sub_graph_handle->sub_node_tab[restart_idx_ou[i]].off_src + k];

					for (l = 0; l < size1; ){
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

						for (l = 0; l < size1; ){
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

				__generic_fill_restart_buffer__()
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

/* ===================================================================== */
/* Compare routines 													 */
/* ===================================================================== */

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
