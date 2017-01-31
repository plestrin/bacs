#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ugraph.h"
#include "../graphPrintDot.h"
#include "../base.h"

#define COUNTRY_NAME_SIZE 256

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void dotPrint_node_data(void* data, FILE* file, void* arg){
	char* name = (char*)data;

	fprintf(file, "[label=\"%s\"]", name);
}

int32_t main(void){
	struct ugraph 	ugraph;
	char 			country_name[COUNTRY_NAME_SIZE];

	struct unode* 	unode_fr;
	struct unode* 	unode_sp;
	struct unode* 	unode_it;
	struct unode* 	unode_ch;
	struct unode* 	unode_de;

	ugraph_init(&ugraph, COUNTRY_NAME_SIZE, 0)

	ugraph_register_dotPrint_callback(&ugraph, dotPrint_node_data, NULL)

	/* Add unode */
	strncpy(country_name, "France", COUNTRY_NAME_SIZE);
	if ((unode_fr = ugraph_add_node(&ugraph, country_name)) == NULL){
		log_err("unable to unode to ugraph");
		return EXIT_FAILURE;
	}

	strncpy(country_name, "Spain", COUNTRY_NAME_SIZE);
	if ((unode_sp = ugraph_add_node(&ugraph, country_name)) == NULL){
		log_err("unable to unode to ugraph");
		return EXIT_FAILURE;
	}

	strncpy(country_name, "Italy", COUNTRY_NAME_SIZE);
	if ((unode_it = ugraph_add_node(&ugraph, country_name)) == NULL){
		log_err("unable to unode to ugraph");
		return EXIT_FAILURE;
	}

	strncpy(country_name, "Switzerland", COUNTRY_NAME_SIZE);
	if ((unode_ch = ugraph_add_node(&ugraph, country_name)) == NULL){
		log_err("unable to unode to ugraph");
		return EXIT_FAILURE;
	}

	strncpy(country_name, "Germany", COUNTRY_NAME_SIZE);
	if ((unode_de = ugraph_add_node(&ugraph, country_name)) == NULL){
		log_err("unable to unode to ugraph");
		return EXIT_FAILURE;
	}

	/* Add uedge */
	if (ugraph_add_edge_(&ugraph, unode_fr, unode_sp) == NULL){
		log_err("unable to add uedge to ugraph");
		return EXIT_FAILURE;
	}

	if (ugraph_add_edge_(&ugraph, unode_fr, unode_it) == NULL){
		log_err("unable to add uedge to ugraph");
		return EXIT_FAILURE;
	}

	if (ugraph_add_edge_(&ugraph, unode_fr, unode_ch) == NULL){
		log_err("unable to add uedge to ugraph");
		return EXIT_FAILURE;
	}

	if (ugraph_add_edge_(&ugraph, unode_fr, unode_de) == NULL){
		log_err("unable to add uedge to ugraph");
		return EXIT_FAILURE;
	}

	if (ugraph_add_edge_(&ugraph, unode_it, unode_ch) == NULL){
		log_err("unable to add uedge to ugraph");
		return EXIT_FAILURE;
	}

	if (ugraph_add_edge_(&ugraph, unode_ch, unode_de) == NULL){
		log_err("unable to add uedge to ugraph");
		return EXIT_FAILURE;
	}

	if (ugraphPrintDot_print(&ugraph, "european.dot", NULL)){
		log_err("unable to print ugraph in DOT format");
		return EXIT_FAILURE;
	}

	ugraph_clean(&ugraph)

	return EXIT_SUCCESS;
}
