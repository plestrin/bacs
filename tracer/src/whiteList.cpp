#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "whiteList.h"

#ifdef __linux__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int whiteList_compare(const void * entry1, const void * entry2);


struct whiteList* whiteList_create(const char* file_name){
	int 				file;
	struct stat 		sb;
	struct whiteList* 	result;
	size_t				i;
	int 				n;


	result = (struct whiteList*)malloc(sizeof(struct whiteList));
	if (result == NULL){
		printf("ERROR: unable to allocate memory\n");
		return NULL;
	}

	file = open(file_name, O_RDONLY);
	if (file == -1){
		printf("ERROR: unable to open file: %s\n", file_name);
		free(result);
		return NULL;
	}

	if (fstat(file, &sb) < 0){
		printf("ERROR: unable to read file size\n");
		close(file);
		free(result);
		return NULL;
	}

	result->buffer = (char*)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, file, 0);
	if (result->buffer == NULL){
		printf("ERROR: unable to mmap memory\n");
		close(file);
		free(result);
		return NULL;
	}

	close(file);

	result->buffer_size = sb.st_size;
	result->nb_entry = 1;

	for (i = 0; i < result->buffer_size; i++){
		if (result->buffer[i] == '\n'){
			result->nb_entry ++;
			result->buffer[i] = '\0';
		}
	}

	result->entries = (char**)malloc(sizeof(char*)*result->nb_entry);
	if (result->entries == NULL){
		printf("ERROR: unable to allocate memory\n");
		munmap(result->buffer, result->buffer_size);
		free(result);
		return NULL;
	}

	result->entries[0] = result->buffer;
	for (n = 1, i = 0; n < result->nb_entry; n++){
		while ((i < result->buffer_size) && (result->buffer[i] != '\0')){
			i++;
		}
		if (i >= result->buffer_size){
			printf("ERROR: searching for end of line fails at line %d:%d\n", n, result->nb_entry);
			result->entries[n] = result->entries[n-1];
		}
		else{
			i++;
			result->entries[n] = result->buffer + i;
		}
	}

	qsort(result->entries, result->nb_entry, sizeof(char*), whiteList_compare);	

	return result;
}

int whiteList_search(struct whiteList* list, const char* entry){
	if (list != NULL){
		if (bsearch(&entry, list->entries, list->nb_entry, sizeof(char*), whiteList_compare) != NULL){
			return 0;
		}
		else{
			return -1;
		}
	}
	else{
		return -1;
	}
}

void whiteList_print(struct whiteList* list){
	int 						n;

	printf("*** PRINT WHITELIST ***\n");

	if (list != NULL){
		printf("Buffer size: \t%u\n", list->buffer_size);
		printf("Nb entrie(s): \t%d\n", list->nb_entry);

		for (n = 0; n < list->nb_entry; n++){
			printf("Entry %d: \"%s\"\n", n, list->entries[n]);
		}
	}
	else{
		printf("WhiteList pointer is NULL\n");
	}
}

void whiteList_delete(struct whiteList* list){
	if (list != NULL){
		if (list->entries != NULL){
			free(list->entries);
		}
		if (list->buffer != NULL && list->buffer_size > 0){
			munmap(list->buffer, list->buffer_size);
		}
		free(list);
	}
}

int whiteList_compare(const void* entry1, const void* entry2){
	return strcmp(*(char**)entry1, *(char**)entry2);
}


#endif