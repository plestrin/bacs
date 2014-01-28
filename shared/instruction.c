#include <stdlib.h>
#include <stdio.h>

#include "instruction.h"

#ifdef WIN32

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define snprintf(str, size, format, ...) _snprintf_s((str), (size), _TRUNCATE, (format), __VA_ARGS__)

#endif

struct multiColumnPrinter* instruction_init_multiColumnPrinter(){
	struct multiColumnPrinter* printer;

	printer = multiColumnPrinter_create(stdout, 5, NULL, NULL, NULL);
	if (printer != NULL){
		#if defined ARCH_32
		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_HEX_32);
		#elif defined ARCH_64
		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_HEX_64);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_UINT32);

		multiColumnPrinter_set_column_size(printer, 3, 64);
		multiColumnPrinter_set_column_size(printer, 4, 32);

		multiColumnPrinter_set_title(printer, 0, "PC");
		multiColumnPrinter_set_title(printer, 1, "Opcode Val");
		multiColumnPrinter_set_title(printer, 2, "Opcode Str");
		multiColumnPrinter_set_title(printer, 3, "Read Access");
		multiColumnPrinter_set_title(printer, 4, "Write Access");
	}
	else{
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
	}

	return printer;
}

void instruction_print(struct multiColumnPrinter* printer, struct instruction *ins){
	uint32_t i;

	if (ins != NULL){
		if (printer != NULL){
			char 		read_access[MULTICOLUMN_STRING_MAX_SIZE];
			char 		write_access[MULTICOLUMN_STRING_MAX_SIZE];
			uint32_t 	str_read_offset = 0;
			uint32_t 	str_write_offset = 0;
			char* 		current_str;
			uint32_t*	current_offset;
			uint32_t	nb_byte_written;

			read_access[0] = '\0';
			write_access[0] = '\0';

			for ( i = 0; i < INSTRUCTION_MAX_NB_DATA; i++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(ins->data[i].type)){
					if (INSTRUCTION_DATA_TYPE_IS_READ(ins->data[i].type)){
						current_str = read_access + str_read_offset;
						current_offset = &str_read_offset;
					}
					else{
						current_str = write_access + str_write_offset;
						current_offset = &str_write_offset;
					}

					if (INSTRUCTION_DATA_TYPE_IS_MEM(ins->data[i].type)){
						#if defined ARCH_32
						nb_byte_written = snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "{Mem @ %08x ", ins->data[i].location.address);
						#elif defined ARCH_64
						#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
						nb_byte_written = snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "{Mem @ %llx ", ins->data[i].location.address);
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
						current_str += nb_byte_written;
						*current_offset += nb_byte_written;
					}
					else if (INSTRUCTION_DATA_TYPE_IS_REG(ins->data[i].type)){
						nb_byte_written = snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "{%s ", reg_2_string(ins->data[i].location.reg));
						current_str += nb_byte_written;
						*current_offset += nb_byte_written;
					}
					else{
						printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
					}

					switch(ins->data[i].size){
					case 1 	: {*current_offset += snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "0x%02x(1)}", ins->data[i].value & 0x000000ff); break;}
					case 2 	: {*current_offset += snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "0x%04x(2)}", ins->data[i].value & 0x0000ffff); break;}
					case 4 	: {*current_offset += snprintf(current_str, MULTICOLUMN_STRING_MAX_SIZE - *current_offset, "0x%08x(4)}", ins->data[i].value & 0xffffffff); break;}
					default : {printf("ERROR: in %s, unexpected data size\n", __func__); break;}
					}
				}
			}
			multiColumnPrinter_print(printer, ins->pc, ins->opcode, instruction_opcode_2_string(ins->opcode), read_access, write_access, NULL);
		}
		else{
			printf("*** Instruction ***\n");
			#if defined ARCH_32
			printf("\tPC: \t\t0x%08x\n", ins->pc);
			#elif defined ARCH_64
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			printf("\tPC: \t\t0x%llx\n", ins->pc);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
			printf("\tOpcode: \t%s (%u)\n", instruction_opcode_2_string(ins->opcode), ins->opcode);

			for ( i = 0; i < INSTRUCTION_MAX_NB_DATA; i++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(ins->data[i].type)){
					if (INSTRUCTION_DATA_TYPE_IS_READ(ins->data[i].type)){
						printf("\tData read:\n");
					}
					else{
						printf("\tData write:\n");
					}

					if (INSTRUCTION_DATA_TYPE_IS_MEM(ins->data[i].type)){
						#if defined ARCH_32
						printf("\t\tMem: \t0x%08x\n", ins->data[i].location.address);
						#elif defined ARCH_64
						#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
						printf("\t\tMem: \t0x%llx\n", ins->data[i].location.address);
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
					}
					else if (INSTRUCTION_DATA_TYPE_IS_REG(ins->data[i].type)){
						printf("\t\tReg: \t%s\n", reg_2_string(ins->data[i].location.reg));
					}
					else{
						printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
					}


					switch(ins->data[i].size){
					case 1 	: {printf("\t\tValue: \t0x%02x\n", ins->data[i].value & 0x000000ff); break;}
					case 2 	: {printf("\t\tValue: \t0x%04x\n", ins->data[i].value & 0x0000ffff); break;}
					case 4 	: {printf("\t\tValue: \t0x%08x\n", ins->data[i].value & 0xffffffff); break;}
					default : {printf("ERROR: in %s, unexpected data size\n", __func__); break;}
					}
					printf("\t\tSize: \t%u\n", ins->data[i].size);
				}
			}
		}
	}
}

int32_t instruction_compare_pc(struct instruction* ins1, struct instruction* ins2){
	return (int32_t)(ins1->pc - ins2->pc);
}

void instruction_flush_tracer_buffer(FILE* file, struct instruction* buffer, uint32_t nb_instruction){
	uint32_t 	i;
	uint32_t 	j;
	uint8_t 	read_tag = 0;
	uint8_t 	write_tag = 0;

	for (i = 0; i < nb_instruction; i++){
		#if defined ARCH_32
		fprintf(file, "{\"pc\":\"%08x\",\"ins\":%u", buffer[i].pc, buffer[i].opcode);
		#elif defined ARCH_64
		#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
		fprintf(file, "{\"pc\":\"%llx\",\"ins\":%u", buffer[i].pc, buffer[i].opcode);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif

		for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
			if (INSTRUCTION_DATA_TYPE_IS_VALID(buffer[i].data[j].type)){
				if (INSTRUCTION_DATA_TYPE_IS_READ(buffer[i].data[j].type)){
					if (!read_tag){
						fprintf(file, ",\"read\":[");
						read_tag = 1; 
					}
					else{
						fprintf(file, ",");
					}
					if (INSTRUCTION_DATA_TYPE_IS_MEM(buffer[i].data[j].type)){
						#if defined ARCH_32
						fprintf(file, "{\"mem\":\"%08x\",\"val\":\"%08x\",\"size\":%u}", buffer[i].data[j].location.address, buffer[i].data[j].value, buffer[i].data[j].size);
						#elif defined ARCH_64
						#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
						fprintf(file, "{\"mem\":\"%llx\",\"val\":\"%08x\",\"size\":%u}", buffer[i].data[j].location.address, buffer[i].data[j].value, buffer[i].data[j].size);
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
					}
					else{
						fprintf(file, "{\"reg\":%u,\"val\":\"%08x\",\"size\":%u}", buffer[i].data[j].location.reg, buffer[i].data[j].value, buffer[i].data[j].size);
					}
				}
			}
			else{
				break;
			}
		}
		
		for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
			if (INSTRUCTION_DATA_TYPE_IS_VALID(buffer[i].data[j].type)){
				if (INSTRUCTION_DATA_TYPE_IS_WRITE(buffer[i].data[j].type)){
					if (!write_tag && read_tag){
						fprintf(file, "],\"write\":[");
						read_tag = 0;
						write_tag = 1;
					}
					else if (!write_tag && !read_tag){
						fprintf(file, ",\"write\":[");
						write_tag = 1;
					}
					else{
						fprintf(file, ",");
					}
					if (INSTRUCTION_DATA_TYPE_IS_MEM(buffer[i].data[j].type)){
						#if defined ARCH_32
						fprintf(file, "{\"mem\":\"%08x\",\"val\":\"%08x\",\"size\":%u}", buffer[i].data[j].location.address, buffer[i].data[j].value, buffer[i].data[j].size);
						#elif defined ARCH_64
						#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
						fprintf(file, "{\"mem\":\"%llx\",\"val\":\"%08x\",\"size\":%u}", buffer[i].data[j].location.address, buffer[i].data[j].value, buffer[i].data[j].size);
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
					}
					else{
						fprintf(file, "{\"reg\":%u,\"val\":\"%08x\",\"size\":%u}", buffer[i].data[j].location.reg, buffer[i].data[j].value, buffer[i].data[j].size);
					}
				}
			}
			else{
				break;
			}
		}
		if (read_tag || write_tag){
			read_tag = 0;
			write_tag = 0;
			fprintf(file, "]},");
		}
		else{
			fprintf(file, "},");
		}
	}

	return;
}

const char* instruction_opcode_2_string(uint32_t opcode){
	switch((xed_iclass_enum_t)opcode){
	case XED_ICLASS_INVALID 	: {return "INVALID";}
	case XED_ICLASS_AAA 		: {return "AAA";}
	case XED_ICLASS_AAD 		: {return "AAD";}
	case XED_ICLASS_AAM 		: {return "AAM";}
	case XED_ICLASS_AAS 		: {return "AAS";}
	case XED_ICLASS_ADC 		: {return "ADC";}
	case XED_ICLASS_ADCX 		: {return "ADCX";}
	case XED_ICLASS_ADD 		: {return "ADD";}
	case XED_ICLASS_ADDPD 		: {return "ADDPD";}
	case XED_ICLASS_ADDPS 		: {return "ADDPS";}
	case XED_ICLASS_ADDSD 		: {return "ADDSD";}
	case XED_ICLASS_ADDSS 		: {return "ADDSS";}
	case XED_ICLASS_ADDSUBPD 	: {return "ADDSUBPD";}
	case XED_ICLASS_ADDSUBPS 	: {return "ADDSUBPS";}
	case XED_ICLASS_ADOX 		: {return "ADOX";}
	case XED_ICLASS_AESDEC 		: {return "AESDEC";}
	case XED_ICLASS_AESDECLAST 	: {return "AESDECLAST";}
	case XED_ICLASS_AESENC 		: {return "AESENC";}
	case XED_ICLASS_AESENCLAST 	: {return "AESENCLAST";}
	case XED_ICLASS_AESIMC 		: {return "AESIMC";}
	case XED_ICLASS_AESKEYGENASSIST : {return "AESKEYGENASSIST";}
	case XED_ICLASS_AND 		: {return "AND";}
	case XED_ICLASS_ANDN 		: {return "ANDN";}
	case XED_ICLASS_ANDNPD 		: {return "ANDNPD";}
	case XED_ICLASS_ANDNPS 		: {return "ANDNPS";}
	case XED_ICLASS_ANDPD 		: {return "ANDPD";}
	case XED_ICLASS_ANDPS 		: {return "ANDPS";}
	case XED_ICLASS_ARPL 		: {return "ARPL";}
	case XED_ICLASS_BEXTR 		: {return "BEXTR";}
	case XED_ICLASS_BEXTR_XOP 	: {return "BEXTR_XOP";}
	case XED_ICLASS_BLCFILL 	: {return "BLCFILL";}
	case XED_ICLASS_BLCI 		: {return "BLCI";}
	case XED_ICLASS_BLCIC 		: {return "BLCIC";}
	case XED_ICLASS_BLCMSK 		: {return "BLCMSK";}
	case XED_ICLASS_BLCS 		: {return "BLCS";}
	case XED_ICLASS_BLENDPD 	: {return "BLENDPD";}
	case XED_ICLASS_BLENDPS 	: {return "BLENDPS";}
	case XED_ICLASS_BLENDVPD 	: {return "BLENDVPD";}
	case XED_ICLASS_BLENDVPS 	: {return "BLENDVPS";}
	case XED_ICLASS_BLSFILL 	: {return "BLSFILL";}
	case XED_ICLASS_BLSI 		: {return "BLSI";}
	case XED_ICLASS_BLSIC 		: {return "BLSIC";}
	case XED_ICLASS_BLSMSK 		: {return "BLSMSK";}
	case XED_ICLASS_BLSR 		: {return "BLSR";}
	case XED_ICLASS_BOUND 		: {return "BOUND";}
	case XED_ICLASS_BSF 		: {return "BSF";}
	case XED_ICLASS_BSR 		: {return "BSR";}
	case XED_ICLASS_BSWAP 		: {return "BSWAP";}
	case XED_ICLASS_BT 			: {return "BT";}
	case XED_ICLASS_BTC 		: {return "BTC";}
	case XED_ICLASS_BTR 		: {return "BTR";}
	case XED_ICLASS_BTS 		: {return "BTS";}
	case XED_ICLASS_BZHI 		: {return "BZHI";}
	case XED_ICLASS_CALL_FAR 	: {return "CALL_FAR";}
	case XED_ICLASS_CALL_NEAR 	: {return "CALL_NEAR";}
	case XED_ICLASS_CBW 		: {return "CBW";}
	case XED_ICLASS_CDQ 		: {return "CDQ";}
	case XED_ICLASS_CDQE 		: {return "CDQE";}
	case XED_ICLASS_CLAC 		: {return "CLAC";}
	case XED_ICLASS_CLC 		: {return "CLC";}
	case XED_ICLASS_CLD 		: {return "CLD";}
	case XED_ICLASS_CLFLUSH 	: {return "CLFLUSH";}
	case XED_ICLASS_CLGI 		: {return "CLGI";}
	case XED_ICLASS_CLI 		: {return "CLI";}
	case XED_ICLASS_CLTS 		: {return "CLTS";}
	case XED_ICLASS_CMC 		: {return "CMC";}
	case XED_ICLASS_CMOVB 		: {return "CMOVB";}
	case XED_ICLASS_CMOVBE 		: {return "CMOVBE";}
	case XED_ICLASS_CMOVL 		: {return "CMOVL";}
	case XED_ICLASS_CMOVLE 		: {return "CMOVLE";}
	case XED_ICLASS_CMOVNB 		: {return "CMOVNB";}
	case XED_ICLASS_CMOVNBE 	: {return "CMOVNBE";}
	case XED_ICLASS_CMOVNL 		: {return "CMOVNL";}
	case XED_ICLASS_CMOVNLE 	: {return "CMOVNLE";}
	case XED_ICLASS_CMOVNO 		: {return "CMOVNO";}
	case XED_ICLASS_CMOVNP 		: {return "CMOVNP";}
	case XED_ICLASS_CMOVNS 		: {return "CMOVNS";}
	case XED_ICLASS_CMOVNZ 		: {return "CMOVNZ";}
	case XED_ICLASS_CMOVO 		: {return "CMOVO";}
	case XED_ICLASS_CMOVP 		: {return "CMOVP";}
	case XED_ICLASS_CMOVS 		: {return "CMOVS";}
	case XED_ICLASS_CMOVZ 		: {return "CMOVZ";}
	case XED_ICLASS_CMP 		: {return "CMP";}
	case XED_ICLASS_CMPPD 		: {return "CMPPD";}
	case XED_ICLASS_CMPPS 		: {return "CMPPS";}
	case XED_ICLASS_CMPSB 		: {return "CMPSB";}
	case XED_ICLASS_CMPSD 		: {return "CMPSD";}
	case XED_ICLASS_CMPSD_XMM 	: {return "CMPSD_XMM";}
	case XED_ICLASS_CMPSQ 		: {return "CMPSQ";}
	case XED_ICLASS_CMPSS 		: {return "CMPSS";}
	case XED_ICLASS_CMPSW 		: {return "CMPSW";}
	case XED_ICLASS_CMPXCHG 	: {return "CMPXCHG";}
	case XED_ICLASS_CMPXCHG16B 	: {return "CMPXCHG16B";}
	case XED_ICLASS_CMPXCHG8B 	: {return "CMPXCH8B";}
	case XED_ICLASS_COMISD 		: {return "COMISD";}
	case XED_ICLASS_COMISS 		: {return "COMISS";}
	case XED_ICLASS_CPUID 		: {return "CPUID";}
	case XED_ICLASS_CQO 		: {return "CQO";}
	case XED_ICLASS_CRC32 		: {return "CRC32";}
	case XED_ICLASS_CVTDQ2PD 	: {return "CVTDQ2PD";}
	case XED_ICLASS_CVTDQ2PS 	: {return "CVTDQ2PS";}
	case XED_ICLASS_CVTPD2DQ 	: {return "CVTPD2DQ";}
	case XED_ICLASS_CVTPD2PI 	: {return "CVTPD2PI";}
	case XED_ICLASS_CVTPD2PS 	: {return "CVTPD2PS";}
	case XED_ICLASS_CVTPI2PD 	: {return "CVTPI2PD";}
	case XED_ICLASS_CVTPI2PS 	: {return "CVTPI2PS";}
	case XED_ICLASS_CVTPS2DQ 	: {return "CVTPS2DQ";}
	case XED_ICLASS_CVTPS2PD 	: {return "CVTPS2PD";}
	case XED_ICLASS_CVTPS2PI 	: {return "CVTPS2PI";}
	case XED_ICLASS_CVTSD2SI 	: {return "CVTSD2SI";}
	case XED_ICLASS_CVTSD2SS 	: {return "CVTSD2SS";}
	case XED_ICLASS_CVTSI2SD 	: {return "CVTSI2SD";}
	case XED_ICLASS_CVTSI2SS 	: {return "CVTSI2SS";}
	case XED_ICLASS_CVTSS2SD 	: {return "CVTSS2SD";}
	case XED_ICLASS_CVTSS2SI 	: {return "CVTSS2SI";}
	case XED_ICLASS_CVTTPD2DQ 	: {return "CVTTPD2DQ";}
	case XED_ICLASS_CVTTPD2PI 	: {return "CVTTPD2PI";}
	case XED_ICLASS_CVTTPS2DQ 	: {return "CVTTPS2DQ";}
	case XED_ICLASS_CVTTPS2PI 	: {return "CVTTPS2PI";}
	case XED_ICLASS_CVTTSD2SI 	: {return "CVTTSD2SI";}
	case XED_ICLASS_CVTTSS2SI 	: {return "CVTTSS2SI";}
	case XED_ICLASS_CWD 		: {return "CWD";}
	case XED_ICLASS_CWDE 		: {return "CWDE";}
	case XED_ICLASS_DAA 		: {return "DAA";}
	case XED_ICLASS_DAS 		: {return "DAS";}
	case XED_ICLASS_DEC 		: {return "DEC";}
	case XED_ICLASS_DIV 		: {return "DIV";}
	case XED_ICLASS_DIVPD 		: {return "DIVPD";}
	case XED_ICLASS_DIVPS 		: {return "DIVPS";}
	case XED_ICLASS_DIVSD 		: {return "DIVSD";}
	case XED_ICLASS_DIVSS 		: {return "DIVSS";}
	case XED_ICLASS_DPPD 		: {return "DPPD";}
	case XED_ICLASS_DPPS 		: {return "DPPS";}
	case XED_ICLASS_EMMS 		: {return "EMMS";}
	case XED_ICLASS_ENTER 		: {return "ENTER";}
	case XED_ICLASS_EXTRACTPS 	: {return "EXTRACTPS";}
	case XED_ICLASS_EXTRQ 		: {return "EXTRQ";}
	case XED_ICLASS_F2XM1 		: {return "F2XM1";}
	case XED_ICLASS_FABS 		: {return "FABS";}
	case XED_ICLASS_FADD 		: {return "FADD";}
	case XED_ICLASS_FADDP 		: {return "FADDP";}
	case XED_ICLASS_FBLD 		: {return "FBLD";}
	case XED_ICLASS_FBSTP 		: {return "FBSTP";}
	case XED_ICLASS_FCHS 		: {return "FCHS";}
	case XED_ICLASS_FCMOVB 		: {return "FCMOVB";}
	case XED_ICLASS_FCMOVBE 	: {return "FCMOVBE";}
	case XED_ICLASS_FCMOVE 		: {return "FCMOVE";}
	case XED_ICLASS_FCMOVNB 	: {return "FCMOVNB";}
	case XED_ICLASS_FCMOVNBE 	: {return "FCMOVNBE";}
	case XED_ICLASS_FCMOVNE 	: {return "FCMOVNE";}
	case XED_ICLASS_FCMOVNU 	: {return "FCMOVNU";}
	case XED_ICLASS_FCMOVU 		: {return "FCMOVU";}
	case XED_ICLASS_FCOM 		: {return "FCOM";}
	case XED_ICLASS_FCOMI 		: {return "FCOMI";}
	case XED_ICLASS_FCOMIP 		: {return "FCOMIP";}
	case XED_ICLASS_FCOMP 		: {return "FCOMP";}
	case XED_ICLASS_FCOMPP 		: {return "FCOMPP";}
	case XED_ICLASS_FCOS 		: {return "FCOS";}
	case XED_ICLASS_FDECSTP 	: {return "FDECSTP";}
	case XED_ICLASS_FDISI8087_NOP : {return "FDISI8087_NOP";}
	case XED_ICLASS_FDIV 		: {return "FDIV";}
	case XED_ICLASS_FDIVP 		: {return "FDIVP";}
	case XED_ICLASS_FDIVR 		: {return "FDIVR";}
	case XED_ICLASS_FDIVRP 		: {return "FDIVRP";}
	case XED_ICLASS_FEMMS 		: {return "FEMMS";}
	case XED_ICLASS_FENI8087_NOP : {return "FENI8087_NOP";}
	case XED_ICLASS_FFREE 		: {return "FFREE";}
	case XED_ICLASS_FFREEP 		: {return "FFREEP";}
	case XED_ICLASS_FIADD 		: {return "FIADD";}
	case XED_ICLASS_FICOM 		: {return "FICOM";}
	case XED_ICLASS_FICOMP 		: {return "FICOMP";}
	case XED_ICLASS_FIDIV 		: {return "FIDIV";}
	case XED_ICLASS_FIDIVR 		: {return "FIDIVR";}
	case XED_ICLASS_FILD 		: {return "FILD";}
	case XED_ICLASS_FIMUL 		: {return "FIMUL";}
	case XED_ICLASS_FINCSTP 	: {return "FINCSTP";}
	case XED_ICLASS_FIST 		: {return "FIST";}
	case XED_ICLASS_FISTP 		: {return "FISTP";}
	case XED_ICLASS_FISTTP 		: {return "FISTTP";}
	case XED_ICLASS_FISUB 		: {return "FISUB";}
	case XED_ICLASS_FISUBR 		: {return "FISUBR";}
	case XED_ICLASS_FLD 		: {return "FLD";}
	case XED_ICLASS_FLD1 		: {return "FLD1";}
	case XED_ICLASS_FLDCW 		: {return "FLDCW";}
	case XED_ICLASS_FLDENV 		: {return "FLDENV";}
	case XED_ICLASS_FLDL2E 		: {return "FLDL2E";}
	case XED_ICLASS_FLDL2T 		: {return "FLDL2T";}
	case XED_ICLASS_FLDLG2 		: {return "FLDLG2";}
	case XED_ICLASS_FLDLN2 		: {return "FLDLN2";}
	case XED_ICLASS_FLDPI 		: {return "FLDPI";}
	case XED_ICLASS_FLDZ 		: {return "FLDZ";}
	case XED_ICLASS_FMUL 		: {return "FMUL";}
	case XED_ICLASS_FMULP 		: {return "FMULP";}
	case XED_ICLASS_FNCLEX 		: {return "FNCLEX";}
	case XED_ICLASS_FNINIT 		: {return "FNINIT";}
	case XED_ICLASS_FNOP 		: {return "FNOP";}
	case XED_ICLASS_FNSAVE 		: {return "FNSAVE";}
	case XED_ICLASS_FNSTCW 		: {return "FNSTCW";}
	case XED_ICLASS_FNSTENV 	: {return "FNSTENV";}
	case XED_ICLASS_FNSTSW 		: {return "FNSTSW";}
	case XED_ICLASS_FPATAN 		: {return "FPATAN";}
	case XED_ICLASS_FPREM 		: {return "FPREM";}
	case XED_ICLASS_FPREM1 		: {return "FPREM1";}
	case XED_ICLASS_FPTAN 		: {return "FPTAN";}
	case XED_ICLASS_FRNDINT 	: {return "FRNDINT";}
	case XED_ICLASS_FRSTOR 		: {return "FRSTOR";}
	case XED_ICLASS_FSCALE 		: {return "FSCALE";}
	case XED_ICLASS_FSETPM287_NOP : {return "FSETPM287_NOP";}
	case XED_ICLASS_FSIN 		: {return "FSIN";}
	case XED_ICLASS_FSINCOS 	: {return "FSINCOS";}
	case XED_ICLASS_FSQRT 		: {return "FSQRT";}
	case XED_ICLASS_FST 		: {return "FST";}
	case XED_ICLASS_FSTP 		: {return "FSTP";}
	case XED_ICLASS_FSTPNCE 	: {return "FSTPNCE";}
	case XED_ICLASS_FSUB 		: {return "FSUB";}
	case XED_ICLASS_FSUBP 		: {return "FSUBP";}
	case XED_ICLASS_FSUBR 		: {return "FSUBR";}
	case XED_ICLASS_FSUBRP 		: {return "FSUBRP";}
	case XED_ICLASS_FTST 		: {return "FTST";}
	case XED_ICLASS_FUCOM 		: {return "FUCOM";}
	case XED_ICLASS_FUCOMI 		: {return "FUCOMI";}
	case XED_ICLASS_FUCOMIP 	: {return "FUCOMIP";}
	case XED_ICLASS_FUCOMP 		: {return "FUCOMP";}
	case XED_ICLASS_FUCOMPP 	: {return "FUCOMPP";}
	case XED_ICLASS_FWAIT 		: {return "FWAIT";}
	case XED_ICLASS_FXAM 		: {return "FXAM";}
	case XED_ICLASS_FXCH 		: {return "FXCH";}
	case XED_ICLASS_FXRSTOR 	: {return "FXRSTOR";}
	case XED_ICLASS_FXRSTOR64 	: {return "FXRSTOR64";}
	case XED_ICLASS_FXSAVE 		: {return "FXSAVE";}
	case XED_ICLASS_FXSAVE64 	: {return "FXSAVE64";}
	case XED_ICLASS_FXTRACT 	: {return "FXTRACT";}
	case XED_ICLASS_FYL2X 		: {return "FYL2X";}
	case XED_ICLASS_FYL2XP1 	: {return "FYL2XP1";}
	case XED_ICLASS_GETSEC 		: {return "GETSEC";}
	case XED_ICLASS_HADDPD 		: {return "HADDPD";}
	case XED_ICLASS_HADDPS 		: {return "HADDPS";}
	case XED_ICLASS_HLT 		: {return "HLT";}
	case XED_ICLASS_HSUBPD 		: {return "HSUBPD";}
	case XED_ICLASS_HSUBPS 		: {return "HSUBPS";}
	case XED_ICLASS_IDIV 		: {return "IDIV";}
	case XED_ICLASS_IMUL 		: {return "IMUL";}
	case XED_ICLASS_IN 			: {return "IN";}
	case XED_ICLASS_INC 		: {return "INC";}
	case XED_ICLASS_INSB 		: {return "INSB";}
	case XED_ICLASS_INSD 		: {return "INSD";}
	case XED_ICLASS_INSERTPS 	: {return "INSERTPS";}
	case XED_ICLASS_INSERTQ 	: {return "INSERTQ";}
	case XED_ICLASS_INSW 		: {return "INSW";}
	case XED_ICLASS_INT 		: {return "INT";}
	case XED_ICLASS_INT1 		: {return "INT1";}
	case XED_ICLASS_INT3 		: {return "INT3";}
	case XED_ICLASS_INTO 		: {return "INTO";}
	case XED_ICLASS_INVD 		: {return "INVD";}
	case XED_ICLASS_INVEPT 		: {return "INVEPT";}
	case XED_ICLASS_INVLPG 		: {return "INVLPG";}
	case XED_ICLASS_INVLPGA 	: {return "INVLPGA";}
	case XED_ICLASS_INVPCID 	: {return "INVPCID";}
	case XED_ICLASS_INVVPID 	: {return "INVVPID";}
	case XED_ICLASS_IRET 		: {return "IRET";}
	case XED_ICLASS_IRETD 		: {return "IRETD";}
	case XED_ICLASS_IRETQ 		: {return "IRETQ";}
	case XED_ICLASS_JB 			: {return "JB";}
	case XED_ICLASS_JBE 		: {return "JBE";}
	case XED_ICLASS_JL 			: {return "JL";}
	case XED_ICLASS_JLE 		: {return "JLE";}
	case XED_ICLASS_JMP 		: {return "JMP";}
	case XED_ICLASS_JMP_FAR 	: {return "JMP_FAR";}
	case XED_ICLASS_JNB 		: {return "JNB";}
	case XED_ICLASS_JNBE 		: {return "JNBE";}
	case XED_ICLASS_JNL 		: {return "JNL";}
	case XED_ICLASS_JNLE 		: {return "JNLE";}
	case XED_ICLASS_JNO 		: {return "JNO";}
	case XED_ICLASS_JNP 		: {return "JNP";}
	case XED_ICLASS_JNS 		: {return "JNS";}
	case XED_ICLASS_JNZ 		: {return "JNZ";}
	case XED_ICLASS_JO 			: {return "JO";}
	case XED_ICLASS_JP 			: {return "JP";}
	case XED_ICLASS_JRCXZ 		: {return "JRCXZ";}
	case XED_ICLASS_JS 			: {return "JS";}
	case XED_ICLASS_JZ 			: {return "JZ";}
	case XED_ICLASS_LAHF 		: {return "LAHF";}
	case XED_ICLASS_LAR 		: {return "LAR";}
	case XED_ICLASS_LDDQU 		: {return "LDDQU";}
	case XED_ICLASS_LDMXCSR 	: {return "LDMXCSR";}
	case XED_ICLASS_LDS 		: {return "LDS";}
	case XED_ICLASS_LEA 		: {return "LEA";}
	case XED_ICLASS_LEAVE 		: {return "LEAVE";}
	case XED_ICLASS_LES 		: {return "LES";}
	case XED_ICLASS_LFENCE 		: {return "LFENCE";}
	case XED_ICLASS_LFS 		: {return "LFS";}
	case XED_ICLASS_LGDT 		: {return "LGDT";}
	case XED_ICLASS_LGS 		: {return "LGS";}
	case XED_ICLASS_LIDT 		: {return "LIDT";}
	case XED_ICLASS_LLDT 		: {return "LLDT";}
	case XED_ICLASS_LLWPCB 		: {return "LLWPCB";}
	case XED_ICLASS_LMSW 		: {return "LMSW";}
	case XED_ICLASS_LODSB 		: {return "LODSB";}
	case XED_ICLASS_LODSD 		: {return "LODSD";}
	case XED_ICLASS_LODSQ 		: {return "LODSQ";}
	case XED_ICLASS_LODSW 		: {return "LODSW";}
	case XED_ICLASS_LOOP 		: {return "LOOP";}
	case XED_ICLASS_LOOPE 		: {return "LOOPE";}
	case XED_ICLASS_LOOPNE 		: {return "LOOPNE";}
	case XED_ICLASS_LSL 		: {return "LSL";}
	case XED_ICLASS_LSS 		: {return "LSS";}
	case XED_ICLASS_LTR 		: {return "LTR";}
	case XED_ICLASS_LWPINS 		: {return "LWPINS";}
	case XED_ICLASS_LWPVAL 		: {return "LWPVAL";}
	case XED_ICLASS_LZCNT 		: {return "LZCNT";}
	case XED_ICLASS_MASKMOVDQU 	: {return "MASKMOVDQU";}
	case XED_ICLASS_MASKMOVQ 	: {return "MASKMOVQ";}
	case XED_ICLASS_MAXPD 		: {return "MAXPD";}
	case XED_ICLASS_MAXPS 		: {return "MAXPS";}
	case XED_ICLASS_MAXSD 		: {return "MAXSD";}
	case XED_ICLASS_MAXSS 		: {return "MAXSS";}
	case XED_ICLASS_MFENCE 		: {return "MFENCE";}
	case XED_ICLASS_MINPD 		: {return "MINPD";}
	case XED_ICLASS_MINPS 		: {return "MINPS";}
	case XED_ICLASS_MINSD 		: {return "MINSD";}
	case XED_ICLASS_MINSS 		: {return "MINSS";}
	case XED_ICLASS_MONITOR 	: {return "MONITOR";}
	case XED_ICLASS_MOV 		: {return "MOV";}
	case XED_ICLASS_MOVAPD 		: {return "MOVAPD";}
	case XED_ICLASS_MOVAPS 		: {return "MOVAPS";}
	case XED_ICLASS_MOVBE 		: {return "MOVBE";}
	case XED_ICLASS_MOVD 		: {return "MOVD";}
	case XED_ICLASS_MOVDDUP 	: {return "MOVDDUP";}
	case XED_ICLASS_MOVDQ2Q 	: {return "MOVDQ2Q";}
	case XED_ICLASS_MOVDQA 		: {return "MOVDQA";}
	case XED_ICLASS_MOVDQU 		: {return "MOVDQU";}
	case XED_ICLASS_MOVHLPS 	: {return "MOVHLPS";}
	case XED_ICLASS_MOVHPD 		: {return "MOVHPD";}
	case XED_ICLASS_MOVHPS 		: {return "MOVHPS";}
	case XED_ICLASS_MOVLHPS 	: {return "MOVLHPS";}
	case XED_ICLASS_MOVLPD 		: {return "MOVLPD";}
	case XED_ICLASS_MOVLPS 		: {return "MOVLPS";}
	case XED_ICLASS_MOVMSKPD 	: {return "MOVMSKPD";}
	case XED_ICLASS_MOVMSKPS 	: {return "MOVMSKPS";}
	case XED_ICLASS_MOVNTDQ 	: {return "MOVNTDQ";}
	case XED_ICLASS_MOVNTDQA 	: {return "MOVNTDQA";}
	case XED_ICLASS_MOVNTI 		: {return "MOVNTI";}
	case XED_ICLASS_MOVNTPD 	: {return "MOVNTPD";}
	case XED_ICLASS_MOVNTPS 	: {return "MOVNTPS";}
	case XED_ICLASS_MOVNTQ 		: {return "MOVNTQ";}
	case XED_ICLASS_MOVNTSD 	: {return "MOVNTSD";}
	case XED_ICLASS_MOVNTSS 	: {return "MOVNTSS";}
	case XED_ICLASS_MOVQ 		: {return "MOVQ";}
	case XED_ICLASS_MOVQ2DQ 	: {return "MOVQ2DQ";}
	case XED_ICLASS_MOVSB 		: {return "MOVSB";}
	case XED_ICLASS_MOVSD 		: {return "MOVSD";}
	case XED_ICLASS_MOVSD_XMM 	: {return "MOVSD_XMM";}
	case XED_ICLASS_MOVSHDUP 	: {return "MOVSHDUP";}
	case XED_ICLASS_MOVSLDUP 	: {return "MOVSLDUP";}
	case XED_ICLASS_MOVSQ 		: {return "MOVSQ";}
	case XED_ICLASS_MOVSS 		: {return "MOVSS";}
	case XED_ICLASS_MOVSW 		: {return "MOVSW";}
	case XED_ICLASS_MOVSX 		: {return "MOVSX";}
	case XED_ICLASS_MOVSXD 		: {return "MOVSXD";}
	case XED_ICLASS_MOVUPD 		: {return "MOVUPD";}
	case XED_ICLASS_MOVUPS 		: {return "MOVUPS";}
	case XED_ICLASS_MOVZX 		: {return "MOVZX";}
	case XED_ICLASS_MOV_CR 		: {return "MOV_CR";}
	case XED_ICLASS_MOV_DR 		: {return "MOV_DR";}
	case XED_ICLASS_MPSADBW 	: {return "MPSADBW";}
	case XED_ICLASS_MUL 		: {return "MUL";}
	case XED_ICLASS_MULPD 		: {return "MULPD";}
	case XED_ICLASS_MULPS 		: {return "MULPS";}
	case XED_ICLASS_MULSD 		: {return "MULSD";}
	case XED_ICLASS_MULSS 		: {return "MULSS";}
	case XED_ICLASS_MULX 		: {return "MULX";}
	case XED_ICLASS_MWAIT 		: {return "MWAIT";}
	case XED_ICLASS_NEG 		: {return "NEG";}
	case XED_ICLASS_NOP 		: {return "NOP";}
	case XED_ICLASS_NOP2 		: {return "NOP2";}
	case XED_ICLASS_NOP3 		: {return "NOP3";}
	case XED_ICLASS_NOP4 		: {return "NOP4";}
	case XED_ICLASS_NOP5 		: {return "NOP5";}
	case XED_ICLASS_NOP6 		: {return "NOP6";}
	case XED_ICLASS_NOP7 		: {return "NOP7";}
	case XED_ICLASS_NOP8 		: {return "NOP8";}
	case XED_ICLASS_NOP9 		: {return "NOP9";}
	case XED_ICLASS_NOT 		: {return "NOT";}
	case XED_ICLASS_OR 			: {return "OR";}
	case XED_ICLASS_ORPD 		: {return "ORPD";}
	case XED_ICLASS_ORPS 		: {return "ORPS";}
	case XED_ICLASS_OUT 		: {return "OUT";}
	case XED_ICLASS_OUTSB 		: {return "OUTSB";}
	case XED_ICLASS_OUTSD 		: {return "OUTSD";}
	case XED_ICLASS_OUTSW 		: {return "OUTSW";}
	case XED_ICLASS_PABSB 		: {return "PABSB";}
	case XED_ICLASS_PABSD 		: {return "PABSD";}
	case XED_ICLASS_PABSW 		: {return "PABSW";}
	case XED_ICLASS_PACKSSDW 	: {return "PACKSSDW";}
	case XED_ICLASS_PACKSSWB 	: {return "PACKSSWB";}
	case XED_ICLASS_PACKUSDW 	: {return "PACKUSDW";}
	case XED_ICLASS_PACKUSWB 	: {return "PACKUSWB";}
	case XED_ICLASS_PADDB 		: {return "PADDB";}
	case XED_ICLASS_PADDD 		: {return "PADDD";}
	case XED_ICLASS_PADDQ 		: {return "PADDQ";}
	case XED_ICLASS_PADDSB 		: {return "PADDSB";}
	case XED_ICLASS_PADDSW 		: {return "PADDSW";}
	case XED_ICLASS_PADDUSB 	: {return "PADDUSB";}
	case XED_ICLASS_PADDUSW 	: {return "PADDUSW";}
	case XED_ICLASS_PADDW 		: {return "PADDW";}
	case XED_ICLASS_PALIGNR 	: {return "PALIGNR";}
	case XED_ICLASS_PAND 		: {return "PAND";}
	case XED_ICLASS_PANDN 		: {return "PANDN";}
	case XED_ICLASS_PAUSE 		: {return "PAUSE";}
	case XED_ICLASS_PAVGB 		: {return "PAVGB";}
	case XED_ICLASS_PAVGUSB 	: {return "PAVGUSB";}
	case XED_ICLASS_PAVGW 		: {return "PAVGW";}
	case XED_ICLASS_PBLENDVB 	: {return "PBLENDVB";}
	case XED_ICLASS_PBLENDW 	: {return "PBLENDW";}
	case XED_ICLASS_PCLMULQDQ 	: {return "PCLMULQDQ";}
	case XED_ICLASS_PCMPEQB 	: {return "PCMPEQB";}
	case XED_ICLASS_PCMPEQD 	: {return "PCMPEQD";}
	case XED_ICLASS_PCMPEQQ 	: {return "PCMPEQQ";}
	case XED_ICLASS_PCMPEQW 	: {return "PCMPEQW";}
	case XED_ICLASS_PCMPESTRI 	: {return "PCMPESTRI";}
	case XED_ICLASS_PCMPESTRM 	: {return "PCMPESTRM";}
	case XED_ICLASS_PCMPGTB 	: {return "PCMPGTB";}
	case XED_ICLASS_PCMPGTD 	: {return "PCMPGTD";}
	case XED_ICLASS_PCMPGTQ 	: {return "PCMPGTQ";}
	case XED_ICLASS_PCMPGTW 	: {return "PCMPGTW";}
	case XED_ICLASS_PCMPISTRI 	: {return "PCMPISTRI";}
	case XED_ICLASS_PCMPISTRM 	: {return "PCMPISTRM";}
	case XED_ICLASS_PDEP 		: {return "PDEP";}
	case XED_ICLASS_PEXT 		: {return "PEXT";}
	case XED_ICLASS_PEXTRB 		: {return "PEXTRB";}
	case XED_ICLASS_PEXTRD 		: {return "PEXTRD";}
	case XED_ICLASS_PEXTRQ 		: {return "PEXTRQ";}
	case XED_ICLASS_PEXTRW 		: {return "PEXTRW";}
	case XED_ICLASS_PF2ID 		: {return "PF2ID";}
	case XED_ICLASS_PF2IW 		: {return "PF2IW";}
	case XED_ICLASS_PFACC 		: {return "PFACC";}
	case XED_ICLASS_PFADD 		: {return "PFADD";}
	case XED_ICLASS_PFCMPEQ 	: {return "PFCMPEQ";}
	case XED_ICLASS_PFCMPGE 	: {return "PFCMPGE";}
	case XED_ICLASS_PFCMPGT 	: {return "PFCMPGT";}
	case XED_ICLASS_PFCPIT1 	: {return "PFCPIT1";}
	case XED_ICLASS_PFMAX 		: {return "PFMAX";}
	case XED_ICLASS_PFMIN 		: {return "PFMIN";}
	case XED_ICLASS_PFMUL 		: {return "PFMUL";}
	case XED_ICLASS_PFNACC 		: {return "PFNACC";}
	case XED_ICLASS_PFPNACC 	: {return "PFPNACC";}
	case XED_ICLASS_PFRCP 		: {return "PFRCP";}
	case XED_ICLASS_PFRCPIT2 	: {return "PFRCPIT2";}
	case XED_ICLASS_PFRSQIT1 	: {return "PFRSQIT1";}
	case XED_ICLASS_PFSQRT 		: {return "PFSQRT";}
	case XED_ICLASS_PFSUB 		: {return "PFSUB";}
	case XED_ICLASS_PFSUBR 		: {return "PFSUBR";}
	case XED_ICLASS_PHADDD 		: {return "PHADDD";}
	case XED_ICLASS_PHADDSW 	: {return "PHADDSW";}
	case XED_ICLASS_PHADDW 		: {return "PHADDW";}
	case XED_ICLASS_PHMINPOSUW 	: {return "PHMINPOSUW";}
	case XED_ICLASS_PHSUBD 		: {return "PHSUBD";}
	case XED_ICLASS_PHSUBSW 	: {return "PHSUBSW";}
	case XED_ICLASS_PHSUBW 		: {return "PHSUBW";}
	case XED_ICLASS_PI2FD 		: {return "PI2FD";}
	case XED_ICLASS_PI2FW 		: {return "PI2FW";}
	case XED_ICLASS_PINSRB 		: {return "PINSRB";}
	case XED_ICLASS_PINSRD 		: {return "PINSRD";}
	case XED_ICLASS_PINSRQ 		: {return "PINSRQ";}
	case XED_ICLASS_PINSRW 		: {return "PINSRW";}
	case XED_ICLASS_PMADDUBSW 	: {return "PMADDUBSW";}
	case XED_ICLASS_PMADDWD 	: {return "PMADDWD";}
	case XED_ICLASS_PMAXSB 		: {return "PMAXSB";}
	case XED_ICLASS_PMAXSD 		: {return "PMAXSD";}
	case XED_ICLASS_PMAXSW 		: {return "PMAXSW";}
	case XED_ICLASS_PMAXUB 		: {return "PMAXUB";}
	case XED_ICLASS_PMAXUD 		: {return "PMAXUD";}
	case XED_ICLASS_PMAXUW 		: {return "PMAXUW";}
	case XED_ICLASS_PMINSB 		: {return "PMINSB";}
	case XED_ICLASS_PMINSD 		: {return "PMINSD";}
	case XED_ICLASS_PMINSW 		: {return "PMINSW";}
	case XED_ICLASS_PMINUB 		: {return "PMINUB";}
	case XED_ICLASS_PMINUD 		: {return "PMINUD";}
	case XED_ICLASS_PMINUW 		: {return "PMINUW";}
	case XED_ICLASS_PMOVMSKB 	: {return "PMOVMSKB";}
	case XED_ICLASS_PMOVSXBD 	: {return "PMOVSXBD";}
	case XED_ICLASS_PMOVSXBQ 	: {return "PMOVSXBQ";}
	case XED_ICLASS_PMOVSXBW 	: {return "PMOVSXBW";}
	case XED_ICLASS_PMOVSXDQ 	: {return "PMOVSXDQ";}
	case XED_ICLASS_PMOVSXWD 	: {return "PMOVSXWD";}
	case XED_ICLASS_PMOVSXWQ 	: {return "PMOVSXWQ";}
	case XED_ICLASS_PMOVZXBD 	: {return "PMOVZXBD";}
	case XED_ICLASS_PMOVZXBQ 	: {return "PMOVZXBQ";}
	case XED_ICLASS_PMOVZXBW 	: {return "PMOVZXBW";}
	case XED_ICLASS_PMOVZXDQ 	: {return "PMOVZXDQ";}
	case XED_ICLASS_PMOVZXWD 	: {return "PMOVZXWD";}
	case XED_ICLASS_PMOVZXWQ 	: {return "PMOVZXWQ";}
	case XED_ICLASS_PMULDQ 		: {return "PMULDQ";}
	case XED_ICLASS_PMULHRSW 	: {return "PMULHRSW";}
	case XED_ICLASS_PMULHRW 	: {return "PMULHRW";}
	case XED_ICLASS_PMULHUW 	: {return "PMULHUW";}
	case XED_ICLASS_PMULHW 		: {return "PMULHW";}
	case XED_ICLASS_PMULLD 		: {return "PMULLD";}
	case XED_ICLASS_PMULLW 		: {return "PMULLW";}
	case XED_ICLASS_PMULUDQ 	: {return "PMULUDQ";}
	case XED_ICLASS_POP 		: {return "POP";}
	case XED_ICLASS_POPA 		: {return "POPA";}
	case XED_ICLASS_POPAD 		: {return "POPAD";}
	case XED_ICLASS_POPCNT 		: {return "POPCNT";}
	case XED_ICLASS_POPF 		: {return "POPF";}
	case XED_ICLASS_POPFD 		: {return "POPFD";}
	case XED_ICLASS_POPFQ 		: {return "POPFQ";}
	case XED_ICLASS_POR 		: {return "POR";}
	case XED_ICLASS_PREFETCHNTA : {return "PREFETCHNTA";}
	case XED_ICLASS_PREFETCHT0 	: {return "PREFETCHT0";}
	case XED_ICLASS_PREFETCHT1 	: {return "PREFETCHT1";}
	case XED_ICLASS_PREFETCHT2 	: {return "PREFETCHT2";}
	case XED_ICLASS_PREFETCHW 	: {return "PREFETCHW";}
	case XED_ICLASS_PREFETCH_EXCLUSIVE : {return "PREFETCH_EXCLUSIVE";}
	case XED_ICLASS_PREFETCH_RESERVED : {return "PREFETCH_RESERVED";}
	case XED_ICLASS_PSADBW 		: {return "PSADBW";}
	case XED_ICLASS_PSHUFB 		: {return "PSHUFB";}
	case XED_ICLASS_PSHUFD 		: {return "PSHUFD";}
	case XED_ICLASS_PSHUFHW 	: {return "PSHUFHW";}
	case XED_ICLASS_PSHUFLW 	: {return "PSHUFLW";}
	case XED_ICLASS_PSHUFW 		: {return "PSHUFW";}
	case XED_ICLASS_PSIGNB 		: {return "PSIGNB";}
	case XED_ICLASS_PSIGND 		: {return "PSIGND";}
	case XED_ICLASS_PSIGNW 		: {return "PSIGNW";}
	case XED_ICLASS_PSLLD 		: {return "PSLLD";}
	case XED_ICLASS_PSLLDQ 		: {return "PSLLDQ";}
	case XED_ICLASS_PSLLQ 		: {return "PSLLQ";}
	case XED_ICLASS_PSLLW 		: {return "PSLLW";}
	case XED_ICLASS_PSRAD 		: {return "PSRAD";}
	case XED_ICLASS_PSRAW 		: {return "PSRAW";}
	case XED_ICLASS_PSRLD 		: {return "PSRLD";}
	case XED_ICLASS_PSRLDQ 		: {return "PSRLDQ";}
	case XED_ICLASS_PSRLQ 		: {return "PSRLQ";}
	case XED_ICLASS_PSRLW 		: {return "PSRLW";}
	case XED_ICLASS_PSUBB 		: {return "PSUBB";}
	case XED_ICLASS_PSUBD 		: {return "PSUBD";}
	case XED_ICLASS_PSUBQ 		: {return "PSUBQ";}
	case XED_ICLASS_PSUBSB 		: {return "PSUBSB";}
	case XED_ICLASS_PSUBSW 		: {return "PSUBSW";}
	case XED_ICLASS_PSUBUSB 	: {return "PSUBUSB";}
	case XED_ICLASS_PSUBUSW 	: {return "PSUBUSW";}
	case XED_ICLASS_PSUBW 		: {return "PSUBW";}
	case XED_ICLASS_PSWAPD 		: {return "PSWAPD";}
	case XED_ICLASS_PTEST 		: {return "PTEST";}
	case XED_ICLASS_PUNPCKHBW 	: {return "PUNPCKHBW";}
	case XED_ICLASS_PUNPCKHDQ 	: {return "PUNPCKHDQ";}
	case XED_ICLASS_PUNPCKHQDQ 	: {return "PUNPCKHQDQ";}
	case XED_ICLASS_PUNPCKHWD 	: {return "PUNPCKHWD";}
	case XED_ICLASS_PUNPCKLBW 	: {return "PUNPCKLBW";}
	case XED_ICLASS_PUNPCKLDQ 	: {return "PUNPCKLDQ";}
	case XED_ICLASS_PUNPCKLQDQ 	: {return "PUNPCKLQDQ";}
	case XED_ICLASS_PUNPCKLWD 	: {return "PUNPCKLWD";}
	case XED_ICLASS_PUSH 		: {return "PUSH";}
	case XED_ICLASS_PUSHA 		: {return "PUSHA";}
	case XED_ICLASS_PUSHAD 		: {return "PUSHAD";}
	case XED_ICLASS_PUSHF 		: {return "PUSHF";}
	case XED_ICLASS_PUSHFD 		: {return "PUSHFD";}
	case XED_ICLASS_PUSHFQ 		: {return "PUSHFQ";}
	case XED_ICLASS_PXOR 		: {return "PXOR";}
	case XED_ICLASS_RCL 		: {return "RCL";}
	case XED_ICLASS_RCPPS 		: {return "RCPPS";}
	case XED_ICLASS_RCPSS 		: {return "RCPSS";}
	case XED_ICLASS_RCR 		: {return "RCR";}
	case XED_ICLASS_RDFSBASE 	: {return "RDFSBASE";}
	case XED_ICLASS_RDGSBASE 	: {return "RDGSBASE";}
	case XED_ICLASS_RDMSR 		: {return "RDMSR";}
	case XED_ICLASS_RDPMC 		: {return "RDPMC";}
	case XED_ICLASS_RDRAND 		: {return "RDRAND";}
	case XED_ICLASS_RDSEED 		: {return "RDSEED";}
	case XED_ICLASS_RDTSC 		: {return "RDTSC";}
	case XED_ICLASS_RDTSCP 		: {return "RDTSCP";}
	case XED_ICLASS_RET_FAR 	: {return "RET_FAR";}
	case XED_ICLASS_RET_NEAR 	: {return "RET_NEAR";}
	case XED_ICLASS_ROL 		: {return "ROL";}
	case XED_ICLASS_ROR 		: {return "ROR";}
	case XED_ICLASS_RORX 		: {return "RORX";}
	case XED_ICLASS_ROUNDPD 	: {return "ROUNDPD";}
	case XED_ICLASS_ROUNDPS 	: {return "ROUNDPS";}
	case XED_ICLASS_ROUNDSD 	: {return "ROUNDSD";}
	case XED_ICLASS_ROUNDSS 	: {return "ROUNDSS";}
	case XED_ICLASS_RSM 		: {return "RSM";}
	case XED_ICLASS_RSQRTPS 	: {return "RSQRTPS";}
	case XED_ICLASS_RSQRTSS 	: {return "RSQRTSS";}
	case XED_ICLASS_SAHF 		: {return "SAHF";}
	case XED_ICLASS_SALC 		: {return "SALC";}
	case XED_ICLASS_SAR 		: {return "SAR";}
	case XED_ICLASS_SARX 		: {return "SARX";}
	case XED_ICLASS_SBB 		: {return "SBB";}
	case XED_ICLASS_SCASB 		: {return "SCASB";}
	case XED_ICLASS_SCASD 		: {return "SCASD";}
	case XED_ICLASS_SCASQ 		: {return "SCASQ";}
	case XED_ICLASS_SCASW 		: {return "SCASW";}
	case XED_ICLASS_SETB 		: {return "SETB";}
	case XED_ICLASS_SETBE 		: {return "SETBE";}
	case XED_ICLASS_SETL 		: {return "SETL";}
	case XED_ICLASS_SETLE 		: {return "SETLE";}
	case XED_ICLASS_SETNB 		: {return "SETNB";}
	case XED_ICLASS_SETNBE 		: {return "SETNBE";}
	case XED_ICLASS_SETNL 		: {return "SETNL";}
	case XED_ICLASS_SETNLE 		: {return "SETNLE";}
	case XED_ICLASS_SETNO 		: {return "SETNO";}
	case XED_ICLASS_SETNP 		: {return "SETNP";}
	case XED_ICLASS_SETNS 		: {return "SETNS";}
	case XED_ICLASS_SETNZ 		: {return "SETNZ";}
	case XED_ICLASS_SETO 		: {return "SETO";}
	case XED_ICLASS_SETP 		: {return "SETP";}
	case XED_ICLASS_SETS 		: {return "SETS";}
	case XED_ICLASS_SETZ 		: {return "SETZ";}
	case XED_ICLASS_SFENCE 		: {return "SFENCE";}
	case XED_ICLASS_SGDT 		: {return "SGDT";}
	case XED_ICLASS_SHL 		: {return "SHL";}
	case XED_ICLASS_SHLD 		: {return "SHLD";}
	case XED_ICLASS_SHLX 		: {return "SHLX";}
	case XED_ICLASS_SHR 		: {return "SHR";}
	case XED_ICLASS_SHRD 		: {return "SHRD";}
	case XED_ICLASS_SHRX 		: {return "SHRX";}
	case XED_ICLASS_SHUFPD 		: {return "SHUFPD";}
	case XED_ICLASS_SHUFPS 		: {return "SHUFPS";}
	case XED_ICLASS_SIDT 		: {return "SIDT";}
	case XED_ICLASS_SKINIT 		: {return "SKINIT";}
	case XED_ICLASS_SLDT 		: {return "SLDT";}
	case XED_ICLASS_SLWPCB 		: {return "SLWPCB";}
	case XED_ICLASS_SMSW 		: {return "SMSW";}
	case XED_ICLASS_SQRTPD 		: {return "SQRTPD";}
	case XED_ICLASS_SQRTPS 		: {return "SQRTPS";}
	case XED_ICLASS_SQRTSD 		: {return "SQRTSD";}
	case XED_ICLASS_SQRTSS 		: {return "SQRTSS";}
	case XED_ICLASS_STAC 		: {return "STAC";}
	case XED_ICLASS_STC 		: {return "STC";}
	case XED_ICLASS_STD 		: {return "STD";}
	case XED_ICLASS_STGI 		: {return "STGI";}
	case XED_ICLASS_STI 		: {return "STI";}
	case XED_ICLASS_STMXCSR 	: {return "STMXCSR";}
	case XED_ICLASS_STOSB 		: {return "STOSB";}
	case XED_ICLASS_STOSD 		: {return "STOSD";}
	case XED_ICLASS_STOSQ 		: {return "STOSQ";}
	case XED_ICLASS_STOSW 		: {return "STOSW";}
	case XED_ICLASS_STR 		: {return "STR";}
	case XED_ICLASS_SUB 		: {return "SUB";}
	case XED_ICLASS_SUBPD 		: {return "SUBPD";}
	case XED_ICLASS_SUBPS 		: {return "SUBPS";}
	case XED_ICLASS_SUBSD 		: {return "SUBSD";}
	case XED_ICLASS_SUBSS 		: {return "SUBSS";}
	case XED_ICLASS_SWAPGS 		: {return "SWAPGS";}
	case XED_ICLASS_SYSCALL 	: {return "SYSCALL";}
	case XED_ICLASS_SYSCALL_AMD : {return "SYSCALL_AMD";}
	case XED_ICLASS_SYSENTER 	: {return "SYSENTER";}
	case XED_ICLASS_SYSEXIT 	: {return "SYSEXIT";}
	case XED_ICLASS_SYSRET 		: {return "SYSRET";}
	case XED_ICLASS_SYSRET_AMD 	: {return "SYSRET_AMD";}
	case XED_ICLASS_T1MSKC 		: {return "T1MSKC";}
	case XED_ICLASS_TEST 		: {return "TEST";}
	case XED_ICLASS_TZCNT 		: {return "TZCNT";}
	case XED_ICLASS_TZMSK 		: {return "TZMSK";}
	case XED_ICLASS_UCOMISD 	: {return "UCOMISD";}
	case XED_ICLASS_UCOMISS 	: {return "UCOMISS";}
	case XED_ICLASS_UD2 		: {return "UD2";}
	case XED_ICLASS_UNPCKHPD 	: {return "UNPCKHPD";}
	case XED_ICLASS_UNPCKHPS 	: {return "UNPCKHPS";}
	case XED_ICLASS_UNPCKLPD 	: {return "UNPCKLPD";}
	case XED_ICLASS_UNPCKLPS 	: {return "UNPCKLPS";}
	case XED_ICLASS_VADDPD 		: {return "VADDPD";}
	case XED_ICLASS_VADDPS 		: {return "VADDPS";}
	case XED_ICLASS_VADDSD 		: {return "VADDSD";}
	case XED_ICLASS_VADDSS 		: {return "VADDSS";}
	case XED_ICLASS_VADDSUBPD 	: {return "VADDSUBPD";}
	case XED_ICLASS_VADDSUBPS 	: {return "VADDSUBPS";}
	case XED_ICLASS_VAESDEC 	: {return "VAESDEC";}
	case XED_ICLASS_VAESDECLAST : {return "VAESDECLAST";}
	case XED_ICLASS_VAESENC 	: {return "VAESENC";}
	case XED_ICLASS_VAESENCLAST : {return "VAESENCLAST";}
	case XED_ICLASS_VAESIMC 	: {return "VAESIMC";}
	case XED_ICLASS_VAESKEYGENASSIST : {return "VAESKEYGENASSIST";}
	case XED_ICLASS_VANDNPD 	: {return "VANDNPD";}
	case XED_ICLASS_VANDNPS 	: {return "VANDNPS";}
	case XED_ICLASS_VANDPD 		: {return "VANDPD";}
	case XED_ICLASS_VANDPS 		: {return "VANDPS";}
	case XED_ICLASS_VBLENDPD 	: {return "VBLENDPD";}
	case XED_ICLASS_VBLENDPS 	: {return "VBLENDPS";}
	case XED_ICLASS_VBLENDVPD 	: {return "VBLENDVPD";}
	case XED_ICLASS_VBLENDVPS 	: {return "VBLENDVPS";}
	case XED_ICLASS_VBROADCASTF128 : {return "VBROADCASTF128";}
	case XED_ICLASS_VBROADCASTI128 : {return "VBROADCASTI128";}
	case XED_ICLASS_VBROADCASTSD : {return "VBROADCASTSD";}
	case XED_ICLASS_VBROADCASTSS : {return "VBROADCASTSS";}
	case XED_ICLASS_VCMPPD 		: {return "VCMPPD";}
	case XED_ICLASS_VCMPPS 		: {return "VCMPPS";}
	case XED_ICLASS_VCMPSD 		: {return "VCMPSD";}
	case XED_ICLASS_VCMPSS 		: {return "VCMPSS";}
	case XED_ICLASS_VCOMISD 	: {return "VCOMISD";}
	case XED_ICLASS_VCOMISS 	: {return "VCOMISS";}
	case XED_ICLASS_VCVTDQ2PD 	: {return "VCVTDQ2PD";}
	case XED_ICLASS_VCVTDQ2PS 	: {return "VCVTDQ2PS";}
	case XED_ICLASS_VCVTPD2DQ 	: {return "VCVTPD2DQ";}
	case XED_ICLASS_VCVTPD2PS 	: {return "VCVTPD2PS";}
	case XED_ICLASS_VCVTPH2PS 	: {return "VCVTPH2PS";}
	case XED_ICLASS_VCVTPS2DQ 	: {return "VCVTPS2DQ";}
	case XED_ICLASS_VCVTPS2PD 	: {return "VCVTPS2PD";}
	case XED_ICLASS_VCVTPS2PH 	: {return "VCVTPS2PH";}
	case XED_ICLASS_VCVTSD2SI 	: {return "VCVTSD2SI";}
	case XED_ICLASS_VCVTSD2SS 	: {return "VCVTSD2SS";}
	case XED_ICLASS_VCVTSI2SD 	: {return "VCVTSI2SD";}
	case XED_ICLASS_VCVTSI2SS 	: {return "VCVTSI2SS";}
	case XED_ICLASS_VCVTSS2SD 	: {return "VCVTSS2SD";}
	case XED_ICLASS_VCVTSS2SI 	: {return "VCVTSS2SI";}
	case XED_ICLASS_VCVTTPD2DQ 	: {return "VCVTTPD2DQ";}
	case XED_ICLASS_VCVTTPS2DQ 	: {return "VCVTTPS2DQ";}
	case XED_ICLASS_VCVTTSD2SI 	: {return "VCVTTSD2SI";}
	case XED_ICLASS_VCVTTSS2SI 	: {return "VCVTTSS2SI";}
	case XED_ICLASS_VDIVPD 		: {return "VDIVPD";}
	case XED_ICLASS_VDIVPS 		: {return "VDIVPS";}
	case XED_ICLASS_VDIVSD 		: {return "VDIVSD";}
	case XED_ICLASS_VDIVSS 		: {return "VDIVSS";}
	case XED_ICLASS_VDPPD 		: {return "VDPPD";}
	case XED_ICLASS_VDPPS 		: {return "VDPPS";}
	case XED_ICLASS_VERR 		: {return "VERR";}
	case XED_ICLASS_VERW 		: {return "VERW";}
	case XED_ICLASS_VEXTRACTF128 : {return "VEXTRACTF128";}
	case XED_ICLASS_VEXTRACTI128 : {return "VEXTRACTI128";}
	case XED_ICLASS_VEXTRACTPS 	: {return "VEXTRACTPS";}
	case XED_ICLASS_VFMADD132PD : {return "VFMADD132PD";}
	case XED_ICLASS_VFMADD132PS : {return "VFMADD132PS";}
	case XED_ICLASS_VFMADD132SD : {return "VFMADD132SD";}
	case XED_ICLASS_VFMADD132SS : {return "VFMADD132SS";}
	case XED_ICLASS_VFMADD213PD : {return "VFMADD213PD";}
	case XED_ICLASS_VFMADD213PS : {return "VFMADD213PS";}
	case XED_ICLASS_VFMADD213SD : {return "VFMADD213SD";}
	case XED_ICLASS_VFMADD213SS : {return "VFMADD213SS";}
	case XED_ICLASS_VFMADD231PD : {return "VFMADD231PD";}
	case XED_ICLASS_VFMADD231PS : {return "VFMADD231PS";}
	case XED_ICLASS_VFMADD231SD : {return "VFMADD231SD";}
	case XED_ICLASS_VFMADD231SS : {return "VFMADD231SS";}
	case XED_ICLASS_VFMADDPD 	: {return "VFMADDPD";}
	case XED_ICLASS_VFMADDPS 	: {return "VFMADDPS";}
	case XED_ICLASS_VFMADDSD 	: {return "VFMADDSD";}
	case XED_ICLASS_VFMADDSS 	: {return "VFMADDSS";}
	case XED_ICLASS_VFMADDSUB132PD : {return "VFMADDSUB132PD";}
	case XED_ICLASS_VFMADDSUB132PS : {return "VFMADDSUB132PS";}
	case XED_ICLASS_VFMADDSUB213PD : {return "VFMADDSUB213PD";}
	case XED_ICLASS_VFMADDSUB213PS : {return "VFMADDSUB213PS";}
	case XED_ICLASS_VFMADDSUB231PD : {return "VFMADDSUB231PD";}
	case XED_ICLASS_VFMADDSUB231PS : {return "VFMADDSUB231PS";}
	case XED_ICLASS_VFMADDSUBPD : {return "VFMADDSUBPD";}
	case XED_ICLASS_VFMADDSUBPS : {return "VFMADDSUBPS";}
	case XED_ICLASS_VFMSUB132PD : {return "VFMSUB132PD";}
	case XED_ICLASS_VFMSUB132PS : {return "VFMSUB132PS";}
	case XED_ICLASS_VFMSUB132SD : {return "VFMSUB132SD";}
	case XED_ICLASS_VFMSUB132SS : {return "VFMSUB132SS";}
	case XED_ICLASS_VFMSUB213PD : {return "VFMSUB213PD";}
	case XED_ICLASS_VFMSUB213PS : {return "VFMSUB213PS";}
	case XED_ICLASS_VFMSUB213SD : {return "VFMSUB213SD";}
	case XED_ICLASS_VFMSUB213SS : {return "VFMSUB213SS";}
	case XED_ICLASS_VFMSUB231PD : {return "VFMSUB231PD";}
	case XED_ICLASS_VFMSUB231PS : {return "VFMSUB231PS";}
	case XED_ICLASS_VFMSUB231SD : {return "VFMSUB231SD";}
	case XED_ICLASS_VFMSUB231SS : {return "VFMSUB231SS";}
	case XED_ICLASS_VFMSUBADD132PD : {return "VFMSUBADD132PD";}
	case XED_ICLASS_VFMSUBADD132PS : {return "VFMSUBADD132PS";}
	case XED_ICLASS_VFMSUBADD213PD : {return "VFMSUBADD213PD";}
	case XED_ICLASS_VFMSUBADD213PS : {return "VFMSUBADD213PS";}
	case XED_ICLASS_VFMSUBADD231PD : {return "VFMSUBADD231PD";}
	case XED_ICLASS_VFMSUBADD231PS : {return "VFMSUBADD231PS";}
	case XED_ICLASS_VFMSUBADDPD : {return "VFMSUBADDPD";}
	case XED_ICLASS_VFMSUBADDPS : {return "VFMSUBADDPS";}
	case XED_ICLASS_VFMSUBPD 	: {return "VFMSUBPD";}
	case XED_ICLASS_VFMSUBPS 	: {return "VFMSUBPS";}
	case XED_ICLASS_VFMSUBSD 	: {return "VFMSUBSD";}
	case XED_ICLASS_VFMSUBSS 	: {return "VFMSUBSS";}
	case XED_ICLASS_VFNMADD132PD : {return "VFNMADD132PD";}
	case XED_ICLASS_VFNMADD132PS : {return "VFNMADD132PS";}
	case XED_ICLASS_VFNMADD132SD : {return "VFNMADD132SD";}
	case XED_ICLASS_VFNMADD132SS : {return "VFNMADD132SS";}
	case XED_ICLASS_VFNMADD213PD : {return "VFNMADD213PD";}
	case XED_ICLASS_VFNMADD213PS : {return "VFNMADD213PS";}
	case XED_ICLASS_VFNMADD213SD : {return "VFNMADD213SD";}
	case XED_ICLASS_VFNMADD213SS : {return "VFNMADD213SS";}
	case XED_ICLASS_VFNMADD231PD : {return "VFNMADD231PD";}
	case XED_ICLASS_VFNMADD231PS : {return "VFNMADD231PS";}
	case XED_ICLASS_VFNMADD231SD : {return "VFNMADD231SD";}
	case XED_ICLASS_VFNMADD231SS : {return "VFNMADD231SS";}
	case XED_ICLASS_VFNMADDPD 	: {return "VFNMADDPD";}
	case XED_ICLASS_VFNMADDPS 	: {return "VFNMADDPS";}
	case XED_ICLASS_VFNMADDSD 	: {return "VFNMADDSD";}
	case XED_ICLASS_VFNMADDSS 	: {return "VFNMADDSS";}
	case XED_ICLASS_VFNMSUB132PD : {return "VFNMSUB132PD";}
	case XED_ICLASS_VFNMSUB132PS : {return "VFNMSUB132PS";}
	case XED_ICLASS_VFNMSUB132SD : {return "VFNMSUB132SD";}
	case XED_ICLASS_VFNMSUB132SS : {return "VFNMSUB132SS";}
	case XED_ICLASS_VFNMSUB213PD : {return "VFNMSUB213PD";}
	case XED_ICLASS_VFNMSUB213PS : {return "VFNMSUB213PS";}
	case XED_ICLASS_VFNMSUB213SD : {return "VFNMSUB213SD";}
	case XED_ICLASS_VFNMSUB213SS : {return "VFNMSUB213SS";}
	case XED_ICLASS_VFNMSUB231PD : {return "VFNMSUB231PD";}
	case XED_ICLASS_VFNMSUB231PS : {return "VFNMSUB231PS";}
	case XED_ICLASS_VFNMSUB231SD : {return "VFNMSUB231SD";}
	case XED_ICLASS_VFNMSUB231SS : {return "VFNMSUB231SS";}
	case XED_ICLASS_VFNMSUBPD 	: {return "VFNMSUBPD";}
	case XED_ICLASS_VFNMSUBPS 	: {return "VFNMSUBPS";}
	case XED_ICLASS_VFNMSUBSD 	: {return "VFNMSUBSD";}
	case XED_ICLASS_VFNMSUBSS 	: {return "VFNMSUBSS";}
	case XED_ICLASS_VFRCZPD 	: {return "VFRCZPD";}
	case XED_ICLASS_VFRCZPS 	: {return "VFRCZPS";}
	case XED_ICLASS_VFRCZSD 	: {return "VFRCZSD";}
	case XED_ICLASS_VFRCZSS 	: {return "VFRCZSS";}
	case XED_ICLASS_VGATHERDPD 	: {return "VGATHERDPD";}
	case XED_ICLASS_VGATHERDPS 	: {return "VGATHERDPS";}
	case XED_ICLASS_VGATHERQPD 	: {return "VGATHERQPD";}
	case XED_ICLASS_VGATHERQPS 	: {return "VGATHERQPS";}
	case XED_ICLASS_VHADDPD 	: {return "VHADDPD";}
	case XED_ICLASS_VHADDPS 	: {return "VHADDPS";}
	case XED_ICLASS_VHSUBPD 	: {return "VHSUBPD";}
	case XED_ICLASS_VHSUBPS 	: {return "VHSUBPS";}
	case XED_ICLASS_VINSERTF128 : {return "VINSERTF128";}
	case XED_ICLASS_VINSERTI128 : {return "VINSERTI128";}
	case XED_ICLASS_VINSERTPS 	: {return "VINSERTPS";}
	case XED_ICLASS_VLDDQU 		: {return "VLDDQU";}
	case XED_ICLASS_VLDMXCSR 	: {return "VLDMXCSR";}
	case XED_ICLASS_VMASKMOVDQU : {return "VMASKMOVDQU";}
	case XED_ICLASS_VMASKMOVPD 	: {return "VMASKMOVPD";}
	case XED_ICLASS_VMASKMOVPS 	: {return "VMASKMOVPS";}
	case XED_ICLASS_VMAXPD 		: {return "VMAXPD";}
	case XED_ICLASS_VMAXPS 		: {return "VMAXPS";}
	case XED_ICLASS_VMAXSD 		: {return "VMAXSD";}
	case XED_ICLASS_VMAXSS 		: {return "VMAXSS";}
	case XED_ICLASS_VMCALL 		: {return "VMCALL";}
	case XED_ICLASS_VMCLEAR 	: {return "VMCLEAR";}
	case XED_ICLASS_VMFUNC 		: {return "VMFUNC";}
	case XED_ICLASS_VMINPD 		: {return "VMINPD";}
	case XED_ICLASS_VMINPS 		: {return "VMINPS";}
	case XED_ICLASS_VMINSD 		: {return "VMINSD";}
	case XED_ICLASS_VMINSS 		: {return "VMINSS";}
	case XED_ICLASS_VMLAUNCH 	: {return "VMLAUNCH";}
	case XED_ICLASS_VMLOAD 		: {return "VMLOAD";}
	case XED_ICLASS_VMMCALL 	: {return "VMMCALL";}
	case XED_ICLASS_VMOVAPD 	: {return "VMOVAPD";}
	case XED_ICLASS_VMOVAPS 	: {return "VMOVAPS";}
	case XED_ICLASS_VMOVD 		: {return "VMOVD";}
	case XED_ICLASS_VMOVDDUP 	: {return "VMOVDDUP";}
	case XED_ICLASS_VMOVDQA 	: {return "VMOVDQA";}
	case XED_ICLASS_VMOVDQU 	: {return "VMOVDQU";}
	case XED_ICLASS_VMOVHLPS 	: {return "VMOVHLPS";}
	case XED_ICLASS_VMOVHPD 	: {return "VMOVHPD";}
	case XED_ICLASS_VMOVHPS 	: {return "VMOVHPS";}
	case XED_ICLASS_VMOVLHPS 	: {return "VMOVLHPS";}
	case XED_ICLASS_VMOVLPD 	: {return "VMOVLPD";}
	case XED_ICLASS_VMOVLPS 	: {return "VMOVLPS";}
	case XED_ICLASS_VMOVMSKPD 	: {return "VMOVMSKPD";}
	case XED_ICLASS_VMOVMSKPS 	: {return "VMOVMSKPS";}
	case XED_ICLASS_VMOVNTDQ 	: {return "VMOVNTDQ";}
	case XED_ICLASS_VMOVNTDQA 	: {return "VMOVNTDQA";}
	case XED_ICLASS_VMOVNTPD 	: {return "VMOVNTPD";}
	case XED_ICLASS_VMOVNTPS 	: {return "VMOVNTPS";}
	case XED_ICLASS_VMOVQ 		: {return "VMOVQ";}
	case XED_ICLASS_VMOVSD 		: {return "VMOVSD";}
	case XED_ICLASS_VMOVSHDUP 	: {return "VMOVSHDUP";}
	case XED_ICLASS_VMOVSLDUP 	: {return "VMOVSLDUP";}
	case XED_ICLASS_VMOVSS 		: {return "VMOVSS";}
	case XED_ICLASS_VMOVUPD 	: {return "VMOVUPD";}
	case XED_ICLASS_VMOVUPS 	: {return "VMOVUPS";}
	case XED_ICLASS_VMPSADBW 	: {return "VMPSADBW";}
	case XED_ICLASS_VMPTRLD 	: {return "VMPTRLD";}
	case XED_ICLASS_VMPTRST 	: {return "VMPTRST";}
	case XED_ICLASS_VMREAD 		: {return "VMREAD";}
	case XED_ICLASS_VMRESUME 	: {return "VMRESUME";}
	case XED_ICLASS_VMRUN 		: {return "VMRUN";}
	case XED_ICLASS_VMSAVE 		: {return "VMSAVE";}
	case XED_ICLASS_VMULPD 		: {return "VMULPD";}
	case XED_ICLASS_VMULPS 		: {return "VMULPS";}
	case XED_ICLASS_VMULSD 		: {return "VMULSD";}
	case XED_ICLASS_VMULSS 		: {return "VMULSS";}
	case XED_ICLASS_VMWRITE 	: {return "VMWRITE";}
	case XED_ICLASS_VMXOFF 		: {return "VMXOFF";}
	case XED_ICLASS_VMXON 		: {return "VMXON";}
	case XED_ICLASS_VORPD 		: {return "VORPD";}
	case XED_ICLASS_VORPS 		: {return "VORPS";}
	case XED_ICLASS_VPABSB 		: {return "VPABSB";}
	case XED_ICLASS_VPABSD 		: {return "VPABSD";}
	case XED_ICLASS_VPABSW 		: {return "VPABSW";}
	case XED_ICLASS_VPACKSSDW 	: {return "VPACKSSDW";}
	case XED_ICLASS_VPACKSSWB 	: {return "VPACKSSWB";}
	case XED_ICLASS_VPACKUSDW 	: {return "VPACKUSDW";}
	case XED_ICLASS_VPACKUSWB 	: {return "VPACKUSWB";}
	case XED_ICLASS_VPADDB 		: {return "VPADDB";}
	case XED_ICLASS_VPADDD 		: {return "VPADDD";}
	case XED_ICLASS_VPADDQ 		: {return "VPADDQ";}
	case XED_ICLASS_VPADDSB 	: {return "VPADDSB";}
	case XED_ICLASS_VPADDSW 	: {return "VPADDSW";}
	case XED_ICLASS_VPADDUSB 	: {return "VPADDUSB";}
	case XED_ICLASS_VPADDUSW 	: {return "VPADDUSW";}
	case XED_ICLASS_VPADDW 		: {return "VPADDW";}
	case XED_ICLASS_VPALIGNR 	: {return "VPALIGNR";}
	case XED_ICLASS_VPAND 		: {return "VPAND";}
	case XED_ICLASS_VPANDN 		: {return "VPANDN";}
	case XED_ICLASS_VPAVGB 		: {return "VPAVGB";}
	case XED_ICLASS_VPAVGW 		: {return "VPAVGW";}
	case XED_ICLASS_VPBLENDD 	: {return "VPBLENDD";}
	case XED_ICLASS_VPBLENDVB 	: {return "VPBLENDVB";}
	case XED_ICLASS_VPBLENDW 	: {return "VPBLENDW";}
	case XED_ICLASS_VPBROADCASTB : {return "VPBROADCASTB";}
	case XED_ICLASS_VPBROADCASTD : {return "VPBROADCASTD";}
	case XED_ICLASS_VPBROADCASTQ : {return "VPBROADCASTQ";}
	case XED_ICLASS_VPBROADCASTW : {return "VPBROADCASTW";}
	case XED_ICLASS_VPCLMULQDQ 	: {return "VPCLMULQDQ";}
	case XED_ICLASS_VPCMOV 		: {return "VPCMOV";}
	case XED_ICLASS_VPCMPEQB 	: {return "VPCMPEQB";}
	case XED_ICLASS_VPCMPEQD 	: {return "VPCMPEQD";}
	case XED_ICLASS_VPCMPEQQ 	: {return "VPCMPEQQ";}
	case XED_ICLASS_VPCMPEQW 	: {return "VPCMPEQW";}
	case XED_ICLASS_VPCMPESTRI 	: {return "VPCMPESTRI";}
	case XED_ICLASS_VPCMPESTRM 	: {return "VPCMPESTRM";}
	case XED_ICLASS_VPCMPGTB 	: {return "VPCMPGTB";}
	case XED_ICLASS_VPCMPGTD 	: {return "VPCMPGTD";}
	case XED_ICLASS_VPCMPGTQ 	: {return "VPCMPGTQ";}
	case XED_ICLASS_VPCMPGTW 	: {return "VPCMPGTW";}
	case XED_ICLASS_VPCMPISTRI 	: {return "VPCMPISTRI";}
	case XED_ICLASS_VPCMPISTRM 	: {return "VPCMPISTRM";}
	case XED_ICLASS_VPCOMB 		: {return "VPCOMB";}
	case XED_ICLASS_VPCOMD 		: {return "VPCOMD";}
	case XED_ICLASS_VPCOMQ 		: {return "VPCOMQ";}
	case XED_ICLASS_VPCOMUB 	: {return "VPCOMUB";}
	case XED_ICLASS_VPCOMUD 	: {return "VPCOMUD";}
	case XED_ICLASS_VPCOMUQ 	: {return "VPCOMUQ";}
	case XED_ICLASS_VPCOMUW 	: {return "VPCOMUW";}
	case XED_ICLASS_VPCOMW 		: {return "VPCOMW";}
	case XED_ICLASS_VPERM2F128 	: {return "VPERM2F128";}
	case XED_ICLASS_VPERM2I128 	: {return "VPERM2I128";}
	case XED_ICLASS_VPERMD 		: {return "VPERMD";}
	case XED_ICLASS_VPERMIL2PD 	: {return "VPERMIL2PD";}
	case XED_ICLASS_VPERMIL2PS 	: {return "VPERMIL2PS";}
	case XED_ICLASS_VPERMILPD 	: {return "VPERMILPD";}
	case XED_ICLASS_VPERMILPS 	: {return "VPERMILPS";}
	case XED_ICLASS_VPERMPD 	: {return "VPERMPD";}
	case XED_ICLASS_VPERMPS 	: {return "VPERMPS";}
	case XED_ICLASS_VPERMQ 		: {return "VPERMQ";}
	case XED_ICLASS_VPEXTRB 	: {return "VPEXTRB";}
	case XED_ICLASS_VPEXTRD 	: {return "VPEXTRD";}
	case XED_ICLASS_VPEXTRQ 	: {return "VPEXTRQ";}
	case XED_ICLASS_VPEXTRW 	: {return "VPEXTRW";}
	case XED_ICLASS_VPGATHERDD 	: {return "VPGATHERDD";}
	case XED_ICLASS_VPGATHERDQ 	: {return "VPGATHERDQ";}
	case XED_ICLASS_VPGATHERQD 	: {return "VPGATHERQD";}
	case XED_ICLASS_VPGATHERQQ 	: {return "VPGATHERQQ";}
	case XED_ICLASS_VPHADDBD 	: {return "VPHADDBD";}
	case XED_ICLASS_VPHADDBQ 	: {return "VPHADDBQ";}
	case XED_ICLASS_VPHADDBW 	: {return "VPHADDBW";}
	case XED_ICLASS_VPHADDD 	: {return "VPHADDD";}
	case XED_ICLASS_VPHADDDQ 	: {return "VPHADDDQ";}
	case XED_ICLASS_VPHADDSW 	: {return "VPHADDSW";}
	case XED_ICLASS_VPHADDUBD 	: {return "VPHADDUBD";}
	case XED_ICLASS_VPHADDUBQ 	: {return "VPHADDUBQ";}
	case XED_ICLASS_VPHADDUBW 	: {return "VPHADDUBW";}
	case XED_ICLASS_VPHADDUDQ 	: {return "VPHADDUDQ";}
	case XED_ICLASS_VPHADDUWD 	: {return "VPHADDUWD";}
	case XED_ICLASS_VPHADDUWQ 	: {return "VPHADDUWQ";}
	case XED_ICLASS_VPHADDW 	: {return "VPHADDW";}
	case XED_ICLASS_VPHADDWD 	: {return "VPHADDWD";}
	case XED_ICLASS_VPHADDWQ 	: {return "VPHADDWQ";}
	case XED_ICLASS_VPHMINPOSUW : {return "VPHMINPOSUW";}
	case XED_ICLASS_VPHSUBBW 	: {return "VPHSUBBW";}
	case XED_ICLASS_VPHSUBD 	: {return "VPHSUBD";}
	case XED_ICLASS_VPHSUBDQ 	: {return "VPHSUBDQ";}
	case XED_ICLASS_VPHSUBSW 	: {return "VPHSUBSW";}
	case XED_ICLASS_VPHSUBW 	: {return "VPHSUBW";}
	case XED_ICLASS_VPHSUBWD 	: {return "VPHSUBWD";}
	case XED_ICLASS_VPINSRB 	: {return "VPINSRB";}
	case XED_ICLASS_VPINSRD 	: {return "VPINSRD";}
	case XED_ICLASS_VPINSRQ 	: {return "VPINSRQ";}
	case XED_ICLASS_VPINSRW 	: {return "VPINSRW";}
	case XED_ICLASS_VPMACSDD 	: {return "VPMACSDD";}
	case XED_ICLASS_VPMACSDQH 	: {return "VPMACSDQH";}
	case XED_ICLASS_VPMACSDQL 	: {return "VPMACSDQL";}
	case XED_ICLASS_VPMACSSDD 	: {return "VPMACSSDD";}
	case XED_ICLASS_VPMACSSDQH 	: {return "VPMACSSDQH";}
	case XED_ICLASS_VPMACSSDQL 	: {return "VPMACSSDQL";}
	case XED_ICLASS_VPMACSSWD 	: {return "VPMACSSWD";}
	case XED_ICLASS_VPMACSSWW 	: {return "VPMACSSWW";}
	case XED_ICLASS_VPMACSWD 	: {return "VPMACSWD";}
	case XED_ICLASS_VPMACSWW 	: {return "VPMACSWW";}
	case XED_ICLASS_VPMADCSSWD 	: {return "VPMADCSSWD";}
	case XED_ICLASS_VPMADCSWD 	: {return "VPMADCSWD";}
	case XED_ICLASS_VPMADDUBSW 	: {return "VPMADDUBSW";}
	case XED_ICLASS_VPMADDWD 	: {return "VPMADDWD";}
	case XED_ICLASS_VPMASKMOVD 	: {return "VPMASKMOVD";}
	case XED_ICLASS_VPMASKMOVQ 	: {return "VPMASKMOVQ";}
	case XED_ICLASS_VPMAXSB 	: {return "VPMAXSB";}
	case XED_ICLASS_VPMAXSD 	: {return "VPMAXSD";}
	case XED_ICLASS_VPMAXSW 	: {return "VPMAXSW";}
	case XED_ICLASS_VPMAXUB 	: {return "VPMAXUB";}
	case XED_ICLASS_VPMAXUD 	: {return "VPMAXUD";}
	case XED_ICLASS_VPMAXUW 	: {return "VPMAXUW";}
	case XED_ICLASS_VPMINSB 	: {return "VPMINSB";}
	case XED_ICLASS_VPMINSD 	: {return "VPMINSD";}
	case XED_ICLASS_VPMINSW 	: {return "VPMINSW";}
	case XED_ICLASS_VPMINUB 	: {return "VPMINUB";}
	case XED_ICLASS_VPMINUD 	: {return "VPMINUD";}
	case XED_ICLASS_VPMINUW 	: {return "VPMINUW";}
	case XED_ICLASS_VPMOVMSKB 	: {return "VPMOVMSKB";}
	case XED_ICLASS_VPMOVSXBD 	: {return "VPMOVSXBD";}
	case XED_ICLASS_VPMOVSXBQ 	: {return "VPMOVSXBQ";}
	case XED_ICLASS_VPMOVSXBW 	: {return "VPMOVSXBW";}
	case XED_ICLASS_VPMOVSXDQ 	: {return "VPMOVSXDQ";}
	case XED_ICLASS_VPMOVSXWD 	: {return "VPMOVSXWD";}
	case XED_ICLASS_VPMOVSXWQ 	: {return "VPMOVSXWQ";}
	case XED_ICLASS_VPMOVZXBD 	: {return "VPMOVZXBD";}
	case XED_ICLASS_VPMOVZXBQ 	: {return "VPMOVZXBQ";}
	case XED_ICLASS_VPMOVZXBW 	: {return "VPMOVZXBW";}
	case XED_ICLASS_VPMOVZXDQ 	: {return "VPMOVZXDQ";}
	case XED_ICLASS_VPMOVZXWD 	: {return "VPMOVZXWD";}
	case XED_ICLASS_VPMOVZXWQ 	: {return "VPMOVZXWQ";}
	case XED_ICLASS_VPMULDQ 	: {return "VPMULDQ";}
	case XED_ICLASS_VPMULHRSW 	: {return "VPMULHRSW";}
	case XED_ICLASS_VPMULHUW 	: {return "VPMULHUW";}
	case XED_ICLASS_VPMULHW 	: {return "VPMULHW";}
	case XED_ICLASS_VPMULLD 	: {return "VPMULLD";}
	case XED_ICLASS_VPMULLW 	: {return "VPMULLW";}
	case XED_ICLASS_VPMULUDQ 	: {return "VPMULUDQ";}
	case XED_ICLASS_VPOR 		: {return "VPOR";}
	case XED_ICLASS_VPPERM 		: {return "VPPERM";}
	case XED_ICLASS_VPROTB 		: {return "VPROTB";}
	case XED_ICLASS_VPROTD 		: {return "VPROTD";}
	case XED_ICLASS_VPROTQ 		: {return "VPROTQ";}
	case XED_ICLASS_VPROTW 		: {return "VPROTW";}
	case XED_ICLASS_VPSADBW 	: {return "VPSADBW";}
	case XED_ICLASS_VPSHAB 		: {return "VPSHAB";}
	case XED_ICLASS_VPSHAD 		: {return "VPSHAD";}
	case XED_ICLASS_VPSHAQ 		: {return "VPSHAQ";}
	case XED_ICLASS_VPSHAW 		: {return "VPSHAW";}
	case XED_ICLASS_VPSHLB 		: {return "VPSHLB";}
	case XED_ICLASS_VPSHLD 		: {return "VPSHLD";}
	case XED_ICLASS_VPSHLQ 		: {return "VPSHLQ";}
	case XED_ICLASS_VPSHLW 		: {return "VPSHLW";}
	case XED_ICLASS_VPSHUFB 	: {return "VPSHUFB";}
	case XED_ICLASS_VPSHUFD 	: {return "VPSHUFD";}
	case XED_ICLASS_VPSHUFHW 	: {return "VPSHUFHW";}
	case XED_ICLASS_VPSHUFLW 	: {return "VPSHUFLW";}
	case XED_ICLASS_VPSIGNB 	: {return "VPSIGNB";}
	case XED_ICLASS_VPSIGND 	: {return "VPSIGND";}
	case XED_ICLASS_VPSIGNW 	: {return "VPSIGNW";}
	case XED_ICLASS_VPSLLD 		: {return "VPSLLD";}
	case XED_ICLASS_VPSLLDQ 	: {return "VPSLLDQ";}
	case XED_ICLASS_VPSLLQ 		: {return "VPSLLQ";}
	case XED_ICLASS_VPSLLVD 	: {return "VPSLLVD";}
	case XED_ICLASS_VPSLLVQ 	: {return "VPSLLVQ";}
	case XED_ICLASS_VPSLLW 		: {return "VPSLLW";}
	case XED_ICLASS_VPSRAD 		: {return "VPSRAD";}
	case XED_ICLASS_VPSRAVD 	: {return "VPSRAVD";}
	case XED_ICLASS_VPSRAW 		: {return "VPSRAW";}
	case XED_ICLASS_VPSRLD 		: {return "VPSRLD";}
	case XED_ICLASS_VPSRLDQ 	: {return "VPSRLDQ";}
	case XED_ICLASS_VPSRLQ 		: {return "VPSRLQ";}
	case XED_ICLASS_VPSRLVD 	: {return "VPSRLVD";}
	case XED_ICLASS_VPSRLVQ 	: {return "VPSRLVQ";}
	case XED_ICLASS_VPSRLW 		: {return "VPSRLW";}
	case XED_ICLASS_VPSUBB 		: {return "VPSUBB";}
	case XED_ICLASS_VPSUBD 		: {return "VPSUBD";}
	case XED_ICLASS_VPSUBQ 		: {return "VPSUBQ";}
	case XED_ICLASS_VPSUBSB 	: {return "VPSUBSB";}
	case XED_ICLASS_VPSUBSW 	: {return "VPSUBSW";}
	case XED_ICLASS_VPSUBUSB 	: {return "VPSUBUSB";}
	case XED_ICLASS_VPSUBUSW 	: {return "VPSUBUSW";}
	case XED_ICLASS_VPSUBW 		: {return "VPSUBW";}
	case XED_ICLASS_VPTEST 		: {return "VPTEST";}
	case XED_ICLASS_VPUNPCKHBW 	: {return "VPUNPCKHBW";}
	case XED_ICLASS_VPUNPCKHDQ 	: {return "VPUNPCKHDQ";}
	case XED_ICLASS_VPUNPCKHQDQ : {return "VPUNPCKHQDQ";}
	case XED_ICLASS_VPUNPCKHWD 	: {return "VPUNPCKHWD";}
	case XED_ICLASS_VPUNPCKLBW 	: {return "VPUNPCKLBW";}
	case XED_ICLASS_VPUNPCKLDQ 	: {return "VPUNPCKLDQ";}
	case XED_ICLASS_VPUNPCKLQDQ : {return "VPUNPCKLQDQ";}
	case XED_ICLASS_VPUNPCKLWD 	: {return "VPUNPCKLWD";}
	case XED_ICLASS_VPXOR 		: {return "VPXOR";}
	case XED_ICLASS_VRCPPS 		: {return "VRCPPS";}
	case XED_ICLASS_VRCPSS 		: {return "VRCPSS";}
	case XED_ICLASS_VROUNDPD 	: {return "VROUNDPD";}
	case XED_ICLASS_VROUNDPS 	: {return "VROUNDPS";}
	case XED_ICLASS_VROUNDSD 	: {return "VROUNDSD";}
	case XED_ICLASS_VROUNDSS 	: {return "VROUNDSS";}
	case XED_ICLASS_VRSQRTPS 	: {return "VRSQRTPS";}
	case XED_ICLASS_VRSQRTSS 	: {return "VRSQRTSS";}
	case XED_ICLASS_VSHUFPD 	: {return "VSHUFPD";}
	case XED_ICLASS_VSHUFPS 	: {return "VSHUFPS";}
	case XED_ICLASS_VSQRTPD 	: {return "VSQRTPD";}
	case XED_ICLASS_VSQRTPS 	: {return "VSQRTPS";}
	case XED_ICLASS_VSQRTSD 	: {return "VSQRTSD";}
	case XED_ICLASS_VSQRTSS 	: {return "VSQRTSS";}
	case XED_ICLASS_VSTMXCSR 	: {return "VSTMXCSR";}
	case XED_ICLASS_VSUBPD 		: {return "VSUBPD";}
	case XED_ICLASS_VSUBPS 		: {return "VSUBPS";}
	case XED_ICLASS_VSUBSD 		: {return "VSUBSD";}
	case XED_ICLASS_VSUBSS 		: {return "VSUBSS";}
	case XED_ICLASS_VTESTPD 	: {return "VTESTPD";}
	case XED_ICLASS_VTESTPS 	: {return "VTESTPS";}
	case XED_ICLASS_VUCOMISD 	: {return "VUCOMISD";}
	case XED_ICLASS_VUCOMISS 	: {return "VUCOMISS";}
	case XED_ICLASS_VUNPCKHPD 	: {return "VUNPCKHPD";}
	case XED_ICLASS_VUNPCKHPS 	: {return "VUNPCKHPS";}
	case XED_ICLASS_VUNPCKLPD 	: {return "VUNPCKLPD";}
	case XED_ICLASS_VUNPCKLPS 	: {return "VUNPCKLPS";}
	case XED_ICLASS_VXORPD 		: {return "VXORPD";}
	case XED_ICLASS_VXORPS 		: {return "VXORPS";}
	case XED_ICLASS_VZEROALL 	: {return "VZEROALL";}
	case XED_ICLASS_VZEROUPPER 	: {return "VZEROUPPER";}
	case XED_ICLASS_WBINVD 		: {return "WBINVD";}
	case XED_ICLASS_WRFSBASE 	: {return "WRFSBASE";}
	case XED_ICLASS_WRGSBASE 	: {return "WRGSBASE";}
	case XED_ICLASS_WRMSR 		: {return "WRMSR";}
	case XED_ICLASS_XABORT 		: {return "XABORT";}
	case XED_ICLASS_XADD 		: {return "XADD";}
	case XED_ICLASS_XBEGIN 		: {return "XBEGIN";}
	case XED_ICLASS_XCHG 		: {return "XCHG";}
	case XED_ICLASS_XEND 		: {return "XEND";}
	case XED_ICLASS_XGETBV 		: {return "XGETBV";}
	case XED_ICLASS_XLAT 		: {return "XLAT";}
	case XED_ICLASS_XOR 		: {return "XOR";}
	case XED_ICLASS_XORPD 		: {return "XORPD";}
	case XED_ICLASS_XORPS 		: {return "XORPS";}
	case XED_ICLASS_XRSTOR 		: {return "XRSTOR";}
	case XED_ICLASS_XRSTOR64 	: {return "XRSTOR64";}
	case XED_ICLASS_XSAVE 		: {return "XSAVE";}
	case XED_ICLASS_XSAVE64 	: {return "XSAVE64";}
	case XED_ICLASS_XSAVEOPT 	: {return "XSAVEOPT";}
	case XED_ICLASS_XSAVEOPT64 	: {return "XSAVEOPT64";}
	case XED_ICLASS_XSETBV 		: {return "XSETBV";}
	case XED_ICLASS_XTEST 		: {return "XTEST";}
	case XED_ICLASS_LAST 		: {return "LAST";}
	}

	return NULL;
}

const char* reg_2_string(enum reg reg){
	switch(reg){
	case REGISTER_INVALID 		: {return "INVALID";}
	case REGISTER_EAX 			: {return "EAX";}
	case REGISTER_AX 			: {return "AX";}
	case REGISTER_AH 			: {return "AH";}
	case REGISTER_AL 			: {return "AL";}
	case REGISTER_EBX 			: {return "EBX";}
	case REGISTER_BX 			: {return "BX";}
	case REGISTER_BH 			: {return "BH";}
	case REGISTER_BL 			: {return "BL";}
	case REGISTER_ECX 			: {return "ECX";}
	case REGISTER_CX 			: {return "CX";}
	case REGISTER_CH 			: {return "CH";}
	case REGISTER_CL 			: {return "CL";}
	case REGISTER_EDX 			: {return "EDX";}
	case REGISTER_DX 			: {return "DX";}
	case REGISTER_DH 			: {return "DH";}
	case REGISTER_DL 			: {return "DL";}
	case REGISTER_ESI 			: {return "ESI";}
	case REGISTER_EDI 			: {return "EDI";}
	case REGISTER_EBP 			: {return "EBP";}
	}

	return NULL;
}