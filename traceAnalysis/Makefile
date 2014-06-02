PROG_NAME		:= analysis
DEBUG 			:= 1
VERBOSE 		:= 1
ARCHI			:= ARCH_32
CC				:= gcc
SRC_DIR 		:= src/
BUILD_DIR 		:= build/
MISC_DIR 		:= ../misc/
SHARED_DIR 		:= ../shared/
XED_DIR 		:= /home/pierre/Documents/pin-2.13-62732-gcc.4.4.7-linux/extras/xed2-ia32/
INCLUDE_SRC		:= -I$(MISC_DIR) -I$(SHARED_DIR) -I$(XED_DIR)include/
INCLUDE_SHARED	:= -I$(MISC_DIR) -I$(XED_DIR)include/

LDFLAGS			:= -lyajl -pthread -ldl $(XED_DIR)lib/libxed.a
CFLAGS			:= -W -Wall -pedantic -Wextra -D$(ARCHI)
OBJ 			:= 	$(BUILD_DIR)codeMap.o				\
					$(BUILD_DIR)instruction.o			\
					$(BUILD_DIR)assembly.o 				\
					$(BUILD_DIR)analysis.o 				\
					$(BUILD_DIR)trace.o 				\
					$(BUILD_DIR)cmReaderJSON.o 			\
					$(BUILD_DIR)refReaderJSON.o 		\
					$(BUILD_DIR)cstReaderJSON.o 		\
					$(BUILD_DIR)simpleTraceStat.o		\
					$(BUILD_DIR)traceFragment.o			\
					$(BUILD_DIR)argument.o 				\
					$(BUILD_DIR)argSet.o 				\
					$(BUILD_DIR)primitiveReference.o 	\
					$(BUILD_DIR)ioChecker.o 			\
					$(BUILD_DIR)cstChecker.o 			\
					$(BUILD_DIR)loop.o 					\
					$(BUILD_DIR)memAccess.o 			\
					$(BUILD_DIR)regAccess.o 			\
					$(BUILD_DIR)ir.o 					\
					$(BUILD_DIR)irImporterDynTrace.o 	\
					$(BUILD_DIR)irImporterAsm.o 		\
					$(BUILD_DIR)irRenameEngine.o 		\
					$(BUILD_DIR)unrolledLoop.o 			\
					$(BUILD_DIR)multiColumn.o 			\
					$(BUILD_DIR)array.o 				\
					$(BUILD_DIR)pageTable.o 			\
					$(BUILD_DIR)printBuffer.o			\
					$(BUILD_DIR)readBuffer.o			\
					$(BUILD_DIR)termReader.o			\
					$(BUILD_DIR)inputParser.o			\
					$(BUILD_DIR)workQueue.o 			\
					$(BUILD_DIR)mapFile.o 				\
					$(BUILD_DIR)graph.o 				\
					$(BUILD_DIR)graphPrintDot.o


ifeq ($(DEBUG),1)
CFLAGS			:= $(CFLAGS) -g
else
CFLAGS			:= $(CFLAGS) -O2
endif

ifeq ($(VERBOSE),1)
LDFLAGS 		:= $(LDFLAGS) -lm
CFLAGS 			:= $(CFLAGS) -DVERBOSE
OBJ 			:= $(OBJ) $(BUILD_DIR)workPercent.o $(BUILD_DIR)multiWorkPercent.o
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
