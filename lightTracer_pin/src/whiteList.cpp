#include "pin.H"

#ifdef WIN32
#include "windowsComp.h"
#endif

#include "whiteList.h"

static int whiteList_compare(const void* entry1, const void* entry2);

int whiteList_init(struct whiteList* list, const char* file_name){
	FILE* 	file = NULL;
	long 	size;
	int 	i;
	int 	n;

	list->buffer = NULL;

	if ((file = fopen(file_name, "rb")) == NULL){
		LOG("ERROR: unable to open file: ");
		LOG(file_name);
		LOG("\n");
		goto error;
	}

	if (fseek(file, 0, SEEK_END)){
		LOG("ERROR: unable to fseek\n");
		goto error;
	}
	
	size = ftell(file);
	rewind(file);

	if ((list->buffer = (char*)malloc(size)) == NULL){
		LOG("ERROR: unable to allocate memory\n");
		goto error;
	}

	if (fread(list->buffer, 1, size, file) != (size_t)size){
		LOG("ERROR: unable to read file\n");
		goto error;
	}

	fclose (file);
	file = NULL;

	for (i = 0, list->nb_entry = 1; i < size; i++){
		if (list->buffer[i] == '\n'){
			list->nb_entry ++;
			list->buffer[i] = '\0';
		}
	}

	if ((list->entries = (char**)malloc(sizeof(char*)*list->nb_entry)) == NULL){
		LOG("ERROR: unable to allocate memory\n");
		goto error;
	}

	list->entries[0] = list->buffer;
	for (n = 1, i = 0; n < list->nb_entry; n++){
		while ((i < size) && (list->buffer[i] != '\0')){
			i++;
		}
		if (i >= size){
			LOG("ERROR: searching for end of line fails at line " + decstr(n) + ":" + decstr(list->nb_entry) + "\n");
			list->entries[n] = list->entries[n-1];
		}
		else{
			i++;
			list->entries[n] = list->buffer + i;
		}
	}

	qsort(list->entries, list->nb_entry, sizeof(char*), whiteList_compare);	

	return 0;

	error:
	if (file != NULL){
		fclose(file);
	}
	if (list->buffer == NULL){
		free(list->buffer);
	}

	return -1;
}

void whiteList_clean(struct whiteList* list){
	if (list->entries != NULL){
		free(list->entries);
	}
	if (list->buffer != NULL){
		free(list->buffer);
	}
}

struct whiteList* whiteList_create(const char* file_name){
	struct whiteList* result;

	if ((result = (struct whiteList*)malloc(sizeof(struct whiteList))) == NULL){
		LOG("ERROR: unable to allocate memory\n");
	}
	else{
		if (whiteList_init(result, file_name)){
			LOG("ERROR: unable to init whiteList\n");
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

void whiteList_delete(struct whiteList* list){
	if (list != NULL){
		whiteList_clean(list);
		free(list);
	}
}

static int whiteList_compare(const void* entry1, const void* entry2){
	#ifdef __linux__
	return strcmp(*(char**)entry1, *(char**)entry2);
	#endif
	#ifdef WIN32
	return _stricmp(*(char**)entry1, *(char**)entry2);
	#endif
}