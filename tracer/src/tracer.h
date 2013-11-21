#ifndef TRACER_H
#define TRACER_H

#include "codeMap.h"
#include "whiteList.h"


struct tracer{
	struct codeMap* 	code_map;
	struct whiteList*	white_list;
};

#endif