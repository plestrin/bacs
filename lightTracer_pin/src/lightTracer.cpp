#include "pin.H"

#ifdef _WIN32
#include "windowsComp.h"
#endif

#include "codeMap.h"
#include "whiteList.h"
#include "traceFile.h"
#include "memTrace.h"

#define DEFAULT_TRACE_FILE_NAME 		"trace"
#define DEFAULT_WHITE_LIST_FILE_NAME	""
#define DEFAULT_MEMORY_ADD_TRACE 		"0"
#define DEFAULT_MEMORY_VAL_TRACE 		"0"

KNOB<string> 	knob_trace(KNOB_MODE_WRITEONCE, "pintool", "o", DEFAULT_TRACE_FILE_NAME, "Specify a directory to write trace results");
KNOB<string> 	knob_white_list(KNOB_MODE_WRITEONCE, "pintool", "w", DEFAULT_WHITE_LIST_FILE_NAME, "Shared library white list. Specify file name");
KNOB<bool> 		knob_memory_add(KNOB_MODE_WRITEONCE, "pintool", "m", DEFAULT_MEMORY_ADD_TRACE, "Save memory addresses alongside the execution trace");
KNOB<bool> 		knob_memory_val(KNOB_MODE_WRITEONCE, "pintool", "v", DEFAULT_MEMORY_VAL_TRACE, "Save memory values alongside the execution trace");

struct lightTracer{
	BUFFER_ID 			block_buffer_id;
	BUFFER_ID 			mem_buffer_id;
	TLS_KEY 			thread_key;
	struct codeMap* 	code_map;
	struct whiteList*	white_list;
	struct traceFile* 	trace_file;
	bool 				trace_mem_add;
	bool 				trace_mem_val;
	PIN_THREAD_UID 		comm_intern_tread_id;
};

struct toolThreadData{
	FILE* 		block_file;
	FILE* 		mem_add_file;
	FILE* 		mem_val_file;
	uint32_t 	mem_flag;
	char 		r_mem1[MEMVALUE_READ_PADDING];
	char 		r_mem2[MEMVALUE_READ_PADDING];
	ADDRINT 	w_mem1;
	ADDRINT 	w_mem2;
	uint32_t 	size_w_mem1;
	uint32_t 	size_w_mem2;
};

#define MEM_FLAG_R_MEM_1 	0x00000001
#define MEM_FLAG_R_MEM_2 	0x00000002
#define MEM_FLAG_W_MEM_1 	0x00000010
#define MEM_FLAG_W_MEM_2 	0x00000020
#define MEM_FLAG_MEM_1 		0x00000011
#define MEM_FLAG_MEM_2 		0x00000022

static struct lightTracer light_tracer;

/* ===================================================================== */
/* Analysis function(s) 	                                             */
/* ===================================================================== */

static void TOOL_routine_analysis(void* cm_routine_ptr){
	struct cm_routine* routine = (struct cm_routine*)cm_routine_ptr;

	CODEMAP_INCREMENT_ROUTINE_EXE(routine);
}

static void INS_trace_memory_r_mem1(ADDRINT address, UINT32 size, THREADID tid){
	struct toolThreadData* data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);
	data->mem_flag |= MEM_FLAG_R_MEM_1;
	PIN_SafeCopy(data->r_mem1, (void*)address, size);
}

static void INS_trace_memory_r_mem2(ADDRINT address, UINT32 size, THREADID tid){
	struct toolThreadData* data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);
	data->mem_flag |= MEM_FLAG_R_MEM_2;
	PIN_SafeCopy(data->r_mem2, (void*)address, size);
}

static void INS_trace_memory_w_mem1(ADDRINT address, UINT32 size, THREADID tid){
	struct toolThreadData* data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);
	data->mem_flag 		|= MEM_FLAG_W_MEM_1;
	data->w_mem1 		= address;
	data->size_w_mem1 	= size;
}

static void INS_trace_memory_w_mem2(ADDRINT address, UINT32 size, THREADID tid){
	struct toolThreadData* data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);
	data->mem_flag 		|= MEM_FLAG_W_MEM_2;
	data->w_mem2 		= address;
	data->size_w_mem2 	= size;
}

static void INS_trace_memory_flush(THREADID tid){
	struct toolThreadData* 	data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);
	char 					value[MEMVALUE_WRITE_PADDING];

	if (data->mem_val_file == NULL){
		char file_name[TRACEFILE_NAME_MAX_LENGTH];

		snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/memValu%u_%u.bin", traceFile_get_dir_name(light_tracer.trace_file), PIN_GetPid(), tid);
		data->mem_val_file = fopen(file_name, "wb");
	}

	if (data->mem_val_file != NULL){
		/* 1 is packed */
		if (data->mem_flag & MEM_FLAG_MEM_1){
			fwrite(data->r_mem1, MEMVALUE_READ_PADDING, 1, data->mem_val_file);
			if (data->mem_flag & MEM_FLAG_W_MEM_1){
				PIN_SafeCopy(value, (void*)data->w_mem1, data->size_w_mem1);
			}
			fwrite(value, MEMVALUE_WRITE_PADDING, 1, data->mem_val_file);
		}
		/* 2 is not */
		if (data->mem_flag & MEM_FLAG_R_MEM_2){
			fwrite(data->r_mem2, MEMVALUE_READ_PADDING, 1, data->mem_val_file);
			fwrite(value, MEMVALUE_WRITE_PADDING, 1, data->mem_val_file);
		}
		if (data->mem_flag & MEM_FLAG_W_MEM_2){
			fwrite(data->r_mem2, MEMVALUE_READ_PADDING, 1, data->mem_val_file);
			PIN_SafeCopy(value, (void*)data->w_mem2, data->size_w_mem2);
			fwrite(value, MEMVALUE_WRITE_PADDING, 1, data->mem_val_file);
		}
	}

	data->mem_flag = 0;
}

/* ===================================================================== */
/* Instrumentation function                                              */
/* ===================================================================== */

static void TOOL_instrumentation_trace(TRACE trace, void* arg){
	BBL 					basic_block;
	struct asmBlockHeader 	block_header;
	INS 					instruction;
	uint32_t 				i;
	uint32_t 				nb_mem_access;
	uint32_t 				nb_mem_read;
	uint32_t 				nb_mem_write;
	uint32_t 				descriptor;

	for(basic_block = TRACE_BblHead(trace); BBL_Valid(basic_block); basic_block = BBL_Next(basic_block)){
		if (codeMap_is_instruction_whiteListed(light_tracer.code_map, (unsigned long)BBL_Address(basic_block)) == CODEMAP_WHITELISTED){

			nb_mem_access = UNTRACK_MEM_ACCESS;
			if (light_tracer.trace_mem_add){
				nb_mem_access = 0;
				for (instruction = BBL_InsHead(basic_block); INS_Valid(instruction); instruction = INS_Next(instruction)){
					if (INS_IsNop(instruction)){
						continue;
					}

					for (i = 0, nb_mem_read = 0, nb_mem_write = 0; i < INS_OperandCount(instruction); i++){
						if (INS_OperandIsMemory(instruction, i)){
							descriptor = MEMADDRESS_DESCRIPTOR_CLEAN;
							nb_mem_access ++;

							if (INS_OperandRead(instruction, i)){
								memAddress_descriptor_set_read(descriptor, nb_mem_read);
								nb_mem_read ++;
							}
							if (INS_OperandWritten(instruction, i)){
								memAddress_descriptor_set_write(descriptor, nb_mem_write);
								nb_mem_write ++;
							}

							if (INS_OperandRead(instruction, i)){
								if (nb_mem_read == 1){
									INS_InsertFillBuffer(instruction, IPOINT_BEFORE, light_tracer.mem_buffer_id, IARG_INST_PTR, offsetof(struct memAddress, pc), IARG_UINT32, descriptor, offsetof(struct memAddress, descriptor), IARG_MEMORYREAD_EA, offsetof(struct memAddress, address), IARG_END);
									if (light_tracer.trace_mem_val){
										INS_InsertCall(instruction, IPOINT_BEFORE, AFUNPTR(INS_trace_memory_r_mem1), IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_THREAD_ID, IARG_END);
									}
								}
								else{
									INS_InsertFillBuffer(instruction, IPOINT_BEFORE, light_tracer.mem_buffer_id, IARG_INST_PTR, offsetof(struct memAddress, pc), IARG_UINT32, descriptor, offsetof(struct memAddress, descriptor), IARG_MEMORYREAD2_EA, offsetof(struct memAddress, address), IARG_END);
									if (light_tracer.trace_mem_val){
										INS_InsertCall(instruction, IPOINT_BEFORE, AFUNPTR(INS_trace_memory_r_mem2), IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_THREAD_ID, IARG_END);
									}
								}
							}
							else{
								INS_InsertFillBuffer(instruction, IPOINT_BEFORE, light_tracer.mem_buffer_id, IARG_INST_PTR, offsetof(struct memAddress, pc), IARG_UINT32, descriptor, offsetof(struct memAddress, descriptor), IARG_MEMORYWRITE_EA, offsetof(struct memAddress, address), IARG_END);
							}
							/* The address is saved only once for both read and write, but the value must be saved two times for a read & write access */
							if (light_tracer.trace_mem_val && INS_OperandWritten(instruction, i)){
								if (INS_OperandWrittenOnly(instruction, i)){
									INS_InsertCall(instruction, IPOINT_BEFORE, AFUNPTR(INS_trace_memory_w_mem2), IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_THREAD_ID, IARG_END);
								}
								else if (nb_mem_write == 1){
									INS_InsertCall(instruction, IPOINT_BEFORE, AFUNPTR(INS_trace_memory_w_mem1), IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_THREAD_ID, IARG_END);
								}
								else{
									LOG("ERROR: this case (second memory write is not write only) is not implemented\n");
								}
							}
						}
					}
					if (light_tracer.trace_mem_val && nb_mem_read + nb_mem_write){
						if (INS_IsBranchOrCall(instruction)){
							INS_InsertCall(instruction, IPOINT_TAKEN_BRANCH, AFUNPTR(INS_trace_memory_flush), IARG_THREAD_ID, IARG_END);
						}
						else{
							INS_InsertCall(instruction, IPOINT_AFTER, AFUNPTR(INS_trace_memory_flush), IARG_THREAD_ID, IARG_END);
						}
					}
				}
			}

			block_header.size 			= BBL_Size(basic_block);
			block_header.nb_ins 		= BBL_NumIns(basic_block);
			block_header.nb_mem_access 	= nb_mem_access;
			block_header.address 		= BBL_Address(basic_block);

			traceFile_write_block(light_tracer.trace_file, block_header)

			BBL_InsertFillBuffer(basic_block, IPOINT_BEFORE, light_tracer.block_buffer_id, IARG_UINT32, block_header.id, 0, IARG_END);
		}
		else{
			BBL_InsertFillBuffer(basic_block, IPOINT_BEFORE, light_tracer.block_buffer_id, IARG_UINT32, BLACK_LISTED_ID, 0, IARG_END);
		}
	}
}

static void TOOL_instrumentation_img(IMG image, void* arg){
	SEC 				section;
	RTN 				routine;
	struct cm_routine*	cm_rtn;
	uint32_t			img_white_listed;
	uint32_t			rtn_white_listed;

	img_white_listed = (whiteList_search(light_tracer.white_list, IMG_Name(image).c_str()) == 0) ? CODEMAP_WHITELISTED : CODEMAP_NOT_WHITELISTED;
	if (codeMap_add_image(light_tracer.code_map, IMG_LowAddress(image), IMG_HighAddress(image), IMG_Name(image).c_str(), img_white_listed)){
		LOG("ERROR: unable to add image " + IMG_Name(image) + " to code map structure\n");
	}
	else{
		for (section = IMG_SecHead(image); SEC_Valid(section); section = SEC_Next(section)){
			if (SEC_IsExecutable(section) && SEC_Mapped(section)){
				if (codeMap_add_section(light_tracer.code_map, SEC_Address(section), SEC_Address(section) + SEC_Size(section), SEC_Name(section).c_str())){
					LOG("ERROR: unable to add section " + SEC_Name(section) + " to code map structure\n");
					break;
				}
				else{
					for (routine = SEC_RtnHead(section); RTN_Valid(routine); routine = RTN_Next(routine)){
						rtn_white_listed = img_white_listed | (whiteList_search(light_tracer.white_list, RTN_Name(routine).c_str()) == 0) ? CODEMAP_WHITELISTED : CODEMAP_NOT_WHITELISTED;
						cm_rtn = codeMap_add_routine(light_tracer.code_map, RTN_Address(routine), RTN_Address(routine) + RTN_Range(routine), RTN_Name(routine).c_str(), rtn_white_listed);
						if (cm_rtn == NULL){
							LOG("ERROR: unable to add routine " + RTN_Name(routine) + " to code map structure\n");
							break;
						}
						else{
							#ifdef __linux__
							if (!SYM_IFuncResolver(RTN_Sym(routine))){
							#endif
								RTN_Open(routine);
								RTN_InsertCall(routine, IPOINT_BEFORE, (AFUNPTR)TOOL_routine_analysis, IARG_PTR, cm_rtn, IARG_END);
								RTN_Close(routine);
							#ifdef __linux__
							}
							#endif
						}
					}
				}
			}
		}
	}
}

/* ===================================================================== */
/* Call back functions													 */
/* ===================================================================== */

static void* TOOL_block_buffer_full(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, void* buffer, UINT64 numElements, void* arg){
	struct toolThreadData* 	data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);
	uint64_t 				i;
	uint32_t 				rewrite_offset;
	uint32_t* 				buffer_id;

	if (data == NULL){
		LOG("ERROR: thread data is NULL for thread " + decstr(tid) + "\n");
		return buffer;
	}

	if (data->block_file == NULL){
		char file_name[TRACEFILE_NAME_MAX_LENGTH];

		snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/blockId%u_%u.bin", traceFile_get_dir_name(light_tracer.trace_file), PIN_GetPid(), tid);
		data->block_file = fopen(file_name, "wb");
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

	if (data->block_file != NULL){
		fwrite(buffer, sizeof(uint32_t), rewrite_offset, data->block_file);
	}

	return buffer;
}

static void* TOOL_mem_buffer_full(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, void* buffer, UINT64 numElements, void* arg){
	struct toolThreadData* data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);

	if (numElements == 0){
		return buffer;
	}

	if (data == NULL){
		LOG("ERROR: thread data is NULL for thread " + decstr(tid) + "\n");
		return buffer;
	}

	if (data->mem_add_file == NULL){
		char file_name[TRACEFILE_NAME_MAX_LENGTH];

		snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/memAddr%u_%u.bin", traceFile_get_dir_name(light_tracer.trace_file), PIN_GetPid(), tid);
		data->mem_add_file = fopen(file_name, "wb");
	}

	if (data->mem_add_file != NULL){
		fwrite(buffer, sizeof(struct memAddress), numElements, data->mem_add_file);
	}

	return buffer;
}

static void TOOL_thread_start(THREADID tid, CONTEXT *ctxt, INT32 flags, void* arg){
	PIN_SetThreadData(light_tracer.thread_key, calloc(1, sizeof(struct toolThreadData)), tid);
}

static void TOOL_thread_stop(THREADID tid, const CONTEXT *ctxt, INT32 code, void* arg){
	struct toolThreadData* data = (struct toolThreadData*)PIN_GetThreadData(light_tracer.thread_key, tid);

	if (data != NULL){
		if (data->block_file != NULL){
			fclose(data->block_file);
		}
		if (data->mem_add_file != NULL){
			fclose(data->mem_add_file);
		}
		if (data->mem_val_file != NULL){
			fclose(data->mem_val_file);
		}
		free(data);
		PIN_SetThreadData(light_tracer.thread_key, NULL, tid);
	}
}

static void TOOL_fini(INT32 code, void* arg){
	PIN_DeleteThreadDataKey(light_tracer.thread_key);

	traceFile_print_codeMap(light_tracer.trace_file, light_tracer.code_map);
	codeMap_delete(light_tracer.code_map);

	whiteList_delete(light_tracer.white_list);

	traceFile_delete(light_tracer.trace_file);
}

/* ===================================================================== */
/* Init function                                                    	 */
/* ===================================================================== */

static int TOOL_init(const char* trace_dir_name, const char* white_list_file_name, bool trace_memory_add, bool trace_memory_val){
	if (trace_memory_val && !trace_memory_add){
		LOG("WARNING: turning memory address one\n");
		trace_memory_add = true;
	}

	light_tracer.thread_key		= -1;
	light_tracer.code_map 		= NULL;
	light_tracer.white_list 	= NULL;
	light_tracer.trace_file 	= NULL;
	light_tracer.trace_mem_add 	= trace_memory_add;
	light_tracer.trace_mem_val 	= trace_memory_val;

	light_tracer.code_map = codeMap_create();
	if (light_tracer.code_map == NULL){
		LOG("ERROR: unable to create codeMap\n");
		goto fail;
	}

	if (white_list_file_name != NULL && strcmp(white_list_file_name, DEFAULT_WHITE_LIST_FILE_NAME)){
		light_tracer.white_list = whiteList_create(white_list_file_name);
		if (light_tracer.white_list == NULL){
			LOG("ERROR: unable to create shared library whiteList\n");
		}
	}

	light_tracer.trace_file = traceFile_create(trace_dir_name, PIN_GetPid());
	if (light_tracer.trace_file == NULL){
		LOG("ERROR: unable to create traceFile\n");
		goto fail;
	}

	light_tracer.thread_key = PIN_CreateThreadDataKey(NULL);
	if (light_tracer.thread_key == -1){
		LOG("Error: could not create ThreadDataKey\n");
		goto fail;
	}

	light_tracer.block_buffer_id = PIN_DefineTraceBuffer(sizeof(uint32_t), 4, TOOL_block_buffer_full, NULL);
	if(light_tracer.block_buffer_id == BUFFER_ID_INVALID){
		LOG("Error: could not allocate initial buffer\n");
		goto fail;
	}

	light_tracer.mem_buffer_id = PIN_DefineTraceBuffer(sizeof(struct memAddress), 4, TOOL_mem_buffer_full, NULL);
	if(light_tracer.mem_buffer_id == BUFFER_ID_INVALID){
		LOG("Error: could not allocate initial buffer\n");
		goto fail;
	}

	#ifdef __linux__
	if (codeMap_add_vdso(light_tracer.code_map, ((whiteList_search(light_tracer.white_list, "VDSO") == 0) ? CODEMAP_WHITELISTED : CODEMAP_NOT_WHITELISTED))){
		LOG("ERROR: unable to add VDSO to code map\n");
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
	PIN_InitSymbolsAlt(SYMBOL_INFO_MODE(IFUNC_SYMBOLS | DEBUG_OR_EXPORT_SYMBOLS));

	if (PIN_Init(argc, argv)){
		LOG("ERROR: unable to init PIN\n");
		return EXIT_FAILURE;
	}

	if (TOOL_init(knob_trace.Value().c_str(), knob_white_list.Value().c_str(), knob_memory_add.Value(), knob_memory_val.Value())){
		LOG("ERROR: unable to init the TOOL\n");
		return EXIT_FAILURE;
	}

	IMG_AddInstrumentFunction(TOOL_instrumentation_img, NULL);
	TRACE_AddInstrumentFunction(TOOL_instrumentation_trace, NULL);
	PIN_AddThreadStartFunction(TOOL_thread_start, NULL);
	PIN_AddThreadFiniFunction(TOOL_thread_stop, NULL);
	PIN_AddFiniFunction(TOOL_fini, NULL);

	PIN_StartProgram();

	return EXIT_SUCCESS;
}
