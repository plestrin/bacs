PROG_NAME		:= analysis
CC				:= gcc
CFLAGS			:= -W -Wall -pedantic -Wextra -g
LDFLAGS			:= -lyajl
SRC_DIR 		:= src/
BUILD_DIR 		:= build/
MISC_DIR 		:= ../misc/
SHARED_DIR 		:= ../shared/
CRYPTO_DIR 		:= ../algoCrypto/
INCLUDE_SRC		:= -Iinclude/ -I$(MISC_DIR) -I$(SHARED_DIR) -I$(CRYPTO_DIR)
INCLUDE_SHARED	:= -I$(MISC_DIR)


all: $(BUILD_DIR)codeMap.o				\
	$(BUILD_DIR)analysis.o 				\
	$(BUILD_DIR)trace.o 				\
	$(BUILD_DIR)traceReaderJSON.o 		\
	$(BUILD_DIR)cmReaderJSON.o 			\
	$(BUILD_DIR)controlFlowGraph.o 		\
	$(BUILD_DIR)basicBlock.o 			\
	$(BUILD_DIR)gephiCFGIO.o 			\
	$(BUILD_DIR)instruction.o			\
	$(BUILD_DIR)simpleTraceStat.o		\
	$(BUILD_DIR)graph.o 				\
	$(BUILD_DIR)graphNode.o				\
	$(BUILD_DIR)traceFragment.o			\
	$(BUILD_DIR)argBuffer.o 			\
	$(BUILD_DIR)callTree.o 				\
	$(BUILD_DIR)graphPrintDot.o			\
	$(BUILD_DIR)primitiveReference.o 	\
	$(BUILD_DIR)ioChecker.o 			\
	$(BUILD_DIR)multiColumn.o 			\
	$(BUILD_DIR)array.o 				\
	$(BUILD_DIR)printBuffer.o			\
	$(BUILD_DIR)MD5.o					\
	$(BUILD_DIR)TEA.o 					\
	$(BUILD_DIR)RC4.o 					\

	$(CC) -o $(PROG_NAME) $^ $(LDFLAGS)

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(INCLUDE_SRC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(MISC_DIR)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(SHARED_DIR)%.c
	$(CC) $(INCLUDE_SHARED) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(CRYPTO_DIR)%.c
	$(CC) -DANALYSIS_REFERENCE_IMPLEMENTATION -o $@ -c $< $(CFLAGS)
	
.PHONY: clean

clean:
	@ rm -f $(BUILD_DIR)*.o
	@ rm -f $(PROG_NAME)
	@ rm -f *.dot
	@ rm -f *.png
