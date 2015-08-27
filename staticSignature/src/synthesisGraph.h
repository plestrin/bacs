#ifndef SYNTHESISGRAPH_H
#define SYNTHESISGRAPH_H

#include "ir.h"
#include "graph.h"
#include "graphPrintDot.h"
#include "array.h"
#include "result.h"

enum synthesisNodeType{
	SYNTHESISNODETYPE_RESULT,
	SYNTHESISNODETYPE_FORWARD_PATH,
	SYNTHESISNODETYPE_BACKWARD_PATH,
	SYNTHESISNODETYPE_IR_NODE
};

struct signatureCluster{
	struct node* 				synthesis_graph_node;
	uint32_t 					nb_in_parameter;
	uint32_t 					nb_ou_parameter;
	struct parameterMapping* 	parameter_mapping;
	struct array 				instance_array;
};

struct synthesisNode{
	enum synthesisNodeType 			type;
	union{
		struct signatureCluster* 	cluster;
		struct array* 				path;
		struct node*				ir_node;
	}								node_type;
	uint32_t 						index; 				/* used to compute the adjacency matrix */
} __attribute__((__may_alias__));

#define synthesisGraph_get_synthesisNode(node) ((struct synthesisNode*)&((node)->data))

#define SYNTHESISGRAPH_EGDE_TAG_RAW 0x00000000

#define synthesisGraph_get_edge_tag_input(index) 	(((index) & 0x3fffffff) | 0x80000000)
#define synthesisGraph_get_edge_tag_output(index) 	(((index) & 0x3fffffff) | 0xc0000000)
#define synthesisGraph_edge_is_input(tag)			(((tag) & 0xc0000000) == 0x80000000)
#define synthesisGraph_edge_is_output(tag) 			(((tag) & 0xc0000000) == 0xc0000000)
#define synthesisGraph_edge_get_parameter(tag) 		((tag) & 0x3fffffff)

struct synthesisGraph{
	struct graph 		graph;
	struct array 		cluster_array;
};

struct synthesisGraph* synthesisGraph_create(struct ir* ir);
int32_t synthesisGraph_init(struct synthesisGraph* synthesis_graph, struct ir* ir);

#define synthesisGraph_printDot(synthesis_graph) graphPrintDot_print(&(synthesis_graph->graph), NULL, NULL)

void synthesisGraph_delete_edge(struct graph* graph, struct edge* edge);

void synthesisGraph_clean(struct synthesisGraph* synthesis_graph);

#define synthesisGraph_delete(synthesis_graph) 								\
	synthesisGraph_clean(synthesis_graph); 									\
	free(synthesis_graph);

#endif