#ifndef UGRAPH_H
#define UGRAPH_H

#include <stdint.h>

/* One can use a better allocator */
#define ugraph_allocate_edge(ugraph) 				malloc(sizeof(struct uedgeContainer) + (ugraph)->uedge_data_size)
#define ugraph_allocate_node(ugraph)				malloc(sizeof(struct unode) + (ugraph)->unode_data_size)
#define ugraph_free_edge(ugraph, uedge_container) 	free(uedge_container)
#define ugraph_free_node(ugraph, unode) 			free(unode)

struct uedge{
	struct unode* 			endp;
	struct uedge* 			prev;
	struct uedge* 			next;
	struct uedgeContainer* 	container;
};

#define uedge_get_endp(uedge_) 	((uedge_)->endp)
#define uedge_get_next(uedge) 	((uedge)->next)
#define uedge_get_prev(uedge) 	((uedge)->prev)
#define uedge_get_data(uedge_) 	uedgeContainer_get_data((uedge_)->container)

struct uedgeContainer{
	struct uedge uedge[2];
};

#define uedgeContainer_get_data(uedge_container) ((void*)((uedge_container) + 1))

struct unode{
	struct unode* 	prev;
	struct unode* 	next;
	uint32_t 		nb_edge;
	struct uedge*	uedge_linkedList;
};

#define unode_get_next(unode) 		((unode)->next)
#define unode_get_prev(unode) 		((unode)->prev)
#define unode_get_head_edge(unode) 	((unode)->uedge_linkedList)
#define unode_get_data(unode_) 		((void*)((struct unode*)(unode_) + 1))

struct ugraph{
	struct unode* 	unode_linkedList_head;
	struct unode* 	unode_linkedList_tail;
	size_t 			unode_data_size;
	size_t 			uedge_data_size;

	void(*dotPrint_node_data)(void*,FILE*,void*);
	void(*dotPrint_edge_data)(void*,FILE*,void*);

	void(*unode_clean_data)(struct unode*);
	void(*uedge_clean_data)(struct uedgeContainer*);
};

struct ugraph* ugraph_create(size_t unode_data_size, size_t uedge_data_size);

#define ugraph_init(ugraph, unode_data_size_, uedge_data_size_)													\
	(ugraph)->unode_linkedList_head 	= NULL; 																\
	(ugraph)->unode_linkedList_tail 	= NULL; 																\
	(ugraph)->unode_data_size 			= unode_data_size_; 													\
	(ugraph)->uedge_data_size 			= uedge_data_size_; 													\
	(ugraph)->dotPrint_node_data 		= NULL; 																\
	(ugraph)->dotPrint_edge_data 		= NULL; 																\
	(ugraph)->unode_clean_data 			= NULL; 																\
	(ugraph)->uedge_clean_data 			= NULL;

#define ugraph_register_dotPrint_callback(ugraph, node_data, edge_data) 										\
	(ugraph)->dotPrint_node_data = (void(*)(void*,FILE*,void*))(node_data); 									\
	(ugraph)->dotPrint_edge_data = (void(*)(void*,FILE*,void*))(edge_data); 									\

#define ugraph_register_node_clean_call_back(ugraph, callback) ((ugraph)->unode_clean_data = callback)
#define ugraph_register_edge_clean_call_back(ugraph, callback) ((ugraph)->uedge_clean_data = callback)

struct unode* ugraph_add_node_(struct ugraph* ugraph);
struct unode* ugraph_add_node(struct ugraph* ugraph, void* data);

struct uedgeContainer* ugraph_add_edge_(struct ugraph* ugraph, struct unode* unode1, struct unode* unode2);
struct uedgeContainer* ugraph_add_edge(struct ugraph* ugraph, struct unode* unode1, struct unode* unode2, void* data);

void ugraph_remove_edge(struct ugraph* ugraph, struct uedge* uedge);
void ugraph_remove_node(struct ugraph* ugraph, struct unode* unode);

#define ugraph_get_head_node(ugraph) 	((ugraph)->unode_linkedList_head)
#define ugraph_get_tail_node(ugraph) 	((ugraph)->unode_linkedList_tail)

#define ugraph_clean(ugraph) 																					\
	while ((ugraph)->unode_linkedList_head != NULL){ 															\
		ugraph_remove_node((ugraph), (ugraph)->unode_linkedList_head); 											\
	}

#define ugraph_delete(ugraph) 																					\
	ugraph_clean(ugraph); 																						\
	free(ugraph);

#endif
