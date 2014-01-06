#ifndef TRACER_H
#define TRACER_H

#ifdef __linux__

#include "codeMap.h"

#endif

#ifdef WIN32

#include "../../shared/codeMap.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif

#include "whiteList.h"


struct tracer{
	struct codeMap* 	code_map;
	struct whiteList*	white_list;
};

#endif