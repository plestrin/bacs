#include <stdlib.h>
#include <stdint.h>

#include "../../base.h"
#include "../llist.h"

struct ll_u32{
	struct llist 	ll;
	uint32_t 		value;
};

static int check(struct ll_head* llh, uint32_t start, uint32_t stop){
	uint32_t 		i;
	struct llist* 	cursor;

	for (cursor = llh_get_top(llh), i = start; cursor != NULL; cursor = llist_get_next(cursor), i ++){
		if (((struct ll_u32*)cursor)->value != i){
			log_err_m("get an unexpected value @%u", i);
			return -1;
		}

		if (llist_get_prev(cursor) == NULL && llh_get_top(llh) != cursor){
			log_err("inconsistency: prev is NULL but it is not registered as top");
			return -1;
		}

		if (llist_get_next(cursor) == NULL && llh_get_bot(llh) != cursor){
			log_err("inconsistency: next is NULL but it is not registered as bot");
			return -1;
		}

		if (llist_get_prev(cursor) != NULL && llist_get_next(llist_get_prev(cursor)) != cursor){
			log_err("inconsistency: not equal to prev->next");
			return -1;
		}

		if (llist_get_next(cursor) != NULL && llist_get_prev(llist_get_next(cursor)) != cursor){
			log_err("inconsistency: not equal to next->prev");
			return -1;
		}
	}

	if (i != stop){
		log_err_m("value %u was never reached", stop);
		return -1;
	}

	return 0;
}

int main(void){
	struct ll_head llh = LLH_INIT;
	struct ll_u32* n1;
	struct ll_u32* n2;
	struct ll_u32* n3;
	struct ll_u32* n4;

	#define create(value_) 												\
		if ((n ## value_ = malloc(sizeof(struct ll_u32))) == NULL){ 	\
			log_err("unable to allocate memory"); 						\
			return EXIT_FAILURE; 										\
		} 																\
		n ##value_ ->value = value_;

	create(1)

	llh_add_top(&llh, (struct llist*)n1);

	create(2)

	llh_add_bot(&llh, (struct llist*)n2);

	/* Expect (1, 2) */
	if (check(&llh, 1, 3)){
		return EXIT_FAILURE;
	}

	create(4)

	llh_add_new_next(&llh, (struct llist*)n2, (struct llist*)n4);

	create(3)

	llh_add_new_prev(&llh, (struct llist*)n4, (struct llist*)n3);

	/* Expect (1, 2, 3, 4) */
	if (check(&llh, 1, 5)){
		return EXIT_FAILURE;
	}

	llh_del(&llh, (struct llist*)n3);
	llh_del(&llh, (struct llist*)n4);
	llh_del(&llh, (struct llist*)n1);

	/* Expect (2) */
	if (check(&llh, 2, 3)){
		return EXIT_FAILURE;
	}

	llh_del(&llh, (struct llist*)n2);

	/* Expect () */
	if (check(&llh, 0, 0)){
		return EXIT_FAILURE;
	}

	free(n1);
	free(n2);

	log_info("test_list completed!");

	return EXIT_SUCCESS;
}
