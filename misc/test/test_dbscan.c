#include <stdlib.h>
#include <stdio.h>

#include "../dbscan.h"

#define NB_ELEMENT 	16
#define RADIUS 		10

int32_t compare_int(uint32_t* a, uint32_t* b){
	if (*a > *b){
		return *a - *b <= RADIUS;
	}
	else{
		return *b - *a <= RADIUS;
	}
}

int main(){
	struct array* 	cluster_set;
	uint32_t 		elements[NB_ELEMENT] = {1, 45, 139, 256, 254, 16, 239, 6998, 12, 241, 33, 2569, 335, 11, 111, 1024};
	uint32_t 		i;
	uint32_t 		j;
	struct array* 	cluster;

	cluster_set = dbscan_cluster(NB_ELEMENT, sizeof(uint32_t), elements, (dbscan_norme)compare_int);
	if (cluster_set == NULL){
		printf("ERROR: in %s, cluster method return NULL\n", __func__);
	}

	for (i = 0; i < array_get_length(cluster_set); i++){
		cluster = array_get(cluster_set, i);
		printf("Cluster %02u : {", i + 1);
		for (j = 0; j < array_get_length(cluster); j++){
			if (j != array_get_length(cluster) - 1){
				printf("%u, ", **(uint32_t**)array_get(cluster, j));
			}
			else{
				printf("%u", **(uint32_t**)array_get(cluster, j));
			}
		}
		printf("}\n");
		array_clean(cluster);
	}
	array_delete(cluster_set);

	return 0;
}

