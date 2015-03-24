#include <stdlib.h>
#include <stdio.h>

#include "instruction.h"

const char* reg_2_string(enum reg reg){
	switch(reg){
		case REGISTER_INVALID 			: {return "INVALID";}
		case REGISTER_EAX 				: {return "EAX";}
		case REGISTER_AX 				: {return "AX";}
		case REGISTER_AH 				: {return "AH";}
		case REGISTER_AL 				: {return "AL";}
		case REGISTER_EBX 				: {return "EBX";}
		case REGISTER_BX 				: {return "BX";}
		case REGISTER_BH 				: {return "BH";}
		case REGISTER_BL 				: {return "BL";}
		case REGISTER_ECX 				: {return "ECX";}
		case REGISTER_CX 				: {return "CX";}
		case REGISTER_CH 				: {return "CH";}
		case REGISTER_CL 				: {return "CL";}
		case REGISTER_EDX 				: {return "EDX";}
		case REGISTER_DX 				: {return "DX";}
		case REGISTER_DH 				: {return "DH";}
		case REGISTER_DL 				: {return "DL";}
		case REGISTER_ESI 				: {return "ESI";}
		case REGISTER_EDI 				: {return "EDI";}
		case REGISTER_EBP 				: {return "EBP";}
		case REGISTER_ESP 				: {return "ESP";}
	}

	return NULL;
}

int32_t reg_is_contained_in(enum reg reg1, enum reg reg2){
	switch(reg1){
		case REGISTER_INVALID 			: {return 0;}
		case REGISTER_EAX 				: {return (REGISTER_EAX == reg2);}
		case REGISTER_AX 				: {return (REGISTER_EAX == reg2 || REGISTER_AX == reg2);}
		case REGISTER_AH 				: {return (REGISTER_EAX == reg2 || REGISTER_AX == reg2 || REGISTER_AH == reg2);}
		case REGISTER_AL 				: {return (REGISTER_EAX == reg2 || REGISTER_AX == reg2 || REGISTER_AL == reg2);}
		case REGISTER_EBX 				: {return (REGISTER_EBX == reg2);}
		case REGISTER_BX 				: {return (REGISTER_EBX == reg2 || REGISTER_BX == reg2);}
		case REGISTER_BH 				: {return (REGISTER_EBX == reg2 || REGISTER_BX == reg2 || REGISTER_BH == reg2);}
		case REGISTER_BL 				: {return (REGISTER_EBX == reg2 || REGISTER_BX == reg2 || REGISTER_BL == reg2);}
		case REGISTER_ECX 				: {return (REGISTER_ECX == reg2);}
		case REGISTER_CX 				: {return (REGISTER_ECX == reg2 || REGISTER_CX == reg2);}
		case REGISTER_CH 				: {return (REGISTER_ECX == reg2 || REGISTER_CX == reg2 || REGISTER_CH == reg2);}
		case REGISTER_CL 				: {return (REGISTER_ECX == reg2 || REGISTER_CX == reg2 || REGISTER_CL == reg2);}
		case REGISTER_EDX 				: {return (REGISTER_EDX == reg2);}
		case REGISTER_DX 				: {return (REGISTER_EDX == reg2 || REGISTER_DX == reg2);}
		case REGISTER_DH 				: {return (REGISTER_EDX == reg2 || REGISTER_DX == reg2 || REGISTER_DH == reg2);}
		case REGISTER_DL 				: {return (REGISTER_EDX == reg2 || REGISTER_DX == reg2 || REGISTER_DL == reg2);}
		case REGISTER_ESI 				: {return (REGISTER_ESI == reg2);}
		case REGISTER_EDI 				: {return (REGISTER_EDI == reg2);}
		case REGISTER_EBP 				: {return (REGISTER_EBP == reg2);}
		case REGISTER_ESP 				: {return (REGISTER_ESP == reg2);}
	}

	return 0;
}

int8_t reg_get_size(enum reg reg){
	switch(reg){
		case REGISTER_INVALID 			: {return 0;}
		case REGISTER_EAX 				: {return 32;}
		case REGISTER_AX 				: {return 16;}
		case REGISTER_AH 				: {return 8;}
		case REGISTER_AL 				: {return 8;}
		case REGISTER_EBX 				: {return 32;}
		case REGISTER_BX 				: {return 16;}
		case REGISTER_BH 				: {return 8;}
		case REGISTER_BL 				: {return 8;}
		case REGISTER_ECX 				: {return 32;}
		case REGISTER_CX 				: {return 16;}
		case REGISTER_CH 				: {return 8;}
		case REGISTER_CL 				: {return 8;}
		case REGISTER_EDX 				: {return 32;}
		case REGISTER_DX 				: {return 16;}
		case REGISTER_DH 				: {return 8;}
		case REGISTER_DL 				: {return 8;}
		case REGISTER_ESI 				: {return 32;}
		case REGISTER_EDI 				: {return 32;}
		case REGISTER_EBP 				: {return 32;}
		case REGISTER_ESP 				: {return 32;}
	}

	return 0;
}