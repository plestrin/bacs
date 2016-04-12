#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "whiteList.h"

int whiteList_compare(const void* entry1, const void* entry2);

#ifdef __linux__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int whiteList_init(struct whiteList* list, const char* file_name){
	int 		file;
	struct stat sb;
	size_t		i;
	int 		n;

	file = open(file_name, O_RDONLY);
	if (file == -1){
		printf("ERROR: in %s, unable to open file: %s\n", __func__, file_name);
		return -1;
	}

	if (fstat(file, &sb) < 0){
		printf("ERROR: in %s, unable to read file size\n", __func__);
		close(file);
		return -1;
	}

	list->buffer = (char*)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, file, 0);
	if (list->buffer == NULL){
		printf("ERROR: in %s, unable to mmap memory\n", __func__);
		close(file);
		return -1;
	}

	close(file);

	list->buffer_size = sb.st_size;
	list->nb_entry = 1;

	for (i = 0; i < list->buffer_size; i++){
		if (list->buffer[i] == '\n'){
			list->nb_entry ++;
			list->buffer[i] = '\0';
		}
	}

	list->entries = (char**)malloc(sizeof(char*)*list->nb_entry);
	if (list->entries == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		munmap(list->buffer, list->buffer_size);
		return -1;
	}

	list->entries[0] = list->buffer;
	for (n = 1, i = 0; n < list->nb_entry; n++){
		while ((i < list->buffer_size) && (list->buffer[i] != '\0')){
			i++;
		}
		if (i >= list->buffer_size){
			printf("ERROR: in %s, searching for end of line fails at line %d:%d\n", __func__, n, list->nb_entry);
			list->entries[n] = list->entries[n-1];
		}
		else{
			i++;
			list->entries[n] = list->buffer + i;
		}
	}

	qsort(list->entries, list->nb_entry, sizeof(char*), whiteList_compare);	

	return 0;
}

void whiteList_clean(struct whiteList* list){
	if (list->entries != NULL){
		free(list->entries);
	}
	if (list->buffer != NULL && list->buffer_size > 0){
		munmap(list->buffer, list->buffer_size);
	}
}

#endif

#ifdef WIN32
#include <windows.h>

#include "windowsComp.h"

int whiteList_init(struct whiteList* list, const char* file_name){
	HANDLE 		file;
	HANDLE 		map_file;
	void* 		view_map;
	size_t		i;
	int 		n;

	file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE){
		printf("ERROR: in %s, unable to open file: \"%s\"\n", __func__, file_name);
		return -1;
	}

	map_file = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (map_file == NULL){
		printf("ERROR: in %s, unable to create file mapping\n", __func__);
		CloseHandle(file);
		return -1;
	}

	view_map = MapViewOfFile(map_file, FILE_MAP_READ, 0, 0, 0);
	if (view_map == NULL){
		printf("ERROR: in %s, unable to create map view\n", __func__);
		CloseHandle(map_file);
		CloseHandle(file);
		return -1;
	}

	list->buffer_size = GetFileSize(file, NULL);
	list->buffer = (char*)malloc(list->buffer_size);
	if (list->buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		UnmapViewOfFile(view_map);
		CloseHandle(map_file);
		CloseHandle(file);
		return -1;
	}

	memcpy(list->buffer, view_map, list->buffer_size);

	UnmapViewOfFile(view_map);
	CloseHandle(map_file);
	CloseHandle(file);

	list->nb_entry = 1;

	for (i = 0; i < list->buffer_size; i++){
		if (list->buffer[i] == '\n'){
			list->nb_entry ++;
			list->buffer[i] = '\0';
		}
	}

	list->entries = (char**)malloc(sizeof(char*)*list->nb_entry);
	if (list->entries == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(list->buffer);
		return -1;
	}

	list->entries[0] = list->buffer;
	for (n = 1, i = 0; n < list->nb_entry; n++){
		while ((i < list->buffer_size) && (list->buffer[i] != '\0')){
			i++;
		}
		if (i >= list->buffer_size){
			printf("ERROR: in %s, searching for end of line fails at line %d:%d\n", __func__, n, list->nb_entry);
			list->entries[n] = list->entries[n-1];
		}
		else{
			i++;
			list->entries[n] = list->buffer + i;
		}
	}

	qsort(list->entries, list->nb_entry, sizeof(char*), whiteList_compare);	

	return 0;
}

void whiteList_clean(struct whiteList* list){
	if (list->entries != NULL){
		free(list->entries);
	}
	if (list->buffer != NULL && list->buffer_size > 0){
		free(list->buffer);
	}
}

#endif

struct whiteList* whiteList_create(const char* file_name){
	struct whiteList* 	result;

	result = (struct whiteList*)malloc(sizeof(struct whiteList));
	if (result == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		if (whiteList_init(result, file_name)){
			printf("ERROR: in %s, unable to init whiteList\n", __func__);
			free(result);
			result = NULL;
		}
	}

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
		whiteList_clean(list);
		free(list);
	}
}

int whiteList_compare(const void* entry1, const void* entry2){
	#ifdef __linux__
	return strcmp(*(char**)entry1, *(char**)entry2);
	#endif
	#ifdef WIN32
	return _stricmp(*(char**)entry1, *(char**)entry2);
	#endif
}