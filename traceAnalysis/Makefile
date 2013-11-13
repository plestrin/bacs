PROG_NAME=analysis
CC=gcc
CFLAGS=-W -Wall -pedantic -Wextra -g
LDFLAGS=-lyajl
SRC_DIRECTORY=src/
BUILD_DIRECTORY=build/


all: $(BUILD_DIRECTORY)analysis.o 			\
	$(BUILD_DIRECTORY)trace.o 				\
	$(BUILD_DIRECTORY)codeMap.o				\
	$(BUILD_DIRECTORY)traceReaderJSON.o 	\
	$(BUILD_DIRECTORY)cmReaderJSON.o 		\
	$(BUILD_DIRECTORY)controlFlowGraph.o 	\
	$(BUILD_DIRECTORY)basicBlock.o 			\
	$(BUILD_DIRECTORY)gephiCFGIO.o 			\
	$(BUILD_DIRECTORY)instruction.o			\
	$(BUILD_DIRECTORY)simpleTraceStat.o		\
	$(BUILD_DIRECTORY)graph.o 				\
	$(BUILD_DIRECTORY)graphNode.o			\
	$(BUILD_DIRECTORY)codeSegment.o			\
	$(BUILD_DIRECTORY)callTree.o 			\
	$(BUILD_DIRECTORY)graphPrintDot.o
	$(CC) -o $(PROG_NAME) $^ $(LDFLAGS)

$(BUILD_DIRECTORY)%.o: $(SRC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)
	

clean:
	@ rm -f $(BUILD_DIRECTORY)*.o
	@ rm -f $(PROG_NAME)
	@ rm -f *.dot
	@ rm -f *.png