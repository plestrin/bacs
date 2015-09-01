PROG_NAME		:= signature
ifeq ($(DEBUG),)
DEBUG 			:= 1
endif
ifeq ($(VERBOSE),)
VERBOSE 		:= 1
endif
ARCHI			:= ARCH_32
CC				:= gcc
SRC_DIR 		:= src/
ifeq ($(BUILD_DIR),)
BUILD_DIR 		:= build/
else
PROG_NAME 		:= $(BUILD_DIR)../$(PROG_NAME)
endif
MISC_DIR 		:= ../misc/
SHARED_DIR 		:= ../shared/
XED_DIR 		:= /home/pierre/Documents/tool/pin-2.14-71313-gcc.4.4.7-linux/extras/xed-ia32/
INCLUDE_SRC		:= -I$(MISC_DIR) -I$(SHARED_DIR) -I$(XED_DIR)include/
INCLUDE_SHARED	:= -I$(MISC_DIR) -I$(XED_DIR)include/

LDFLAGS			:= -lyajl -lelf $(XED_DIR)lib/libxed.a
CFLAGS			:= -W -Wall -pedantic -Wextra -D$(ARCHI) -std=gnu99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
OBJ 			:= 	$(BUILD_DIR)codeMap.o				\
					$(BUILD_DIR)assembly.o 				\
					$(BUILD_DIR)memTrace.o 				\
					$(BUILD_DIR)analysis.o 				\
					$(BUILD_DIR)trace.o 				\
					$(BUILD_DIR)cmReaderJSON.o 			\
					$(BUILD_DIR)ir.o 					\
					$(BUILD_DIR)irImporterAsm.o 		\
					$(BUILD_DIR)irRenameEngine.o 		\
					$(BUILD_DIR)irNormalize.o 			\
					$(BUILD_DIR)irMemory.o 				\
					$(BUILD_DIR)irCheck.o 				\
					$(BUILD_DIR)irVariableSize.o 		\
					$(BUILD_DIR)irVariableRange.o 		\
					$(BUILD_DIR)irExpression.o 			\
					$(BUILD_DIR)synthesisGraph.o 		\
					$(BUILD_DIR)signatureCollection.o 	\
					$(BUILD_DIR)codeSignature.o 		\
					$(BUILD_DIR)codeSignatureReader.o 	\
					$(BUILD_DIR)modeSignature.o 		\
					$(BUILD_DIR)modeSignatureReader.o 	\
					$(BUILD_DIR)result.o 				\
					$(BUILD_DIR)assemblyElfLoader.o 	\
					$(BUILD_DIR)callGraph.o 			\
					$(BUILD_DIR)multiColumn.o 			\
					$(BUILD_DIR)array.o 				\
					$(BUILD_DIR)printBuffer.o			\
					$(BUILD_DIR)readBuffer.o			\
					$(BUILD_DIR)inputParser.o			\
					$(BUILD_DIR)mapFile.o				\
					$(BUILD_DIR)graph.o 				\
					$(BUILD_DIR)graphPrintDot.o 		\
					$(BUILD_DIR)subGraphIsomorphism.o 	\
					$(BUILD_DIR)graphReader.o 			\
					$(BUILD_DIR)dagPartialOrder.o 		\
					$(BUILD_DIR)dijkstra.o 				\
					$(BUILD_DIR)set.o 					\
					$(BUILD_DIR)variableRange.o

ifeq ($(DEBUG),1)
CFLAGS			:= $(CFLAGS) -g3 -DIR_FULL_CHECK
else
CFLAGS			:= $(CFLAGS) -O2
endif

ifeq ($(VERBOSE),1)
CFLAGS 			:= $(CFLAGS) -DVERBOSE -DINTERACTIVE -DCOLOR
OBJ 			:= $(OBJ) $(BUILD_DIR)termReader.o
endif

all: $(PROG_NAME)

$(PROG_NAME): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(INCLUDE_SRC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(MISC_DIR)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(SHARED_DIR)%.c
	$(CC) $(INCLUDE_SHARED) -o $@ -c $< $(CFLAGS)
	
.PHONY: clean

clean:
	@ rm -f $(BUILD_DIR)*.o
	@ rm -f $(PROG_NAME)
	@ rm -f *.dot
	@ rm -f *.png
