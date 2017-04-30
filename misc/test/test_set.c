#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../set.h"
#include "../printBuffer.h"
#include "../base.h"

#define ELEMENT_SIZE 		15
#define NB_ELEMENT_BLOCK 	13
#define NB_ELEMENT_TOT 		4096
#define NB_ELEMENT_STOP 	128

int main(void){
	uint32_t 			i;
	uint32_t 			j;
	struct set* 		set;
	char 				data[ELEMENT_SIZE];
	uint8_t 			s[NB_ELEMENT_TOT];
	struct setIterator 	iterator;
	char* 				data_ptr;
	uint32_t 			offset;
	uint8_t* 			buffer;
	uint32_t 			nb_element;

	if ((set = set_create(ELEMENT_SIZE, NB_ELEMENT_BLOCK)) == NULL){
		log_err("unable to create set");
		return EXIT_FAILURE;
	}

	for (i = 0; i < NB_ELEMENT_TOT; i++){
		memset(data, (int)i, ELEMENT_SIZE);
		if (set_add(set, data) < 0){
			log_err_m("unable to add element %u", i);
		}
		s[i] = (uint8_t)i;
	}

	while(set->nb_element_tot > NB_ELEMENT_STOP){
		for (data_ptr = setIterator_get_first(set, &iterator), i = 0, offset = 0; data_ptr != NULL; data_ptr = setIterator_get_next(&iterator), i++){
			memset(data, s[i], ELEMENT_SIZE);
			if (memcmp(data, data_ptr, ELEMENT_SIZE)){
				log_err_m("element %u seems to be different", i);
			}

			if ((rand() & 0x03) == 0x03){
				setIterator_pop(&iterator);
			}
			else{
				s[offset ++] = s[i];
			}
		}
	}

	if ((buffer = set_export_buffer_unique(set, &nb_element)) != NULL){
		for (i = 0; i < nb_element; i++){
			fprintBuffer_raw(stdout, (char*)(buffer + i * ELEMENT_SIZE), ELEMENT_SIZE); putchar('\n');

			for (j = i + 1; j < nb_element; j++){
				if (!memcmp(buffer + i * ELEMENT_SIZE, buffer + j * ELEMENT_SIZE, ELEMENT_SIZE)){
					log_err("duplicate element");
				}
			}
		}
	}
	else{
		log_err("unable to export buffer unique");
	}

	set_delete(set);

	return EXIT_SUCCESS;
}
