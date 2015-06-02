#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../ugraph.h"

#define NODE_SIZE 23
#define NB_NODE 16
#define NB_ROUND 50

int32_t main(){
	struct ugraph 	ugraph;
	uint8_t* 		adj;
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	char 			data[NODE_SIZE] = {0};
	struct unode* 	node_buffer[NB_NODE];

	adj = (uint8_t*)calloc(NB_NODE * NB_NODE, sizeof(uint8_t));
	if (adj == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return 0;
	}

	ugraph_init(&ugraph, NODE_SIZE);

	for (i = 0; i < NB_NODE; i++){
		node_buffer[i] = ugraph_add_node(&ugraph, data);
		if (node_buffer[i] == NULL){
			printf("ERROR: in %s, unable to add unode %u\n", __func__, i);
		}
	}

	for (i = 0; i < NB_ROUND; i++){
		for (j = 0; j < NB_NODE; j++){
			if (node_buffer[j] == NULL){
				if (rand() & 0x1){
					node_buffer[j] = ugraph_add_node(&ugraph, data);
					if (node_buffer[j] == NULL){
						printf("ERROR: in %s, unable to add unode %u @ %u\n", __func__, j, i);
						continue;
					}
				}
			}
			else if (rand() & 0x1){
				ugraph_remove_node(&ugraph, node_buffer[j]);
				node_buffer[j] = NULL;

				for (k = j + 1; k < NB_NODE; k++){
					adj[j * NB_NODE + k] = 0;
				}

				for (k = 0; k < j; k++){
					adj[k * NB_NODE + j] = 0;
				}
			}
			else{
				for (k = j + 1; k < NB_NODE; k++){
					if (node_buffer[k] != NULL){
						if (adj[j * NB_NODE + k]){
							if (rand() & 0x01){
								ugraph_remove_edge(node_buffer[j], node_buffer[k]);
								adj[j * NB_NODE + k] = 0;
							}
						}
						else{
							if (rand() & 0x1){
								ugraph_add_edge(node_buffer[j], node_buffer[k]);
								adj[j * NB_NODE + k] = 1;
							}
						}
					}
				}
			}
		}
	}

	ugraph_clean(&ugraph);

	free(adj);

	return 0;
}