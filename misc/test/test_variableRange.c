#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../variableRange.h"
#include "../base.h"

#define NB_TEST 		10000000
#define MAX_SIZE 		8
#define MASK 			(0xffffffffffffffff >> (64 - MAX_SIZE))
/*#define SEED 			10347*/ 				/* comment this line to start to used a different SEED */
#define TEST_ADD
#define TEST_AND
#define TEST_SHL 
#define TEST_SHR
#define TEST_INCLUDE
#define TEST_INTERSECT
/*#define TEST_CUSTOM*/
#define NO_PRINT 								/* comment this line to print successful test */

#ifndef SEED
#define SEED getpid()
#endif

#ifdef TEST_ADD
static int32_t test_add(void){
	uint32_t 				i;
	uint64_t 				j;
	uint64_t 				k;
	struct variableRange 	range[3];
	uint64_t 				value;
	struct variableRange 	cst;
	uint64_t 				nb_test_predicted;
	uint64_t 				nb_test_performed;

	log_info_m("starting %u TEST ADD", NB_TEST);

	for (i = 0; i < NB_TEST; i++){
		nb_test_predicted = 1;
		nb_test_performed = 0;

		range[0].index_lo 	= rand();
		range[0].index_up 	= rand();
		range[0].scale 		= rand() % MAX_SIZE;
		range[0].disp 		= rand();
		range[0].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		range[1].index_lo 	= rand();
		range[1].index_up 	= rand();
		range[1].scale 		= range[0].scale; /* cannot add range with different scale */
		range[1].disp 		= rand();
		range[1].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		variableRange_pack(range + 0);
		variableRange_pack(range + 1);

		nb_test_predicted = variableRange_get_nb_value(range + 0) * variableRange_get_nb_value(range + 1);

		memcpy(range + 2, range + 0, sizeof(struct variableRange));
		variableRange_add(range + 2, range + 1, MAX_SIZE);

		for (j = 0; j <= range[0].size_mask; j++){
			variableRange_init_cst(&cst, j, MAX_SIZE);
			cst.size_mask 	= range[0].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 0, &cst)){
				continue;
			}

			for (k = 0; k <= range[1].size_mask; k++){
				variableRange_init_cst(&cst, k, MAX_SIZE);
				cst.size_mask 	= range[1].size_mask;
				cst.disp 		= cst.disp & cst.size_mask;
				if (!variableRange_include(range + 1, &cst)){
					continue;
				}
				nb_test_performed ++;

				value = j + k;
				variableRange_init_cst(&cst, value, MAX_SIZE);
				cst.size_mask 	= range[2].size_mask;
				cst.disp 		= cst.disp & cst.size_mask;
				if (!variableRange_include(range + 2, &cst)){
					log_err_m("incorrect range @ %u, seed=%u", i, SEED);
					printf("\t-0x%llx + 0x%llx = 0x%llx not in ",  j, k, value);
					variableRange_print(range + 2);
					printf("\n\t\t-0x%llx is in ", j); variableRange_print(range + 0);
					printf("\n\t\t-0x%llx is in ", k); variableRange_print(range + 1);
					printf("\n");
					return -1;
				}
			}
		 }

		if (nb_test_performed != nb_test_predicted){
			log_err_m("TEST ADD %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
			printf("\t-"); variableRange_print(range + 0); printf(" nb_value=%llu\n", variableRange_get_nb_value(range + 0));
			printf("\t-"); variableRange_print(range + 1); printf(" nb_value=%llu\n", variableRange_get_nb_value(range + 1));
			return -1;
		}
		#ifndef NO_PRINT
		else{
			log_info_m("TEST ADD %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
		}
		#endif
	}

	return 0;
} 
#endif

#ifdef TEST_AND
static int32_t test_and(void){
	uint32_t 				i;
	uint64_t 				j;
	struct variableRange 	range[3];
	uint64_t 				value;
	struct variableRange 	cst;
	uint64_t 				nb_test_predicted;
	uint64_t 				nb_test_performed;

	log_info_m("starting %u TEST AND", NB_TEST);

	for (i = 0; i < NB_TEST; i++){
		nb_test_predicted = 1;
		nb_test_performed = 0;

		range[0].index_lo 	= rand();
		range[0].index_up 	= rand();
		range[0].scale 		= rand() % MAX_SIZE;
		range[0].disp 		= rand();
		range[0].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		range[1].index_lo 	= 0;
		range[1].index_up 	= 0;
		range[1].scale 		= 0xffffffff; /* cannot mask range by a not constant value */
		range[1].disp 		= rand();
		range[1].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		variableRange_pack(range + 0);
		variableRange_pack(range + 1);

		nb_test_predicted = variableRange_get_nb_value(range + 0);

		memcpy(range + 2, range + 0, sizeof(struct variableRange));
		variableRange_and(range + 2, range + 1, MAX_SIZE);

		for (j = 0; j <= range[0].size_mask; j++){
			variableRange_init_cst(&cst, j, MAX_SIZE);
			cst.size_mask 	= range[0].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 0, &cst)){
				continue;
			}
				
			nb_test_performed ++;

			value = j & range[1].disp;
			variableRange_init_cst(&cst, value, MAX_SIZE);
			cst.size_mask 	= range[2].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 2, &cst)){
				log_err_m("incorrect range @ %u, seed=%u", i, SEED);
				printf("\t-0x%llx & 0x%llx = 0x%llx not in ",  j, range[1].disp, cst.disp);
				variableRange_print(range + 2);
				printf("\n\t\t-0x%llx is in ", j); variableRange_print(range + 0);
				printf("\n");
				return -1;
			}
		 }

		if (nb_test_performed != nb_test_predicted){
			log_err_m("TEST AND %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
			printf("\t-"); variableRange_print(range + 0); printf(" nb_value=%llu\n", variableRange_get_nb_value(range + 0));
			return -1;
		}
		#ifndef NO_PRINT
		else{
			log_info_m("TEST AND %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
		}
		#endif
	}

	return 0;
} 
#endif

#ifdef TEST_SHL
static int32_t test_shl(void){
	uint32_t 				i;
	uint64_t 				j;
	struct variableRange 	range[3];
	uint64_t 				value;
	struct variableRange 	cst;
	uint64_t 				nb_test_predicted;
	uint64_t 				nb_test_performed;

	log_info_m("starting %u TEST SHL", NB_TEST);

	for (i = 0; i < NB_TEST; i++){
		nb_test_predicted = 1;
		nb_test_performed = 0;

		range[0].index_lo 	= rand();
		range[0].index_up 	= rand();
		range[0].scale 		= rand() % MAX_SIZE;
		range[0].disp 		= rand();
		range[0].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		variableRange_pack(range + 0);

		if (variableRange_is_cst(range + 0)){ /* skip cst -it is pretty obvious */
			i --;
			continue;
		}

		range[1].index_lo 	= 0;
		range[1].index_up 	= 0;
		range[1].scale 		= 0xffffffff; /* cannot shift by a not constant value */
		range[1].disp 		= rand() % (MAX_SIZE + 1);
		range[1].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		
		variableRange_pack(range + 1);

		nb_test_predicted = variableRange_get_nb_value(range + 0);

		memcpy(range + 2, range + 0, sizeof(struct variableRange));
		variableRange_shl(range + 2, range + 1, MAX_SIZE);

		for (j = 0; j <= range[0].size_mask; j++){
			variableRange_init_cst(&cst, j, MAX_SIZE);
			cst.size_mask 	= range[0].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 0, &cst)){
				continue;
			}
				
			nb_test_performed ++;

			value = j << range[1].disp;
			variableRange_init_cst(&cst, value, MAX_SIZE);
			cst.size_mask 	= range[2].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 2, &cst)){
				log_err_m("incorrect range @ %u, seed=%u", i, SEED);
				printf("\t-0x%llx << 0x%llx = 0x%llx not in ",  j, range[1].disp, cst.disp);
				variableRange_print(range + 2);
				printf("\n\t\t-0x%llx is in ", j); variableRange_print(range + 0);
				printf("\n");
				return -1;
			}
		 }

		if (nb_test_performed != nb_test_predicted){
			log_err_m("TEST SHL %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
			printf("\t-"); variableRange_print(range + 0); printf(" nb_value=%llu\n", variableRange_get_nb_value(range + 0));
			return -1;
		}
		#ifndef NO_PRINT
		else{
			log_info_m("TEST SHL %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
		}
		#endif
	}

	return 0;
} 
#endif

#ifdef TEST_SHR
static int32_t test_shr(void){
	uint32_t 				i;
	uint64_t 				j;
	struct variableRange 	range[3];
	uint64_t 				value;
	struct variableRange 	cst;
	uint64_t 				nb_test_predicted;
	uint64_t 				nb_test_performed;

	log_info_m("starting %u TEST SHR", NB_TEST);

	for (i = 0; i < NB_TEST; i++){
		nb_test_predicted = 1;
		nb_test_performed = 0;

		range[0].index_lo 	= rand();
		range[0].index_up 	= rand();
		range[0].scale 		= rand() % MAX_SIZE;
		range[0].disp 		= rand();
		range[0].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		variableRange_pack(range + 0);

		if (variableRange_is_cst(range + 0)){ /* skip cst -it is pretty obvious */
			i --;
			continue;
		}

		range[1].index_lo 	= 0;
		range[1].index_up 	= 0;
		range[1].scale 		= 0xffffffff; /* cannot shift by a not constant value */
		range[1].disp 		= rand() % (MAX_SIZE + 1);
		range[1].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		
		variableRange_pack(range + 1);

		nb_test_predicted = variableRange_get_nb_value(range + 0);

		memcpy(range + 2, range + 0, sizeof(struct variableRange));
		variableRange_shr(range + 2, range + 1, MAX_SIZE);

		for (j = 0; j <= range[0].size_mask; j++){
			variableRange_init_cst(&cst, j, MAX_SIZE);
			cst.size_mask 	= range[0].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 0, &cst)){
				continue;
			}
				
			nb_test_performed ++;

			value = j >> range[1].disp;
			variableRange_init_cst(&cst, value, MAX_SIZE);
			cst.size_mask 	= range[2].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;
			if (!variableRange_include(range + 2, &cst)){
				log_err_m("incorrect range @ %u, seed=%u", i, SEED);
				printf("\t-0x%llx >> 0x%llx = 0x%llx not in ",  j, range[1].disp, cst.disp);
				variableRange_print(range + 2);
				printf("\n\t\t-0x%llx is in ", j); variableRange_print(range + 0);
				printf("\n");
				return -1;
			}
		 }

		if (nb_test_performed != nb_test_predicted){
			log_err_m("TEST SHR %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
			printf("\t-"); variableRange_print(range + 0); printf(" nb_value=%llu\n", variableRange_get_nb_value(range + 0));
			return -1;
		}
		#ifndef NO_PRINT
		else{
			log_info_m("TEST SHR %u, %llu/%llu", i, nb_test_performed, nb_test_predicted);
		}
		#endif
	}

	return 0;
} 
#endif

#ifdef TEST_INCLUDE
static int32_t test_include(void){
	uint32_t 				i;
	uint32_t 				j;
	struct variableRange 	range[2];
	struct variableRange 	cst;
	uint32_t 				included_predicted;
	uint32_t 				included_performed;
	uint64_t 				nb_test;

	log_info_m("starting %u TEST INCLUDE", NB_TEST);

	for (i = 0; i < NB_TEST; i++){
		range[0].index_lo 	= rand();
		range[0].index_up 	= rand();
		range[0].scale 		= rand() % MAX_SIZE;
		range[0].disp 		= rand();
		range[0].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));

		variableRange_pack(range + 0);

		if (variableRange_is_cst(range + 0)){ /* skip cst -it is pretty obvious */
			i --;
			continue;
		}

		range[1].index_lo 	= rand();
		range[1].index_up 	= rand();
		range[1].scale 		= rand() % MAX_SIZE;
		range[1].disp 		= rand();
		range[1].size_mask 	= (0xffffffffffffffff >> (64 - ((rand() % (MAX_SIZE - 1)) + 1)));;

		variableRange_pack(range + 1);

		included_predicted = variableRange_include(range + 0, range + 1);

		for (j = 0, included_performed = 1, nb_test = 0; j <= range[1].size_mask; j++){
			variableRange_init_cst(&cst, j, MAX_SIZE);
			cst.size_mask 	= range[1].size_mask;
			cst.disp 		= cst.disp & cst.size_mask;

			if (variableRange_include(range + 1, &cst)){
				nb_test ++;
				if (!variableRange_include(range + 0, &cst)){
					included_performed = 0;
					if (included_predicted){
						log_err_m("incorrect include relation @ %u, seed=%u", i, SEED);
						printf("\t-0x%x in ", j); variableRange_print(range + 1); printf("\n");
						printf("\t-0x%x not in ", j); variableRange_print(range + 0); printf("\n");
						return -1;
					}
				}
			}
		}

		if (nb_test != variableRange_get_nb_value(range + 1)){
			log_err_m("TEST INCLUDE %u %llu/%llu", i, nb_test, variableRange_get_nb_value(range + 1));
			printf("\t"); variableRange_print(range + 1); printf("\n");
			return -1;
		}
		#ifndef NO_PRINT
		else{
			log_info_m("TEST INCLUDE %u, %llu/%llu", i, nb_test, variableRange_get_nb_value(range + 1));
		}
		#endif

		if ((included_performed  && !included_predicted) || (!included_performed  && included_predicted)){
			log_err_m("incorrect include relation @ %u, seed=%u, (predicted=%u performed=%u)", i, SEED, included_predicted, included_performed);
			printf("\t"); variableRange_print(range + 1); printf(" C "); variableRange_print(range + 0); printf("\n");
			return -1;
		}
	}

	return 0;
}
#endif

#ifdef TEST_INTERSECT
static int32_t test_intersect(void){
	uint32_t 				i;
	uint32_t 				j;
	struct variableRange 	range1;
	struct variableRange 	range2;
	struct variableRange 	cst;
	uint32_t 				intersect_predicted;
	uint32_t 				intersect_performed;
	uint64_t 				nb_test;

	log_info_m("starting %u TEST INTERSECT", NB_TEST);

	for (i = 0; i < NB_TEST; i++){
		range1.index_lo 	= rand();
		range1.index_up 	= rand();
		range1.scale 		= rand() % MAX_SIZE;
		range1.disp 		= rand();
		range1.size_mask 	= MASK;

		variableRange_pack(&range1);

		if (variableRange_is_cst(&range1)){ /* skip cst -it is pretty obvious */
			i --;
			continue;
		}

		range2.index_lo 	= rand();
		range2.index_up 	= rand();
		range2.scale 		= rand() % MAX_SIZE;
		range2.disp 		= rand();
		range2.size_mask 	= MASK;
		
		variableRange_pack(&range2);

		if (variableRange_is_cst(&range2)){ /* skip cst -it is pretty obvious */
			i --;
			continue;
		}

		intersect_predicted = variableRange_intersect(&range1, &range2);

		for (j = 0, intersect_performed = 0, nb_test = 0; j <= range2.size_mask; j++){
			variableRange_init_cst(&cst, j, MAX_SIZE);
			cst.size_mask 	= range2.size_mask;
			cst.disp 		= cst.disp & cst.size_mask;

			if (variableRange_include(&range2, &cst)){
				nb_test ++;
				if (variableRange_include(&range1, &cst)){
					intersect_performed = 1;
					if (!intersect_predicted){
						log_err_m("incorrect intersect relation @ %u, seed=%u", i, SEED);
						printf("\t-0x%x in ", j); variableRange_print(&range2); printf("\n");
						printf("\t-0x%x in ", j); variableRange_print(&range1); printf("\n");
						return -1;
					}
				}
			}
		}

		if (nb_test != variableRange_get_nb_value(&range2)){
			log_err_m("TEST INTERSECT %u %llu/%llu", i, nb_test, variableRange_get_nb_value(&range2));
			printf("\t"); variableRange_print(&range2); printf("\n");
		}
		#ifndef NO_PRINT
		else{
			log_info_m("TEST INTERSECT %u, %llu/%llu", i, nb_test, variableRange_get_nb_value(&range2));
		}
		#endif

		if ((intersect_performed  && !intersect_predicted) || (!intersect_performed  && intersect_predicted)){
			log_err_m("incorrect intersect relation @ %u, seed=%u, (predicted=%u performed=%u)", i, SEED, intersect_predicted, intersect_performed);
			printf("\t"); variableRange_print(&range2); printf(" |_| "); variableRange_print(&range1); printf("\n");
			return -1;
		}
	}

	return 0;
}
#endif

#ifdef TEST_CUSTOM
static int32_t test_custom(void){
	struct variableRange 	range1;
	struct variableRange 	range2;
	uint64_t 				v1 = 0x6c; /*0x1c*/
	uint64_t 				v2 = 0x1;
	uint64_t 				v3;
	struct variableRange 	cst;

	range1.index_lo 	= 0x0;
	range1.index_up 	= 0x12;
	range1.scale 		= 2;
	range1.disp 		= 0x5c;
	range1.size_mask 	= MASK;

	range2.index_lo 	= 0x0;
	range2.index_up 	= 0x0;
	range2.scale 		= 0xffffffff;
	range2.disp 		= 0x1;
	range2.size_mask 	= MASK;

	variableRange_init_cst(&cst, v1);
	if (!variableRange_include(&range1, &cst, MAX_SIZE)){
		log_err("v1 is not in range1");
		return -1;
	}

	variableRange_init_cst(&cst, v2);
	if (!variableRange_include(&range2, &cst, MAX_SIZE)){
		log_err("v2 is not in range2");
		return -1;
	}

	v3 = v1 >> v2;

	variableRange_shr(&range1, &range2, MAX_SIZE);
	variableRange_init_cst(&cst, v3);
	if (!variableRange_include(&range1, &cst, MAX_SIZE)){
		log_err("test failed");
		printf("\t0x%llx not in ", v3); variableRange_print(&range1); printf("\n");
		return -1;
	}

	return 0;
}
#endif

int main(void){
	#ifndef SEED
	#define SEED getpid()
	#endif
	log_info_m("seed=%d", SEED);
	srand(SEED);

	#ifdef TEST_CUSTOM
	if (test_custom()){
		return 0;
	}
	#endif
	#ifdef TEST_ADD
	if (test_add()){
		return 0;
	}
	#endif
	#ifdef TEST_AND
	if (test_and()){
		return 0;
	}
	#endif
	#ifdef TEST_SHL
	if (test_shl()){
		return 0;
	}
	#endif
	#ifdef TEST_SHR
	if (test_shr()){
		return 0;
	}
	#endif
	#ifdef TEST_INCLUDE
	if (test_include()){
		return 0;
	}
	#endif
	#ifdef TEST_INTERSECT
	if (test_intersect()){
		return 0;
	}
	#endif

	return 0;
}