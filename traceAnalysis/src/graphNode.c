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

int graphNode_may_add_element(struct graphNode* node, struct graphNode_callback* callback, void* element){
	if (callback != NULL){
		if (callback->may_add_element != NULL){
			return callback->may_add_element(node->data, element);
		}
	}

	return 0;
}

int graphNode_add_element(struct graphNode* node, struct graphNode_callback* callback, void* element){
	if (callback != NULL){
		if (callback->add_element != NULL){
			return callback->add_element(node->data, element);
		}
	}

	return 0;
}

int graphNode_element_is_owned(struct graphNode* node, struct graphNode_callback* callback, void* element){
	if (callback != NULL){
		if (callback->element_is_owned != NULL){
			return callback->element_is_owned(node->data, element);
		}
	}

	return 0;
}

int graphNode_may_leave(struct graphNode* node, struct graphNode_callback* callback){
	if (callback != NULL){
		if (callback->may_leave != NULL){
			return callback->may_leave(node->data);
		}
	}

	return 0;
}

int graphNode_split_leave(struct graphNode* node_orig, struct graphNode* node_new, int* nb_execution, struct graphNode_callback* callback){
	int result = -1;

	if (callback != NULL){
		if (callback->split_leave != NULL){
			node_new->entry_point = graphNode_generate_id();
			node_new->exit_point = node_orig->exit_point;
			node_orig->exit_point = graphNode_generate_id();

			result = callback->split_leave(node_orig->data, &(node_new->data));
			if (callback->get_nb_execution != NULL){
				*nb_execution = callback->get_nb_execution(node_orig->data);
			}
			else{
				*nb_execution = 1;
			}
		}
	}

	return result;
}

int graphNode_split_enter(struct graphNode* node_orig, struct graphNode* node_new, int* nb_execution, struct graphNode_callback* callback, void* element){
	int result = -1;

	if (callback != NULL){
		if (callback->split_enter != NULL){
			node_new->entry_point = graphNode_generate_id();
			node_new->exit_point = node_orig->exit_point;
			node_orig->exit_point = graphNode_generate_id();

			result = callback->split_enter(node_orig->data, &(node_new->data), element);
			if (callback->get_nb_execution != NULL){
				*nb_execution = callback->get_nb_execution(node_orig->data);
			}
			else{
				*nb_execution = 1;
			}
		}
	}

	return result;
}

void graphNode_print_dot(struct graphNode* node, struct graphNode_callback* callback, FILE* file){
	if (callback != NULL){
		if (callback->print_dot != NULL){
			callback->print_dot(node->data, file);
		}
	}
}

void graphNode_clean(struct graphNode* node, struct graphNode_callback* callback){
	if (callback != NULL){
		if (callback->delete_data != NULL){
			callback->delete_data(node->data);
			node->data = NULL;
		}
	}
}
