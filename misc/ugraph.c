#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ugraph.h"
#include "base.h"

/* One can implement special callback mecanisms here */
#define ugraph_node_set_data(ugraph, unode, data_) 				memcpy(unode_get_data(unode), (data_), (ugraph)->unode_data_size);
#define ugraph_edge_set_data(ugraph, uedge_container, data_) 	memcpy(uedgeContainer_get_data(uedge_container), (data_), (ugraph)->uedge_data_size);
#define ugraph_node_clean_data(ugraph, unode) 					if ((ugraph)->unode_clean_data != NULL){(ugraph)->unode_clean_data(unode);}
#define ugraph_edge_clean_data(ugraph, uedge_container) 		if ((ugraph)->uedge_clean_data != NULL){(ugraph)->uedge_clean_data(uedge_container);}

#define ugraph_connect_node(ugraph, unode) 														\
	(unode)->prev 				= NULL; 														\
	(unode)->next 				= (ugraph)->unode_linkedList_head; 								\
	(unode)->nb_edge 			= 0; 															\
	(unode)->uedge_linkedList 	= NULL; 														\
																								\
	if ((unode)->next != NULL){ 																\
		(unode)->next->prev = (unode); 															\
	} 																							\
	else{ 																						\
		(ugraph)->unode_linkedList_tail = (unode); 												\
	} 																							\
																								\
	(ugraph)->unode_linkedList_head = (unode);

#define ugraph_connect_edgeContainer(unode1, unode2, uedge_container) 							\
	(uedge_container)->uedge[0].container = (uedge_container); 									\
	ugraph_connect_edge(unode1, unode2, (uedge_container)->uedge + 0) 							\
 																								\
	(uedge_container)->uedge[1].container = (uedge_container); 									\
	ugraph_connect_edge(unode2, unode1, (uedge_container)->uedge + 1)

#define ugraph_connect_edge(unode1, unode2, uedge) 												\
	(uedge)->endp = (unode2); 																	\
	(uedge)->prev = NULL; 																		\
	(uedge)->next = (unode1)->uedge_linkedList; 												\
																								\
	if ((uedge)->next != NULL){ 																\
		(uedge)->next->prev = (uedge); 															\
	} 																							\
																								\
	(unode1)->nb_edge ++; 																		\
	(unode1)->uedge_linkedList = (uedge);

#define ugraph_disconnect_node(ugraph, unode) 													\
	if ((unode)->prev == NULL){ 																\
		(ugraph)->unode_linkedList_head = (unode)->next; 										\
	} 																							\
	else{ 																						\
		(unode)->prev->next = (unode)->next; 													\
	} 																							\
																								\
	if ((unode)->next == NULL){ 																\
		(ugraph)->unode_linkedList_tail = (unode)->prev; 										\
	} 																							\
	else{ 																						\
		(unode)->next->prev = (unode)->prev; 													\
	}

#define ugraph_disconnect_edgeContainer(uedge_container) 										\
	ugraph_disconnect_edge((uedge_container)->uedge[1].endp, (uedge_container)->uedge + 0) 		\
	ugraph_disconnect_edge((uedge_container)->uedge[0].endp, (uedge_container)->uedge + 1)

#define ugraph_disconnect_edge(unode, uedge) 													\
	(unode)->nb_edge --; 																		\
																								\
	if ((uedge)->prev != NULL){ 																\
		(uedge)->prev->next = (uedge)->next; 													\
	} 																							\
	else{ 																						\
		(unode)->uedge_linkedList = (uedge)->next; 												\
	} 																							\
																								\
	if ((uedge)->next != NULL){ 																\
		(uedge)->next->prev = (uedge)->prev; 													\
	}

struct ugraph* ugraph_create(size_t unode_data_size, size_t uedge_data_size){
	struct ugraph* ugraph;

	ugraph = (struct ugraph*)malloc(sizeof(struct ugraph));
	if (ugraph != NULL){
		ugraph_init(ugraph, unode_data_size, uedge_data_size);
	}
	else{
		log_err("unable to allocate memory");
	}

	return ugraph;
}

struct unode* ugraph_add_node_(struct ugraph* ugraph){
	struct unode* unode;

	if ((unode = ugraph_allocate_node(ugraph)) != NULL){
		ugraph_connect_node(ugraph, unode)
	}
	else{
		log_err("unable to allocate unode");
	}

	return unode;
}

struct unode* ugraph_add_node(struct ugraph* ugraph, void* data){
	struct unode* unode;

	if ((unode = ugraph_allocate_node(ugraph)) != NULL){
		ugraph_node_set_data(ugraph, unode, data)
		ugraph_connect_node(ugraph, unode)
	}
	else{
		log_err("unable to allocate unode");
	}

	return unode;
}

struct uedgeContainer* ugraph_add_edge_(struct ugraph* ugraph, struct unode* unode1, struct unode* unode2){
	struct uedgeContainer* uedge_container;

	if ((uedge_container = ugraph_allocate_edge(ugraph)) != NULL){
		ugraph_connect_edgeContainer(unode1, unode2, uedge_container)
	}
	else{
		log_err("unable to allocate uedge");
	}

	return uedge_container;
}

struct uedgeContainer* ugraph_add_edge(struct ugraph* ugraph, struct unode* unode1, struct unode* unode2, void* data){
	struct uedgeContainer* uedge_container;

	if ((uedge_container = ugraph_allocate_edge(ugraph)) != NULL){
		ugraph_edge_set_data(ugraph, uedge_container, data)
		ugraph_connect_edgeContainer(unode1, unode2, uedge_container)
	}
	else{
		log_err("unable to allocate uedge");
	}

	return uedge_container;
}

void ugraph_remove_edge(struct ugraph* ugraph, struct uedge* uedge){
	struct uedgeContainer* uedge_container;

	uedge_container = uedge->container;

	ugraph_disconnect_edgeContainer(uedge_container)

	ugraph_edge_clean_data(ugraph, uedge_container)

	ugraph_free_edge(ugraph, uedge_container);
}

void ugraph_remove_node(struct ugraph* ugraph, struct unode* unode){
	while (unode->uedge_linkedList != NULL){
		ugraph_remove_edge(ugraph, unode->uedge_linkedList);
	}

	ugraph_disconnect_node(ugraph, unode)

	ugraph_node_clean_data(ugraph, unode)

	ugraph_free_node(ugraph, unode);
}
