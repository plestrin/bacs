#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argSet.h"

int32_t argSet_init(struct argSet* set, char* tag){
	set->input = array_create(sizeof(struct argBuffer));
	if (set->input == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		return -1;
	}
	set->output = array_create(sizeof(struct argBuffer));
	if (set->output == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		array_delete(set->input);
		return -1;
	}

	strncpy(set->tag, tag, ARGSET_TAG_MAX_LENGTH);

	return 0;
}

void argSet_clean(struct argSet* set){
	uint32_t 			i;
	struct argBuffer* 	arg;

	if (set->input != NULL){
		for (i = 0; i < array_get_length(set->input); i++){
			arg = (struct argBuffer*)array_get(set->input, i);
			free(arg->data);
		}
		array_delete(set->input);
		set->input = NULL;
	}

	if (set->output != NULL){
		for (i = 0; i < array_get_length(set->output); i++){
			arg = (struct argBuffer*)array_get(set->output, i);
			free(arg->data);
		}
		array_delete(set->output);
		set->output = NULL;
	}
}