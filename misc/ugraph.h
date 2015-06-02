#ifndef UGRAPH_H
#define UGRAPH_H

#include <stdint.h>

#include "set.h"

#define UGRAPH_NB_EDGE_PER_BLOCK 16

struct unode{
	struct unode* 	next;
	struct unode* 	prev;
	char* 			data;
	struct set 		edge_set;	
};

#define unode_get_size(ugraph) ((ugraph)->node_size + sizeof(struct unode) + set_get_size(sizeof(struct unode*), UGRAPH_NB_EDGE_PER_BLOCK) - sizeof(struct set))

#define unode_init(unode) 																																	\
	(unode)->next 		= NULL; 																															\
	(unode)->prev 		= NULL; 																															\
	(unode)->data 		= (char*)(unode) + (sizeof(struct unode) + set_get_size(sizeof(struct unode*), UGRAPH_NB_EDGE_PER_BLOCK) - sizeof(struct set)); 	\
	set_init(&((unode)->edge_set), sizeof(struct unode*), UGRAPH_NB_EDGE_PER_BLOCK);

#define unode_get_nb_edge(unode) ((unode)->edge_set.nb_element_tot)

#define unode_clean(unode) set_clean(&((unode)->edge_set))

#define unode_delete(unode) 																																\
	unode_clean(unode); 																																	\
	free(unode);
	
struct ugraph{
	struct unode* 	head;
	struct unode* 	tail;
	uint32_t 		node_size;
};

struct ugraph* ugraph_create(uint32_t node_size);

#define ugraph_init(ugraph, node_size_) 																													\
	(ugraph)->head 		= NULL; 																															\
	(ugraph)->tail 		= NULL; 																															\
	(ugraph)->node_size = node_size_;

struct unode* ugraph_add_node_(struct ugraph* ugraph);
struct unode* ugraph_add_node(struct ugraph* ugraph, void* data);
int32_t ugraph_add_edge(struct unode* unode1, struct unode* unode2);

#define ugraph_remove_edge(unode1, unode2) 																													\
	set_remove(&(unode1->edge_set), &unode2); 																												\
	set_remove(&(unode2->edge_set), &unode1);

void ugraph_remove_node(struct ugraph* ugraph, struct unode* unode);
void ugraph_clean(struct ugraph* ugraph);

#define ugraph_delete(ugraph) 																																\
	ugraph_clean(ugraph); 																																	\
	free(ugraph);

#endif 