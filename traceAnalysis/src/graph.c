#include <stdlib.h>
#include <stdio.h>

#include "graph.h"

static int graph_create_node(struct graph* graph);
static int graph_create_edge(struct graph* graph);

static int graph_search_node(struct graph* graph, void* element);
static int graph_search_edge(struct graph* graph, unsigned long src, unsigned long dst);

static int graph_add_edge(struct graph* graph);
static int graph_add_edge_split(struct graph* graph, unsigned int long src, unsigned long dst, int nb_execution);

static void graphBuilder_init(struct graphBuilder* builder);
static inline int graphBuilder_is_valid(struct graphBuilder* builder);
static inline void graphBuilder_move(struct graphBuilder* builder, int new_offset);


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
	int status 				= -1;
	int node_offset;
	int new_node_offset;
	int nb_edge_execution;

	if (graph != NULL){
		if (graphBuilder_is_valid(&(graph->builder))){
			if (graphNode_may_add_element(graph->nodes + graph->builder.current_node_offset, graph->callback_node, element) == 0){
				status = graphNode_add_element(graph->nodes + graph->builder.current_node_offset, graph->callback_node, element);
				if (status){
					printf("ERROR: in %s, unable to add element to current node\n", __func__);
				}
			}
			else{
				if (graphNode_may_leave(graph->nodes + graph->builder.current_node_offset, graph->callback_node) != 0){
					node_offset = graph_create_node(graph);
					if (node_offset < 0){
						printf("ERROR: in %s, unable to create node\n", __func__);
					}
					else{
						status = graphNode_split_leave(graph->nodes + graph->builder.current_node_offset, graph->nodes + node_offset, &(nb_edge_execution), graph->callback_node);
						if (status){
							printf("ERROR: in %s, unable to split node - early leaving\n", __func__);
						}
						else{
							status = graph_add_edge_split(graph, graph->nodes[graph->builder.current_node_offset].exit_point, graph->nodes[node_offset].entry_point, nb_edge_execution);
							if (status){
								printf("ERROR: in %s, unable to add edge after split\n", __func__);
							}
						}
					}
				}

				node_offset = graph_search_node(graph, element);
				if (node_offset < 0){
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
							status  = graph_add_edge(graph);
							if (status){
								printf("ERROR: in %s, unable to add edge\n", __func__);
							}
						}
					}
				}
				else{
					if (graphNode_may_add_element(graph->nodes + node_offset, graph->callback_node, element) == 0){
						status = graphNode_add_element(graph->nodes + node_offset, graph->callback_node, element);
						if (status){
							printf("ERROR: in %s, unable to add element to current node\n", __func__);
						}
						graphBuilder_move(&(graph->builder), node_offset);
						status = graph_add_edge(graph);
						if (status){
							printf("ERROR: in %s, unable to add edge\n", __func__);
						}
					}
					else{
						new_node_offset = graph_create_node(graph);
						if (new_node_offset < 0){
							printf("ERROR: in %s, unable to create node\n", __func__);
						}
						else{
							status = graphNode_split_enter(graph->nodes + node_offset, graph->nodes + new_node_offset, &nb_edge_execution, graph->callback_node, element);
							if (status){
								printf("ERROR: in %s, unable to split node - late entry\n", __func__);
							}
							else{
								status = graph_add_edge_split(graph, graph->nodes[node_offset].exit_point, graph->nodes[new_node_offset].entry_point, nb_edge_execution);
								if (status){
									printf("ERROR: in %s, unable to add edge after split\n", __func__);
								}
								else{
									status = graphNode_add_element(graph->nodes + new_node_offset, graph->callback_node, element);
									if (status){
										printf("ERROR: in %s, unable to add element to current node\n", __func__);
									}
									graphBuilder_move(&(graph->builder), node_offset);
									status = graph_add_edge(graph);
									if (status){
										printf("ERROR: in %s, unable to add edge\n", __func__);
									}
								}
							}
						}
					}
				}
			}
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
					status = graph_add_edge(graph);
					if (status){
						printf("ERROR: in %s, unable to add edge\n", __func__);
					}
				}
			}
		}
	}

	return status;
}

void graph_delete(struct graph* graph){
	int n;

	if (graph != NULL){
		for (n = 0; n < graph->nb_node; n++){
			graphNode_clean(graph->nodes + n, graph->callback_node);
		}

		free(graph->nodes);
		free(graph->edges);

		free(graph);
	}
}

static int graph_create_node(struct graph* graph){
	int 				node_offset;
	struct graphNode* 	nodes;

	if (graph->nb_node == graph->nb_allocated_node){
		nodes = (struct graphNode*)realloc(graph->nodes, (graph->nb_allocated_node + GRAPH_NODE_BATCH) * sizeof(struct graphNode));
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
	struct graphEdge* 	edges;

	if (graph->nb_edge == graph->nb_allocated_edge){
		edges = (struct graphEdge*)realloc(graph->edges, (graph->nb_allocated_edge + GRAPH_EDGE_BATCH) * sizeof(struct graphEdge));
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

static int graph_search_node(struct graph* graph, void* element){
	int result = -1;
	int n;

	for (n = 0; n < graph->nb_node; n++){
		if (graphNode_element_is_owned(graph->nodes + n, graph->callback_node, element) == 0){
			result = n;
			break;
		}
	}

	return result;
}

static int graph_search_edge(struct graph* graph, unsigned long src, unsigned long dst){
	int result = -1;
	int e;

	for (e = 0; e < graph->nb_edge; e++){
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
		graph->entry_point = graph->nodes[graph->builder.current_node_offset].entry_point;
	}
	else{
		unsigned long src = graph->nodes[graph->builder.previous_node_offset].exit_point;
		unsigned long dst = graph->nodes[graph->builder.current_node_offset].entry_point;

		edge_offset = graph_search_edge(graph, src, dst);
		if (edge_offset < 0){
			edge_offset = graph_create_edge(graph);
			if (edge_offset < 0){
				printf("ERROR: in %s, unable to create edge\n", __func__);
			}
			else{
				graphEdge_init(graph->edges + edge_offset, src, dst, 1);
			}
		}
		else{
			graphEdge_increment(graph->edges + edge_offset, 1);
		}
	}

	return status;
}

static int graph_add_edge_split(struct graph* graph, unsigned int long src, unsigned long dst, int nb_execution){
	int status = 0;
	int edge_offset;

	edge_offset = graph_search_edge(graph, src, dst);
	if (edge_offset < 0){
		edge_offset = graph_create_edge(graph);
		if (edge_offset < 0){
			printf("ERROR: in %s, unable to create edge\n", __func__);
		}
		else{
			graphEdge_init(graph->edges + edge_offset, src, dst, nb_execution);
		}
	}
	else{
		graphEdge_increment(graph->edges + edge_offset, nb_execution);
	}

	return status;
}

/* ===================================================================== */
/* Graph Builder function(s) 	                                         */
/* ===================================================================== */


void graphBuilder_init(struct graphBuilder* builder){
	builder->current_node_offset = -1;
	builder->previous_node_offset = -1;
}

static inline int graphBuilder_is_valid(struct graphBuilder* builder){
	return (builder->current_node_offset >= 0);
}

static inline void graphBuilder_move(struct graphBuilder* builder, int new_offset){
	builder->previous_node_offset = builder->current_node_offset;
	builder->current_node_offset = new_offset;
}