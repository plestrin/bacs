#ifndef WHITELIST_H
#define WHITELIST_H


#ifdef __linux__

struct whiteList{
	char*	buffer;
	size_t	buffer_size;
	int 	nb_entry;
	char** 	entries;
};

#endif

struct whiteList* whiteList_create(const char* file_name);
int whiteList_search(struct whiteList* list, const char* entry);
void whiteList_print(struct whiteList* list);
void whiteList_delete(struct whiteList* list);


#endif