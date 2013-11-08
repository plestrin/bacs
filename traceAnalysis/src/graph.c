#include <stdlib.h>
#include <stdio.h>

#include "graph.h"

static int graph_create_node(struct graph* graph);
static int graph_create_edge(struct graph* graph);

static int graph_search_edge(struct graph* graph, unsigned long src, unsigned long dst);

static int graph_add_edge(struct graph* graph);

static void graphBuilder_init(struct graphBuilder* builder);
static inline int graphBuilder_is_valid(struct graphBuilder* builder);
static inline void graphBuilder_move(struct graphBuilder*, int new_offset);


struct graph* graph_create(struct graphNode_callback* callback_node){
	struct graph* graph = (struct graph*)malloc(sizeof(struct graph));
	if (graph != NULL){
		graph->nodes = (struct graphNode*)malloc(sizeof(struct graphNode) * GRAPH_NODE_BATCH);
		if (graph->nodes != NULL){
			graph->nb_allocated_node = GRAPH_NODE_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to alloacte memory\n", __func__);
			graph->nb_allocated_node = 0;
		}

		graph->edges = (struct graphEdge*)malloc(sizeof(struct graphEdge) * GRAPH_EDGE_BATCH);
		if (graph->edges != NULL){
			graph->nb_allocated_edge = GRAPH_EDGE_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			graph->nb_allocated_edge = 0;
		}

		graph->nb_node = 0;
		graph->nb_edge = 0;
		graph->entry_point = 0;
		graph->exit_point = 0;
		graph->callback_node = callback_node;

		graphBuilder_init(&(graph->builder));
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return graph;
}

int graph_add_element(struct graph* graph, void* element){
	int status = -1;
	int node_offset;

	if (graph != NULL){
		if (graphBuilder_is_valid(&(graph->builder))){
			/*a completer */
		}
		else{
			node_offset = graph_create_node(graph);
			if (node_offset < 0){
				printf("ERROR: in %s, unable to create node\n", __func__);
			}
			else{
				status = graphNode_init(graph->nodes + node_offset, graph->callback_node, element);
				if (status){
					printf("ERROR: in %s, unable to init graph node\n", __func__);
				}
				else{
					graphBuilder_move(&(graph->builder), node_offset);
					graph_add_edge(graph);
				}
			}
		}
	}

	return status;
}

void graph_delete(struct graph* graph){
	if (graph != NULL){
		/* a completer */
	}
}

static int graph_create_node(struct graph* graph){
	int 				node_offset;
	struct graphNode* 	nodes;

	if (graph->nb_node == graph->nb_allocated_node){
		nodes = realloc(graph->nodes, (graph->nb_allocated_node + GRAPH_NODE_BATCH) * sizeof(struct graphNode));
		if (nodes != NULL){
			graph->nodes = nodes;
			graph->nb_allocated_node += GRAPH_NODE_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
	}
	else{
		node_offset = graph->nb_node;
		graph->nb_node ++;
	}

	return node_offset;
}

static int graph_create_edge(struct graph* graph){
	int 				edge_offset;
	struct graphNode* 	edges;

	if (graph->nb_edge == graph->nb_allocated_edge){
		edges = realloc(graph->edges, (graph->nb_allocated_edge + GRAPH_EDGE_BATCH) * sizeof(struct graphEdge));
		if (edges != NULL){
		 	graph->edges = edges;
			graph->nb_allocated_edge += GRAPH_EDGE_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
	}
	else{
		edge_offset = graph->nb_edge;
		graph->nb_edge ++;
	}

	return edge_offset;
}

static int graph_search_edge(struct graph* graph, unsigned long src, unsigned long dst){
	int result = -1;
	int e;

	for (e = 0; e graph->nb_edge; e++){
		if (graph->edges[e].src_id == src && graph->edges[e].dst_id == dst){
			result = e;
			break;
		}
	}

	return result;
}

static int graph_add_edge(struct graph* graph){
	int status = 0;
	int edge_offset;

	if (graph->builder.previous_node_offset < 0){
		graph->entry_point = graph->nodes[graph->builder.current_node_offset].id_entry;
	}
	else{
		unsigned long src = graph->nodes[graph->builder.previous_node_offset].id_exit;
		unsigned long dst = graph->nodes[graph->builder.current_node_offset].id_entry;

		edge_offset = graph_search_edge(graph, src, dst);
		if (edge_offset < 0){
			edge_offset = graph_create_edge(graph);
			if (edge_offset < 0){
				printf("ERROR: in %s, unable to create edge\n");
			}
			else{
				graphEdge_init(graph->edges + edge_offset, src, dst, 1);
			}
		}
		else{
			graphEdge_increment(graph->edges + edge_offset);
		}
	}

	return status;
}

/* ===================================================================== */
/* Graph Builder function(s) 	                                         */
/* ===================================================================== */


void graphBuilder_init(struct graphBuidler* builder){
	builder->current_node_offset = -1;
	builder->previous_node_builder = -1;
	/* a completer */
}

static inline int graphBuilder_is_valid(struct graphBuilder* builder){
	return (builder->current_node_offset >= 0);
}

static inline void graphBuilder_move(struct graphBuilder*, int new_offset){
	builder->previous_node_offset = builder->current_node_offset;
	builder->current_node_offset = new_offset;
}