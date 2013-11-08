#include <stdlib.h>
#include <stdio.h>

#include "graphNode.h"

unsigned long id_generator = 0;

static inline unsigned long graphNode_generate_id(){
	id_generator ++;
	return id_generator - 1;
}

int graphNode_init(struct graphNode* node, struct graphNode_callback* callback, void* element){
	int status = 0;

	if (callback != NULL){
		if (callback->create_data != NULL){
			node->data = callback->create_data(element);
			if (node->data == NULL){
				printf("ERROR: in %s, create data callback returns NULL pointer\n", __func__);
				status = -1;
			}
		}
	}

	node->entry_point = graphNode_generate_id();
	node->exit_point = graphNode_generate_id();

	return status;
}
