#ifndef GEPHICFGIO_H
#define GEPHICFGIO_H

#include "controlFlowGraph.h"

#define GEPHICFGIO_DEFAULT_FILE_NAME "cfg.gexf"

int gephiCFGIO_print(struct controlFlowGraph* cfg, const char* file_name);

#endif