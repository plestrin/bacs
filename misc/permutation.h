#ifndef PERMUTATION_H
#define PERMUTATION_H

#include <stdint.h>
#include <alloca.h>

#define INIT_PERMUTATION(nb_element) 																														\
	uint32_t* 	permutation_state; 																															\
	uint32_t* 	permutation_counter; 																														\
	uint32_t 	permutation_depth; 																															\
	uint32_t 	permutation_tmp; 																															\
	uint32_t 	permutation_nb_element;																														\
																																							\
	permutation_state = (uint32_t*)alloca(sizeof(uint32_t) * (nb_element)); 																				\
	permutation_counter = (uint32_t*)alloca(sizeof(uint32_t) * (nb_element)); 																				\
																																							\
	for (i = 0; i < (nb_element); i++){ 																													\
		permutation_state[i] = i; 																															\
		permutation_counter[i] = 0; 																														\
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
		if ((permutation_nb_element - permutation_depth) & 0x00000001){ 																				\
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
	} 																																						\

#endif