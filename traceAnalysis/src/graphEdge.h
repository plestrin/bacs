#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

struct graphEdge{
	unsigned long				src_id;
	unsigned long				dst_id;
	int 						nb_execution;
};


static inline void graphEdge_init(struct graphEdge* edge, unsigned long src, unsigned long dst, int nb_execution){
	edge->src_id = src;
	edge->dst_id = dst;
	edge->nb_execution = nb_execution;
}

static inline void graphEdge_increment(struct graphEdge* edge, int nb_execution){
	edge->nb_execution += nb_execution;
}

#endif