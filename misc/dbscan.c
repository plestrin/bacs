#include <stdlib.h>
#include <stdio.h>

#include "dbscan.h"

struct neighborHeader{
	uint32_t 	nb_neighbor;
	uint32_t 	neighbor_offset;
	uint8_t 	visited;
};

struct neighborDataBase{
	uint32_t* 				neighbors;
	struct neighborHeader 	headers[1];
};

static struct neighborDataBase* dbscan_create_neighborDB(uint32_t nb_element, uint32_t element_size, void* elements, dbscan_norme norme);
static void dbscan_add_element_to_cluster(struct array* cluster, struct neighborDataBase* db, uint32_t index, uint32_t element_size, void* elements);

static struct neighborDataBase* dbscan_create_neighborDB(uint32_t nb_element, uint32_t element_size, void* elements, dbscan_norme norme){
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					counter = 0;
	struct neighborDataBase* 	db = NULL;

	if (nb_element > 0){
		db = (struct neighborDataBase*)malloc(sizeof(struct neighborDataBase) + (sizeof(struct neighborHeader) * (nb_element - 1)));
		if (db != NULL){
			db->neighbors = (uint32_t*)malloc(sizeof(uint32_t) * nb_element * nb_element);
			if (db->neighbors != NULL){
				for (i = 0; i < nb_element; i++){
					db->headers[i].nb_neighbor = 0;
					db->headers[i].neighbor_offset = counter;
					db->headers[i].visited = 0;

					for (j = 0; j < nb_element; j++){
						if (norme((void*)((char*)elements + element_size * i), (void*)((char*)elements + element_size * j))){
							db->headers[i].nb_neighbor ++;
							db->neighbors[counter] = j;
							counter ++;
						}
					}
				}
				db->neighbors = (uint32_t*)realloc(db->neighbors, sizeof(uint32_t) * counter);
				if (db->neighbors == NULL){
					printf("ERROR: in %s, unable to reallocate memory\n", __func__);
					free(db);
					db = NULL;
				}
			}
			else{
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				free(db);
				db = NULL;
			}
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, incorrect parametres\n", __func__);
	}

	return db;
}

static void dbscan_add_element_to_cluster(struct array* cluster, struct neighborDataBase* db, uint32_t index, uint32_t element_size, void* elements){
	void* 		el;
	uint32_t 	j;

	el = (void*)((char*)elements + element_size * index);
	if (array_add(cluster, &el) < 0){
		printf("ERROR: in %s, unable to add element to cluster\n", __func__);
	}

	for (j = 0; j < db->headers[index].nb_neighbor; j++){
		if (!db->headers[db->neighbors[db->headers[index].neighbor_offset + j]].visited){
			db->headers[db->neighbors[db->headers[index].neighbor_offset + j]].visited = 1;

			dbscan_add_element_to_cluster(cluster, db, db->neighbors[db->headers[index].neighbor_offset + j], element_size, elements);
		}
	}
}

struct array* dbscan_cluster(uint32_t nb_element, uint32_t element_size, void* elements, dbscan_norme norme){
	struct neighborDataBase* 	db;
	struct array* 				cluster_set = NULL;
	uint32_t 					i;
	struct array 				cluster;

	db = dbscan_create_neighborDB(nb_element, element_size, elements, norme);
	if (db == NULL){
		printf("ERROR: in %s, unable to create neighbor data base\n", __func__);
		return cluster_set;
	}

	cluster_set = array_create(sizeof(struct array));
	if (cluster_set != NULL){
		for (i = 0; i < nb_element; i++){
			if (!db->headers[i].visited){
				db->headers[i].visited = 1;
				if (array_init(&cluster, sizeof(void*))){
					printf("ERROR: in %s, unable to init array\n", __func__);
					continue;
				}

				dbscan_add_element_to_cluster(&cluster, db, i, element_size, elements);

				if (array_add(cluster_set, &cluster) < 0){
					printf("ERROR: in %s, unable to add cluster to cluster set\n", __func__);
					array_clean(&cluster);
				}

			}
		}
	}
	else{
		printf("ERROR: in %s, unable to create array\n", __func__);
	}

	free(db->neighbors);
	free(db);

	return cluster_set;
}
