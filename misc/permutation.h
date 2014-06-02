#ifndef PERMUTATION_H
#define PERMUTATION_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define PERMUTATION_CREATE(nb_element, alloc_func) 																											\
	uint8_t* 	permutation_state; 																															\
	uint8_t* 	permutation_counter; 																														\
	uint8_t 	permutation_depth; 																															\
	uint8_t 	permutation_tmp; 																															\
	uint8_t 	permutation_i; 																																\
	uint8_t 	permutation_nb_element;																														\
																																							\
	permutation_state = (uint8_t*)(alloc_func)(sizeof(uint8_t) * (nb_element)); 																			\
	permutation_counter = (uint8_t*)(alloc_func)(sizeof(uint8_t) * (nb_element)); 																			\
																																							\
	if (permutation_state == NULL || permutation_counter == NULL){ 																							\
		printf("ERROR: in %s, unable to allocate memory\n", __func__); 																						\
	} 																																						\
																																							\
	for (permutation_i = 0; permutation_i < (nb_element); permutation_i++){ 																				\
		permutation_state[permutation_i] = permutation_i; 																									\
		permutation_counter[permutation_i] = 0; 																											\
	} 																																						\
																																							\
	permutation_depth = (nb_element); 																														\
	permutation_nb_element = (nb_element);

#define PERMUTATION_INIT(nb_element, permutation_state_, permutation_counter_) 																				\
	uint8_t* 	permutation_state = (uint8_t*)(permutation_state_); 																						\
	uint8_t* 	permutation_counter = (uint8_t*)(permutation_counter_); 																					\
	uint8_t 	permutation_depth; 																															\
	uint8_t 	permutation_tmp; 																															\
	uint8_t 	permutation_i; 																																\
	uint8_t 	permutation_nb_element;																														\
																																							\
	for (permutation_i = 0; permutation_i < (nb_element); permutation_i++){ 																				\
		permutation_state[permutation_i] = permutation_i; 																									\
	} 																																						\
																																							\
	permutation_depth = (nb_element); 																														\
	permutation_nb_element = (nb_element);

#define PERMUTATION_GET_FIRST(state) 																														\
	state = permutation_state;

#define PERMUTATION_GET_NEXT(state) 																														\
	while ((permutation_nb_element - permutation_depth) <= permutation_counter[permutation_depth - 1] && permutation_depth > 1){ 							\
		permutation_depth --; 																																\
		permutation_counter[permutation_depth] = 0;																											\
	} 																																						\
																																							\
	if ((permutation_nb_element - permutation_depth) > permutation_counter[permutation_depth - 1]){															\
		if ((permutation_nb_element - permutation_depth) & 0x01){ 																							\
			permutation_tmp = permutation_state[permutation_depth - 1]; 																					\
			permutation_state[permutation_depth - 1] = permutation_state[permutation_nb_element - 1 - permutation_counter[permutation_depth - 1]]; 			\
			permutation_state[permutation_nb_element - 1 - permutation_counter[permutation_depth - 1]] = permutation_tmp; 									\
			permutation_counter[permutation_depth - 1] ++; 																									\
			permutation_depth = permutation_nb_element - 1; 																								\
			state = permutation_state; 																														\
		}																																					\
		else{ 																																				\
			permutation_tmp = permutation_state[permutation_depth - 1]; 																					\
			permutation_state[permutation_depth - 1] = permutation_state[permutation_nb_element - 1]; 														\
			permutation_state[permutation_nb_element - 1] = permutation_tmp; 																				\
			permutation_counter[permutation_depth - 1] ++; 																									\
			permutation_depth = permutation_nb_element - 1; 																								\
			state = permutation_state; 																														\
		} 																																					\
	} 																																						\
	else{ 																																					\
		state = NULL; 																																		\
	}

#define PERMUTATION_DELETE() 																																\
	free(permutation_state); 																																\
	free(permutation_counter);

#endif