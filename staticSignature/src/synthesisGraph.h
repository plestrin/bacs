#ifndef SYNTHESISGRAPH_H
#define SYNTHESISGRAPH_H

#include "ir.h"
#include "graph.h"
#include "graphPrintDot.h"
#include "array.h"
#include "result.h"

enum synthesisNodeType{
	SYNTHESISNODETYPE_RESULT,
	SYNTHESISNODETYPE_PATH,
	SYNTHESISNODETYPE_IR_NODE
};

struct signatureCluster{
	struct node* 				synthesis_graph_node;
	struct symbolMapping* 		symbol_mapping;
	struct array 				instance_array;
};

struct synthesisNode{
	enum synthesisNodeType 			type;
	union{
		struct signatureCluster* 	cluster;
		struct{
			uint32_t 				nb_node;
			struct node** 			node_buffer;
		} 							path;
		struct node*				ir_node;
	}								node_type;
};

struct synthesisEdge{
	uint32_t 						tag;
};

#define synthesisGraph_get_synthesisNode(node) ((struct synthesisNode*)node_get_data(node))
#define synthesisGraph_get_synthesisEdge(edge) ((struct synthesisEdge*)edge_get_data(edge))

void synthesisGraph_printDot_node(void* data, FILE* file, void* arg);
void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg);

/* Bit map description of the edgeTag (there is not anymore the fragment notion)
	- [0 :29] 	parameter index
	- [30] 		1 for input and 0 for output
	- [31] 		1 for a paremter and 0 for a raw edge
*/

#define SYNTHESISEGDE_TAG_RAW 0x00000000

#define synthesisEdge_get_tag_input(index) 		(((index) & 0x3fffffff) | 0xc0000000)
#define synthesisEdge_get_tag_output(index) 	(((index) & 0x3fffffff) | 0x80000000)
#define synthesisEdge_is_input(tag)				(((tag) & 0xc0000000) == 0xc0000000)
#define synthesisEdge_is_output(tag) 			(((tag) & 0xc0000000) == 0x80000000)
#define synthesisEdge_get_parameter(tag) 		((tag) & 0x3fffffff)

struct synthesisGraph{
	struct graph 		graph;
	struct array 		cluster_array;
};

struct synthesisGraph* synthesisGraph_create(struct ir* ir);
int32_t synthesisGraph_init(struct synthesisGraph* synthesis_graph, struct ir* ir);

#define synthesisGraph_printDot(synthesis_graph, name) graphPrintDot_print(&(synthesis_graph->graph), name, NULL)

void synthesisGraph_delete_edge(struct graph* graph, struct edge* edge);

void synthesisGraph_clean(struct synthesisGraph* synthesis_graph);

#define synthesisGraph_delete(synthesis_graph) 								\
	synthesisGraph_clean(synthesis_graph); 									\
	free(synthesis_graph);

#endif