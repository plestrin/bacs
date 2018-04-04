#ifndef LLIST_H
#define LLIST_H

struct llist{
	struct llist* next;
	struct llist* prev;
};

#define llist_get_next(llist) ((llist)->next)
#define llist_get_prev(llist) ((llist)->prev)

static inline void llist_add_new_next(struct llist* ll, struct llist* new){
	new->next = ll->next;
	new->prev = ll;

	ll->next = new;
	if (new->next != NULL){
		new->next->prev = new;
	}
}

static inline void llist_add_new_prev(struct llist* ll, struct llist* new){
	new->next = ll;
	new->prev = ll->prev;

	ll->prev = new;
	if (new->prev != NULL){
		new->prev->next = new;
	}
}

static inline void llist_del(struct llist* ll){
	if (ll->next != NULL){
		ll->next->prev = ll->prev;
	}
	if (ll->prev != NULL){
		ll->prev->next = ll->next;
	}
}

struct ll_head{
	struct llist* ll_top;
	struct llist* ll_bot;
};

#define LLH_INIT {.ll_top = NULL, .ll_bot = NULL}

#define llh_get_top(llh) ((llh)->ll_top)
#define llh_get_bot(llh) ((llh)->ll_bot)

static inline void llh_init(struct ll_head* llh){
	llh->ll_top = NULL;
	llh->ll_bot = NULL;
}

static inline void llh_add_top(struct ll_head* llh, struct llist* new){
	new->next = llh->ll_top;
	new->prev = NULL;

	llh->ll_top = new;
	if (new->next == NULL){
		llh->ll_bot = new;
	}
	else{
		new->next->prev = new;
	}
}

static inline void llh_add_bot(struct ll_head* llh, struct llist* new){
	new->next = NULL;
	new->prev = llh->ll_bot;

	llh->ll_bot = new;
	if (new->prev == NULL){
		llh->ll_top = new;
	}
	else{
		new->prev->next = new;
	}
}

static inline void llh_add_new_next(struct ll_head* llh, struct llist* ll, struct llist* new){
	new->next = ll->next;
	new->prev = ll;

	ll->next = new;
	if (new->next == NULL){
		llh->ll_bot = new;
	}
	else{
		new->next->prev = new;
	}
}

static inline void llh_add_new_prev(struct ll_head* llh, struct llist* ll, struct llist* new){
	new->next = ll;
	new->prev = ll->prev;

	ll->prev = new;
	if (new->prev == NULL){
		llh->ll_top = new;
	}
	else{
		new->prev->next = new;
	}
}

static inline void llh_del(struct ll_head* llh, struct llist* ll){
	if (ll->next == NULL){
		llh->ll_bot = ll->prev;
	}
	else{
		ll->next->prev = ll->prev;
	}

	if (ll->prev == NULL){
		llh->ll_top = ll->next;
	}
	else{
		ll->prev->next = ll->next;
	}
}

#define llh_is_empty(llh) ((llh)->ll_top == NULL && (llh)->ll_bot == NULL)

#endif
