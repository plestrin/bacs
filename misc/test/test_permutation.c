#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../permutation.h"
#include "../base.h"

#define NUMBER_OF_ELEMENT_TO_PERMUTE 5

int main(){
	uint32_t 	element[NUMBER_OF_ELEMENT_TO_PERMUTE];
	uint32_t 	i;
	uint32_t 	j;
	uint8_t* 	state;
	uint32_t* 	value;
	uint32_t 	nb_perm;
	uint32_t 	counter = 0;

	for (i = 0, nb_perm = 1; i < NUMBER_OF_ELEMENT_TO_PERMUTE; i++){
		element[i] = i;
		nb_perm = nb_perm * (i+1);
	}

	value = (uint32_t*)malloc(sizeof(uint32_t) * nb_perm * NUMBER_OF_ELEMENT_TO_PERMUTE);
	if (value == NULL){
		log_err("unable to allocate memory");
		return 0;
	}

	{
		PERMUTATION_CREATE(NUMBER_OF_ELEMENT_TO_PERMUTE, malloc)
		PERMUTATION_GET_FIRST(state)
		while(state != NULL){
			for (i = 0; i < NUMBER_OF_ELEMENT_TO_PERMUTE; i++){
				value[counter * NUMBER_OF_ELEMENT_TO_PERMUTE + i] = element[state[i]];
				if (i == NUMBER_OF_ELEMENT_TO_PERMUTE - 1){
					printf("%u\n", element[state[i]]);
				}
				else{
					printf("%u, ", element[state[i]]);
				}
			}
			PERMUTATION_GET_NEXT(state)
			counter++;
		}
		PERMUTATION_DELETE()
	}

	if (counter != nb_perm){
		log_err_m("incorrect number of permutation: expected %u but get %u", nb_perm, counter);
	}
	else{
		for (i = 0; i < counter; i++){
			for (j = 0; j < counter; j++){
				if (i != j && !memcmp(value + i*NUMBER_OF_ELEMENT_TO_PERMUTE, value + j*NUMBER_OF_ELEMENT_TO_PERMUTE, NUMBER_OF_ELEMENT_TO_PERMUTE * sizeof(uint32_t))){
					log_err_m("collision detected between %u and %u", i, j);
				}
			}
		}
	}

	free(value);

	return 0;
}