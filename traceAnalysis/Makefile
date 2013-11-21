PROG_NAME	:= analysis
CC			:= gcc
CFLAGS		:= -W -Wall -pedantic -Wextra -g
LDFLAGS		:= -lyajl
SRC_DIR 	:= src/
BUILD_DIR 	:= build/
MISC_DIR 	:= ../misc/
INCLUDE		:= -I include/ -I $(MISC_DIR)


all: $(BUILD_DIR)analysis.o 			\
	$(BUILD_DIR)trace.o 				\
	$(BUILD_DIR)codeMap.o				\
	$(BUILD_DIR)traceReaderJSON.o 	\
	$(BUILD_DIR)cmReaderJSON.o 		\
	$(BUILD_DIR)controlFlowGraph.o 	\
	$(BUILD_DIR)basicBlock.o 			\
	$(BUILD_DIR)gephiCFGIO.o 			\
	$(BUILD_DIR)instruction.o			\
	$(BUILD_DIR)simpleTraceStat.o		\
	$(BUILD_DIR)graph.o 				\
	$(BUILD_DIR)graphNode.o			\
	$(BUILD_DIR)codeSegment.o			\
	$(BUILD_DIR)callTree.o 			\
	$(BUILD_DIR)graphPrintDot.o		\
	$(MISC_DIR)multiColumn.o
	$(CC) -o $(PROG_NAME) $^ $(LDFLAGS)

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(INCLUDE) -o $@ -c $< $(CFLAGS)

$(MISC_DIR)%.o: $(MISC_DIR)%.c
	$(CC) -o $@ -c $< $(CFLAGS)
	
.PHONY: clean

clean:
	@ rm -f $(BUILD_DIR)*.o
	@ rm -f $(PROG_NAME)
	@ rm -f *.dot
	@ rm -f *.png
	@ rm -f $(MISC_DIR)*.o