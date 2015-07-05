#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../set.h"
#include "../base.h"

#define ELEMENT_SIZE 		15
#define NB_ELEMENT_BLOCK 	13
#define NB_ELEMENT_TOT 		4096
#define NB_ELEMENT_STOP 	4

int main(){
	uint32_t 			i;
	struct set* 		set;
	char 				data[ELEMENT_SIZE];
	uint8_t 			s[NB_ELEMENT_TOT];
	struct setIterator 	iterator;
	char* 				data_ptr;
	uint32_t 			offset;

	set = set_create(ELEMENT_SIZE, NB_ELEMENT_BLOCK);
	if (set == NULL){
		log_err("unable to create set");
	}

	for (i = 0; i < NB_ELEMENT_TOT; i++){
		memset(data, i, ELEMENT_SIZE);
		if (set_add(set, data)){
			log_err_m("unable to add element %u", i);
		}
		s[i] = i;
	}

	while(set->nb_element_tot > NB_ELEMENT_STOP){
		for (data_ptr = setIterator_get_first(set, &iterator), i = 0, offset = 0; data_ptr != NULL; data_ptr = setIterator_get_next(&iterator), i++){
			memset(data, s[i], ELEMENT_SIZE);
			if (memcmp(data, data_ptr, ELEMENT_SIZE)){
				log_err_m("element %u seems to be different", i);
			}

			if ((rand() & 0x03) != 0x03){
				setIterator_pop(&iterator);
			}
			else{
				s[offset ++] = s[i];
			}
		}
	}

	set_delete(set);

	return 0;
}