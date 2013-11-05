#include <stdlib.h>
#include <stdio.h>

#include "gephiCFGIO.h"

static void gephiCFGIO_print_header(FILE* file);
static void gephiCFGIO_print_node(FILE* file, struct controlFlowGraph* cfg);
static void gephiCFGIO_print_edge(FILE* file, struct controlFlowGraph* cfg);
static void gephiCFGIO_print_footer(FILE* file);


int gephiCFGIO_print(struct controlFlowGraph* cfg, const char* file_name){
	FILE* file;


	if (file_name != NULL){
		file = fopen(file_name, "w");
	}
	else{
		file = fopen(GEPHICFGIO_DEFAULT_FILE_NAME, "w");
	}

	if (file == NULL){
		if (file_name != NULL){
			printf("ERROR: in %s, unable to open file: %s\n", __func__, file_name);
		}
		else{
			printf("ERROR: in %s, unable to open file: %s\n", __func__, GEPHICFGIO_DEFAULT_FILE_NAME);
		}
		return -1;
	}

	gephiCFGIO_print_header(file);

	if (cfg != NULL){
		gephiCFGIO_print_node(file, cfg);
		gephiCFGIO_print_edge(file, cfg);
	}

	gephiCFGIO_print_footer(file);

	fclose(file);

	return 0;
}

static void gephiCFGIO_print_header(FILE* file){
	fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gexf xmlns=\"http://www.gexf.net/1.2draft\" xmlns:viz=\"http://www.gexf.net/1.1draft/viz\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.gexf.net/1.2draft http://www.gexf.net/1.2draft/gexf.xsd\" version=\"1.2\">\n");
	fprintf(file, "\t<graph defaultedgetype=\"directed\">\n");
	fprintf(file, "\t\t<attributes class=\"node\">\n\t\t\t<attribute id=\"0\" title=\"nb_instruction\" type=\"integer\"/>\n\t\t\t<attribute id=\"1\" title=\"nb_execution\" type=\"integer\"/>\n\t\t</attributes>\n");
	fprintf(file, "\t\t<attributes class=\"edge\">\n\t\t\t<attribute id=\"0\" title=\"nb_execution\" type=\"integer\"/>\n\t\t</attributes>\n");
}

static void gephiCFGIO_print_node(FILE* file, struct controlFlowGraph* cfg){
	int i;

	fprintf(file, "\t\t<nodes>\n");

	for (i = 0; i < cfg->nb_block; i++){
		fprintf(file, "\t\t\t<node id=\"%d\" label=\"Block %d\" >\n\t\t\t\t<attvalues>\n\t\t\t\t\t<attvalue for=\"0\" value=\"%d\"/>\n\t\t\t\t\t<attvalue for=\"1\" value=\"%d\"/>\n\t\t\t\t</attvalues>\n\t\t\t</node>\n", i, i, basicBlock_get_nb_instruction(cfg->blocks + i), basicBlock_get_nb_execution(cfg->blocks + i));
	}

	fprintf(file, "\t\t</nodes>\n");
}

static void gephiCFGIO_print_edge(FILE* file, struct controlFlowGraph* cfg){
	int i;

	fprintf(file, "\t\t<edges>\n");

	for (i = 0; i < cfg->nb_edge; i++){
		fprintf(file, "\t\t\t<edge id=\"%d\" source=\"%d\" target=\"%d\">\n\t\t\t\t<attvalues>\n\t\t\t\t\t<attvalue for=\"0\" value=\"%d\"/>\n\t\t\t\t</attvalues>\n\t\t\t</edge>\n", i, controlFlowGraph_get_edge_src_offset(cfg, i), controlFlowGraph_get_edge_dst_offset(cfg, i), edge_get_nb_execution(cfg->edges + i));
	}

	fprintf(file, "\t\t</edges>\n");
}

static void gephiCFGIO_print_footer(FILE* file){
	fprintf(file, "\t</graph>\n</gexf>\n");
}