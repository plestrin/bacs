#ifndef GRAPHNODE_H
#define GRAPHNODE_H

struct graphNode{
	void* 						data;
	unsigned long				entry_point;
	unsigned long				exit_point;
};

struct graphNode_callback{
	void*(*create_data)(void* first_element);								/* Create the data for  a new node, taking first element as argument 												DYNAMIC 	*/
	int(*may_add_element)(void* data, void* element);						/* Can the given element be added to this node?																		DYNAMIC		*/
	int(*add_element)(void* data, void* element);							/* Add an element to the node data - "may_add_element" has been run previously 										DYNAMIC		*/
	int(*element_is_owned)(void* data, void* element);						/* Does the given element belong to the node data? 																	STATIC 		*/
	int(*may_leave)(void* data);											/* Can the node be leaved without splitting it?																		DYNAMIC	 	*/
	int(*split_leave)(void* data_orig, void** data_new);					/* Split the current node data due to an early leaving - "may_leave" has been run previously						DYNAMIC 	*/
	int(*split_enter)(void* data_orig, void** data_new, void* element);		/* Split the current node data due to late enter - "may_add_element" & "element_is_owned" has been run previously 	DYNAMIC 	*/
	int(*get_nb_execution)(void* data);										/* Return the number of execution for the node 																		STATIC 		*/
	void(*delete_data)(void* data);											/* Delete the node's data 																							DYNAMIC		*/
};

int graphNode_init(struct graphNode* node, struct graphNode_callback* callback, void* element);
int graphNode_may_add_element(struct graphNode* node, struct graphNode_callback* callback, void* element);
int graphNode_add_element(struct graphNode* node, struct graphNode_callback* callback, void* element);
int graphNode_element_is_owned(struct graphNode* node, struct graphNode_callback* callback, void* element);
int graphNode_may_leave(struct graphNode* node, struct graphNode_callback* callback);
int graphNode_split_leave(struct graphNode* node_orig, struct graphNode* node_new, int* nb_execution, struct graphNode_callback* callback);
int graphNode_split_enter(struct graphNode* node_orig, struct graphNode* node_new, int* nb_execution, struct graphNode_callback* callback, void* element);
void graphNode_clean(struct graphNode* node, struct graphNode_callback* callback);


static inline void graphNode_callback_init(struct graphNode_callback* callback){
	callback->create_data 		= NULL;
	callback->may_add_element 	= NULL;
	callback->add_element 		= NULL;
	callback->element_is_owned 	= NULL;
	callback->may_leave 		= NULL;
	callback->split_leave 		= NULL;
	callback->split_enter 		= NULL;
	callback->get_nb_execution 	= NULL;
	callback->delete_data 		= NULL;
}


#endif