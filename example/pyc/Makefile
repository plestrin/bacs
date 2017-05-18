include ../../path.mk

PROG 	:= python_cbc_aes.py
WHL 	:= whiteList.lst
TOOL 	:= ../../lightTracer_pin/obj-ia32/lightTracer.so
ANAL 	:= ../../test/analysis 

all: search

trace: $(WHL) $(PROG)
	@ rm -rf trace
	$(PIN_BIN) -t $(TOOL) -w $(WHL) -m -o trace -- python $(PROG)

$(WHL):
	echo $$(readlink -f $(PROG)) > $(WHL)

search: $(ANAL) trace
	@ $(ANAL) "load code signature ../../reference/codeSignature_aes.txt" 		\
			"load code signature ../../reference/codeSignature_aes128.txt" 		\
			"load code signature codeSignature_aes128_bis.txt" 					\
			"load trace trace" 													\
			"export trace [17949545:17950422]" 									\
			"export trace [17950493:17951370]" 									\
			"export trace [17951441:17952318]" 									\
			"export trace [17952389:17953266]" 									\
			"create ir" 														\
			"normalize ir" 														\
			"search code signature" 											\
			"export result aes128_v3" 											\
			"export trace [17949500:17953300]" 									\
			"create compound ir [-1:]" 											\
			"simplify concrete ir [-1:]" 										\
			"create synthesis [-1:]" 											\
			"printDot synthesis 4" 												\
			"exit"

clean:
	@ rm -rf $(WHL)
	@ rm -rf trace
	@ rm -f *.log
	@ rm -f *.dot
