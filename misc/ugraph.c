#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ugraph.h"

struct ugraph* ugraph_create(uint32_t node_size){
	struct ugraph* ugraph;

	ugraph = (struct ugraph*)malloc(sizeof(struct ugraph));
	if (ugraph != NULL){
		ugraph_init(ugraph, node_size);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return ugraph;
}

struct unode* ugraph_add_node_(struct ugraph* ugraph){
	struct unode* unode;

	unode = (struct unode*)malloc(unode_get_size(ugraph));
	if (unode != NULL){
		unode_init(unode);

		if (ugraph->head != NULL){
			unode->next = ugraph->head;
			unode->next->prev = unode;
		}
		else{
			ugraph->tail = unode;
		}
		ugraph->head = unode;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return unode;
}

struct unode* ugraph_add_node(struct ugraph* ugraph, void* data){
	struct unode* unode;

	unode = ugraph_add_node_(ugraph);
	if (unode != NULL){
		memcpy(unode->data, data, ugraph->node_size);
	}
	else{
		printf("ERROR: in %s, unable to create unode\n", __func__);
	}

	return unode;
}

int32_t ugraph_add_edge(struct unode* unode1, struct unode* unode2){
	if (set_add(&(unode1->edge_set), &unode2)){
		printf("ERROR: in %s, unable to add element to set\n", __func__);
		return -1;
	}

	if (set_add(&(unode2->edge_set), &unode1)){
		printf("ERROR: in %s, unable to add element to set\n", __func__);
		set_remove(&(unode1->edge_set), &unode2);
		return -1;
	}

	return 0;
}

int32_t ugraph_add_unique_edge(struct unode* unode1, struct unode* unode2){
	if (set_get_length(&(unode1->edge_set)) > set_get_length(&(unode2->edge_set))){
		if (set_search(&(unode2->edge_set), &unode1) < 0){
			return ugraph_add_edge(unode1, unode2);
		}
		else{
			return 0; 
		}
	}
	else{
		if (set_search(&(unode1->edge_set), &unode2) < 0){
			return ugraph_add_edge(unode1, unode2);
		}
		else{
			return 0; 
		}
	}
}

void ugraph_remove_node(struct ugraph* ugraph, struct unode* unode){
	struct setIterator 	iterator;
	struct unode** 		link_ptr;
	struct unode* 		link;

	for (link_ptr = (struct unode**)setIterator_get_first(&(unode->edge_set), &iterator); link_ptr != NULL; link_ptr = (struct unode**)setIterator_get_next(&iterator)){
		link = *link_ptr;
		set_remove(&(link->edge_set), &unode);
	}

	if (unode->next != NULL){
		unode->next->prev = unode->prev;
	}
	else{
		ugraph->tail = unode->prev;
	}

	if (unode->prev != NULL){
		unode->prev->next = unode->next;
	}
	else{
		ugraph->head = unode->next;
	}

	unode_delete(unode);
}

void ugraph_clean(struct ugraph* ugraph){
	struct unode* unode_cusor;
	struct unode* unode_delete;

	for (unode_cusor = ugraph->head; unode_cusor != NULL; ){
		unode_delete = unode_cusor;
		unode_cusor = unode_cusor->next;
		unode_delete(unode_delete);
	}
}