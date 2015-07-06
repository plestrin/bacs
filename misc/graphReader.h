#ifndef GRAPHREADER_H
#define GRAPHREADER_H

#include <stdint.h>

struct graphReaderCallback{
	void(*callback_graph_name)(const char*,size_t,void*);
	void(*callback_graph_label)(const char*,size_t,void*);
	void(*callback_graph_end)(void*);
	void*(*callback_create_node)(void*);
	void(*callback_node_label)(const char*,size_t,void*,void*);
	void(*callback_node_io)(const char*,size_t,void*,void*);
	void*(*callback_create_edge)(void*,void*,void*);
	void(*callback_edge_label)(const char*,size_t,void*,void*);
	void* arg;
};

void graphReader_parse(const void* buffer, size_t buffer_size, struct graphReaderCallback* callback);

#endif