#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include "pin.H"

#ifdef WIN32
#include "windowsComp.h"
#endif

#include "codeMap.h"
#include "whiteList.h"
#include "traceFile.h"

#define DEFAULT_TRACE_FILE_NAME 		"trace"
#define DEFAULT_WHITE_LIST_FILE_NAME	""

KNOB<string> 	knob_trace(KNOB_MODE_WRITEONCE, "pintool", "o", DEFAULT_TRACE_FILE_NAME, "Specify a directory to write trace results");
KNOB<string> 	knob_white_list(KNOB_MODE_WRITEONCE, "pintool", "w", DEFAULT_WHITE_LIST_FILE_NAME, "(Optional) Shared library white list. Specify file name");

struct lightTracer{
	BUFFER_ID 					block_buffer_id;
	TLS_KEY 					thread_key;
	struct codeMap* 			code_map;
	struct whiteList*			white_list;
	struct traceFile* 			trace_file;
};

struct lightTracer	light_tracer;

/* ===================================================================== */
/* Analysis function(s) 	                                             */
/* ===================================================================== */

void TOOL_routine_analysis(void* cm_routine_ptr){
	struct cm_routine* routine = (struct cm_routine*)cm_routine_ptr;
	
	CODEMAP_INCREMENT_ROUTINE_EXE(routine);
}

/* ===================================================================== */
/* Instrumentation function                                              */
/* ===================================================================== */

void TOOL_instrumentation_trace(TRACE trace, void* arg){
	BBL 					basic_block;
	struct asmBlockHeader 	block_header;

	for(basic_block = TRACE_BblHead(trace); BBL_Valid(basic_block); basic_block = BBL_Next(basic_block)){
		if (codeMap_is_instruction_whiteListed(light_tracer.code_map, (unsigned long)BBL_Address(basic_block)) == CODEMAP_NOT_WHITELISTED){
			block_header.size 		= BBL_Size(basic_block);
			block_header.nb_ins 	= BBL_NumIns(basic_block);
			block_header.address 	= BBL_Address(basic_block);

			traceFile_write_block(light_tracer.trace_file, block_header)

			BBL_InsertFillBuffer(basic_block, IPOINT_BEFORE, light_tracer.block_buffer_id, IARG_UINT32, block_header.id, 0, IARG_END);
		}
		else{
			BBL_InsertFillBuffer(basic_block, IPOINT_BEFORE, light_tracer.block_buffer_id, IARG_UINT32, BLACK_LISTED_ID, 0, IARG_END);
		}
	}
}

void TOOL_instrumentation_img(IMG image, void* arg){
	SEC 				section;
	RTN 				routine;
	struct cm_routine*	cm_rtn;
	char				white_listed;

	white_listed = (whiteList_search(light_tracer.white_list, IMG_Name(image).c_str()) == 0) ? CODEMAP_WHITELISTED : CODEMAP_NOT_WHITELISTED;
	if (codeMap_add_image(light_tracer.code_map, IMG_LowAddress(image), IMG_HighAddress(image), IMG_Name(image).c_str(), white_listed)){
		std::cerr << "ERROR: in " << __func__ << ", unable to add image " << IMG_Name(image) << " to code map structure" << std::endl;
	}
	else{
		for (section = IMG_SecHead(image); SEC_Valid(section); section = SEC_Next(section)){
			if (SEC_IsExecutable(section) && SEC_Mapped(section)){
				if (codeMap_add_section(light_tracer.code_map, SEC_Address(section), SEC_Address(section) + SEC_Size(section), SEC_Name(section).c_str())){
					std::cerr << "ERROR: in " << __func__ << ", unable to add section " << SEC_Name(section) << " to code map structure" << std::endl;
					break;
				}
				else{
					for (routine = SEC_RtnHead(section); RTN_Valid(routine); routine = RTN_Next(routine)){
						white_listed |= (whiteList_search(light_tracer.white_list, RTN_Name(routine).c_str()) == 0) ? CODEMAP_WHITELISTED : CODEMAP_NOT_WHITELISTED;
						cm_rtn = codeMap_add_routine(light_tracer.code_map, RTN_Address(routine), RTN_Address(routine) + RTN_Range(routine), RTN_Name(routine).c_str(), white_listed);
						if (cm_rtn == NULL){
							std::cerr << "ERROR: in " << __func__ << ", unable to add routine " << RTN_Name(routine) << " to code map structure" << std::endl;
							break;
						}
						else if (white_listed == CODEMAP_NOT_WHITELISTED){
							RTN_Open(routine);
							RTN_InsertCall(routine, IPOINT_BEFORE, (AFUNPTR)TOOL_routine_analysis, IARG_PTR, cm_rtn, IARG_END);
							RTN_Close(routine);
						}
					}
				}
			}
		}
	}
}

/* ===================================================================== */
/* Call back function                                                 	 */
/* ===================================================================== */

void* TOOL_block_buffer_full(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, void* buffer, UINT64 numElements, void* arg){
	FILE* 		file = (FILE*)PIN_GetThreadData(light_tracer.thread_key, tid);
	uint64_t 	i;
	uint64_t 	rewrite_offset;
	uint32_t* 	buffer_id;

	if (file == NULL){
		char file_name[TRACEFILE_NAME_MAX_LENGTH];

		snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/blockId%u.bin", traceFile_get_dir_name(light_tracer.trace_file), tid);
		#ifdef __linux__
		file = fopen(file_name, "wb");
		#endif
		#ifdef WIN32
		fopen_s(&file, file_name, "wb");
		#endif
		PIN_SetThreadData(light_tracer.thread_key, file, tid);
	}

	for (i = 1, rewrite_offset = 1, buffer_id = (uint32_t*)buffer; i < numElements; i++){
		if (buffer_id[i - 1] == BLACK_LISTED_ID){
			if (buffer_id[i] != BLACK_LISTED_ID){
				buffer_id[rewrite_offset ++] = buffer_id[i];
			}
		}
		else{
			buffer_id[rewrite_offset ++] = buffer_id[i];
		}
	}

	if (file != NULL){
		fwrite(buffer, sizeof(uint32_t), rewrite_offset, file);
	}

	return buffer;
}

void TOOL_thread_start(THREADID tid, CONTEXT *ctxt, INT32 flags, void* arg){
	PIN_SetThreadData(light_tracer.thread_key, NULL, tid);
}


void TOOL_thread_stop(THREADID tid, const CONTEXT *ctxt, INT32 code, void* arg){
	FILE* file = (FILE*)PIN_GetThreadData(light_tracer.thread_key, tid);

	if (file != NULL){
		fclose(file);
		PIN_SetThreadData(light_tracer.thread_key, NULL, tid);
	}
}

void TOOL_clean(INT32 code, void* arg){
	traceFile_print_codeMap(light_tracer.trace_file, light_tracer.code_map);
	codeMap_delete(light_tracer.code_map);

	whiteList_delete(light_tracer.white_list);

	traceFile_delete(light_tracer.trace_file);
}

/* ===================================================================== */
/* Init function                                                    	 */
/* ===================================================================== */

int TOOL_init(const char* trace_dir_name, const char* white_list_file_name){
	light_tracer.thread_key	= -1;
	light_tracer.code_map 	= NULL;
	light_tracer.white_list = NULL;
	light_tracer.trace_file = NULL;

	light_tracer.code_map = codeMap_create();
	if (light_tracer.code_map == NULL){
		std::cerr << "ERROR: in " << __func__ << ", unable to create codeMap" << std::endl;
		goto fail;
	}

	if (white_list_file_name != NULL && strcmp(white_list_file_name, DEFAULT_WHITE_LIST_FILE_NAME)){
		light_tracer.white_list = whiteList_create(white_list_file_name);
		if (light_tracer.white_list == NULL){
			std::cerr << "ERROR: in " << __func__ << ", unable to create shared library whiteList" << std::endl;
			goto fail;
		}
	}
	else{
		light_tracer.white_list = NULL;
	}

	light_tracer.trace_file = traceFile_create(trace_dir_name);
	if (light_tracer.trace_file == NULL){
		std::cerr << "ERROR: in " << __func__ << ", unable to create traceFile" << std::endl;
		goto fail;
	}
	
	light_tracer.thread_key = PIN_CreateThreadDataKey(NULL);
	if (light_tracer.thread_key == -1){
		std::cerr << "Error: in " <<__func__ << ", could not create ThreadDataKey" << std::endl;
		goto fail;
	}

	light_tracer.block_buffer_id = PIN_DefineTraceBuffer(sizeof(uint32_t), 4, TOOL_block_buffer_full, NULL);
	if(light_tracer.block_buffer_id == BUFFER_ID_INVALID){
		std::cerr << "Error: in " <<__func__ << ", could not allocate initial buffer" << std::endl;
		goto fail;
	}

	#ifdef __linux__
	if (codeMap_add_vdso(light_tracer.code_map, ((whiteList_search(light_tracer.white_list, "VDSO") == 0) ? CODEMAP_WHITELISTED : CODEMAP_NOT_WHITELISTED))){
		std::cerr << "ERROR: in " << __func__ << ", unable to add VDSO to code map" << std::endl;
	}
	#endif

	return 0;

	fail:
	if (light_tracer.thread_key != -1){
		PIN_DeleteThreadDataKey(light_tracer.thread_key); 
	}
	if (light_tracer.code_map != NULL){
		codeMap_delete(light_tracer.code_map);
	}
	if (light_tracer.white_list != NULL){
		whiteList_delete(light_tracer.white_list);
	}
	if (light_tracer.trace_file != NULL){
		traceFile_delete(light_tracer.trace_file);
	}

	return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[]){
	PIN_InitSymbols();

	if (PIN_Init(argc, argv)){
		std::cerr << "ERROR: in %s, unable to init PIN" << std::endl << KNOB_BASE::StringKnobSummary();
		return -1;
	}

	if (TOOL_init(knob_trace.Value().c_str(), knob_white_list.Value().c_str())){
		std::cerr << "ERROR: in %s, unable to init the TOOL" << std::endl << KNOB_BASE::StringKnobSummary();
		return -1;
	}

	IMG_AddInstrumentFunction(TOOL_instrumentation_img, NULL);
	TRACE_AddInstrumentFunction(TOOL_instrumentation_trace, NULL);
	PIN_AddThreadStartFunction(TOOL_thread_start, NULL);
    PIN_AddThreadFiniFunction(TOOL_thread_stop, NULL);
	PIN_AddFiniFunction(TOOL_clean, NULL);
	
	PIN_StartProgram();

	return 0;
}
