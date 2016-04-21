#ifndef WHITELIST_H
#define WHITELIST_H

struct whiteList{
	char*	buffer;
	int 	nb_entry;
	char** 	entries;
};

struct whiteList* whiteList_create(const char* file_name);
int whiteList_init(struct whiteList* list, const char* file_name);
int whiteList_search(struct whiteList* list, const char* entry);
void whiteList_print(struct whiteList* list);
void whiteList_clean(struct whiteList* list);
void whiteList_delete(struct whiteList* list);


#endif