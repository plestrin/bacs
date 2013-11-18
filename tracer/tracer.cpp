#include <iostream>
#include <fstream>
#include "pin.H"

#include "tracer.h"
#include "traceFiles.h"

#define DEFAULT_TRACE_FILE_NAME 		"trace"
#define DEFAULT_WHITE_LIST				"0"
#define DEFAULT_WHITE_LIST_FILE_NAME	"NULL"

#define TRACE_FILE_MIN_LENGTH			10

/*
 * Todo list:
 *	- trace format JSON
 *	- la liste des infos qu'il faut enregistrer
 *	- déterminer la façon dont il faut instrumenter le code: une fois par basic bloc ou une fois par instruction
 *	- essayer de faire un fichier pour logger les infos du tracer (ne pas utiliser printf, car mélange avec la sortie standard de l'application a tracer)
 *	- pour l'instant je ne suis pas satisfait de la gestion des KNOB pour la whiteList: il faudrait en faire qu'un seul. En plus afficher l'aide en cas d'erreur
 *	- essayer de ne pas faire trop de truc en statique, car ce n'est pas très beau
 * 	- utiliser une double thread pour l'écriture des traces (avec zlib pour la compression trop fou !!)
 *	- utiliser l'ecriture dans un buffer au lieu de faire une analyse (plus rapide - a voir)
 *	- traiter le cas des instructions qui n'appartiennent pas à une routine et dont l'image est whitelistée
 * 	- 
 * 	- que faire des predicated?
 * 	- 
 *	- d'autres idées sont les bienvenues
 */


struct traceFiles* 	trace = NULL;					/* ne pas laisser en statique */
struct tracer		tracer;							/* ne pas laisser en statique */


/* Je suis bien tenté de virer les KNOB et da faire quelque chose à la mano à la place */
/* Tant qu'à faire on pourrait faire un truc un peu standard le mutualisé avec le programme de trace ?? */
KNOB<string> knob_trace_file_name(KNOB_MODE_WRITEONCE, "pintool", "o", DEFAULT_TRACE_FILE_NAME, "Specify trace file name");
KNOB<BOOL> knob_white_list(KNOB_MODE_WRITEONCE, "pintool", "w", DEFAULT_WHITE_LIST, "(Optional) Shared library white list");
KNOB<string> knob_white_list_file_name(KNOB_MODE_WRITEONCE, "pintool", "whitelist-name", DEFAULT_WHITE_LIST_FILE_NAME, "Specify shared library white list file name");

/* ===================================================================== */
/* Analysis function(s) 	                                             */
/* ===================================================================== */

void pintool_instruction_analysis(ADDRINT pc, ADDRINT pc_next, UINT32 opcode){
	fprintf(trace->ins_file, "{\"pc\":\"%08x\",\"pc_next\":\"%08x\",\"ins\":%u},", pc, pc_next, opcode);
}

void pintool_routine_analysis(void* cm_routine_ptr){
	struct cm_routine* routine = (struct cm_routine*)cm_routine_ptr;
	
	CODEMAP_INCREMENT_ROUTINE_EXE(routine);
}

/* ===================================================================== */
/* Instrumentation function                                              */
/* ===================================================================== */

/* En terme de granularité de l'instrumentation qui peux moins peux le plus - peut être plus rapide pour la whiteliste sinon on fait quelque chose d'aasez systématique */
void pintool_instrumentation_ins(INS instruction, void* arg){
	if (codeMap_is_instruction_whiteListed(tracer.code_map, (unsigned long)INS_Address(instruction)) == CODEMAP_NOT_WHITELISTED){
		INS_InsertCall(instruction, IPOINT_BEFORE, (AFUNPTR)pintool_instruction_analysis, 
			IARG_INST_PTR, 												/* program counter */
			IARG_ADDRINT, INS_NextAddress(instruction), 				/* address of the next instruction */
			IARG_UINT32, INS_Opcode(instruction),						/* opcode */
			IARG_END);
	}
}

void pintool_instrumentation_img(IMG image, void* val){
	SEC 				section;
	RTN 				routine;
	struct cm_routine*	cm_rtn;
	char				white_listed;


	white_listed = (whiteList_search(tracer.white_list, IMG_Name(image).c_str()) == 0)?CODEMAP_WHITELISTED:CODEMAP_NOT_WHITELISTED;
	if (codeMap_add_image(tracer.code_map, IMG_LowAddress(image), IMG_HighAddress(image), IMG_Name(image).c_str(), white_listed)){
		printf("ERROR: in %s, unable to add image to code map structure\n", __func__);
	}
	else{
		for (section= IMG_SecHead(image); SEC_Valid(section); section = SEC_Next(section)){
			if (SEC_IsExecutable(section) && SEC_Mapped(section)){
				if (codeMap_add_section(tracer.code_map, SEC_Address(section), SEC_Address(section) + SEC_Size(section), SEC_Name(section).c_str())){
					printf("ERROR: in %s, unable to add section to code map structure\n", __func__);
					break;
				}
				else{
					for (routine= SEC_RtnHead(section); RTN_Valid(routine); routine = RTN_Next(routine)){
						white_listed |= (whiteList_search(tracer.white_list, RTN_Name(routine).c_str()) == 0)?CODEMAP_WHITELISTED:CODEMAP_NOT_WHITELISTED;
						cm_rtn = codeMap_add_routine(tracer.code_map, RTN_Address(routine), RTN_Address(routine) + RTN_Size(routine), RTN_Name(routine).c_str(), white_listed);
						if (cm_rtn == NULL){
							printf("ERROR: in %s, unable to add routine to code map structure\n", __func__);
							break;
						}
						else{
							RTN_Open(routine);
							RTN_InsertCall(routine, IPOINT_BEFORE, (AFUNPTR)pintool_routine_analysis, IARG_PTR, cm_rtn, IARG_END);
							RTN_Close(routine);
						}
					}
				}
			}
		}
	}
}


/* ===================================================================== */
/* Init function                                                    	 */
/* ===================================================================== */

int pintool_init(const char* trace_dir_name, const char* white_list_file_name){
	trace = traceFiles_create(trace_dir_name);
	if (trace == NULL){
		printf("ERROR: unable to create trace file\n");
		return -1;
	}

	tracer.code_map = codeMap_create();
	if (tracer.code_map == NULL){
		printf("ERROR: unable to create code map\n");
		return -1;
	}

	if (white_list_file_name != NULL){
		tracer.white_list = whiteList_create(white_list_file_name);
		if (tracer.white_list == NULL){
			printf("ERROR: unable to create shared library white list\n");
		}
	}
	else{
		tracer.white_list = NULL;
	}

	return 0;
}


/* ===================================================================== */
/* Cleanup function                                                 	 */
/* ===================================================================== */

void pintool_clean(INT32 code, void* arg){
	traceFiles_print_codeMap(trace, tracer.code_map);
	traceFiles_delete(trace);

	codeMap_delete(tracer.code_map);

    whiteList_delete(tracer.white_list);
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[]){
     PIN_InitSymbols();
	
	if (PIN_Init(argc, argv)){
		printf("ERROR: unable to init PIN\n");
		return -1;
	}

	if (knob_white_list.Value()){
		if (pintool_init(knob_trace_file_name.Value().c_str(), knob_white_list_file_name.Value().c_str())){
			return -1;
		}
	}
	else{
		if (pintool_init(knob_trace_file_name.Value().c_str(), NULL)){
			return -1;
		}
	}
	
    IMG_AddInstrumentFunction(pintool_instrumentation_img, NULL);
    INS_AddInstrumentFunction(pintool_instrumentation_ins, NULL);
    PIN_AddFiniFunction(pintool_clean, NULL);
	
    PIN_StartProgram();
    
    return 0;
}
