#ifndef GRAPHNODE_H
#define GRAPHNODE_H

struct graphNode{
	void* 						data
	unsigned long				entry_point
	unsigned long				exit_point;
};

struct graphNode_callback{
	void*(*create_data)(void* first_element);
	void(*delete_data)(void* data);
};

int graphNode_init(struct graphNode* node, struct graphNode_callback* callback, void* element);


#endif