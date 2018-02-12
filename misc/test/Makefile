TEST_ARRAY				:= test_array
TEST_MULTICOLUMN 		:= test_multiColumn
TEST_INPUTPARSER 		:= test_inputParser
TEST_MAPFILE 			:= test_mapFile
TEST_GRAPH 				:= test_graph
TEST_UGRAPH 			:= test_ugraph
TEST_SUBGRAPHISO 		:= test_subGraphIso
TEST_GRAPHMINPATH 		:= test_graphMinPath
TEST_DAGPARTIALORDER 	:= test_dagPartialOrder
TEST_SET 				:= test_set
TEST_LIST 				:= test_list
TEST_GRAPHREADER		:= test_graphReader
TEST_VARIABLERANGE 		:= test_variableRange
TEST_READBUFFER 		:= test_readBuffer
TEST_ARRAYMINCOVERAGE 	:= test_arrayMinCoverage

CC						:= gcc
# gcc FLAGS
CFLAGS					:= -W -Wall -pedantic -Wextra -g -O0 -std=gnu99 -DINTERACTIVE -DCOLOR -DVERBOSE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -DEXTRA_CHECK
# clang FLAGS
# CFLAGS 				:= -g -pedantic -Weverything -Wno-newline-eof -Wno-reserved-id-macro -Wno-gnu-zero-variadic-macro-arguments -Wno-switch-enum -DINTERACTIVE -DCOLOR -DVERBOSE -D_GNU_SOURCE
LDFLAGS					:=

all: 										\
	$(TEST_ARRAY) 					 		\
	$(TEST_MULTICOLUMN) 					\
	$(TEST_INPUTPARSER) 					\
	$(TEST_MAPFILE) 						\
	$(TEST_GRAPH) 							\
	$(TEST_UGRAPH) 							\
	$(TEST_SUBGRAPHISO) 					\
	$(TEST_GRAPHMINPATH) 					\
	$(TEST_DAGPARTIALORDER) 				\
	$(TEST_SET) 							\
	$(TEST_LIST) 							\
	$(TEST_GRAPHREADER) 					\
	$(TEST_VARIABLERANGE) 					\
	$(TEST_READBUFFER) 						\
	$(TEST_ARRAYMINCOVERAGE)

# *** Test compile rules ***
$(TEST_ARRAY): array.o test_array.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_MULTICOLUMN): multiColumn.o test_multiColumn.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_INPUTPARSER): test_inputParser.o inputParser.o array.o multiColumn.o termReader.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_MAPFILE): test_mapFile.o mapFile.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_GRAPH): test_graph.o graph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_UGRAPH): test_ugraph.o ugraph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_SUBGRAPHISO): test_subGraphIsomorphism.o subGraphIsomorphism.o graph.o graphPrintDot.o array.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_GRAPHMINPATH): test_graphMinPath.o graphMinPath.o graph.o graphPrintDot.o array.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_DAGPARTIALORDER): test_dagPartialOrder.o dagPartialOrder.o graph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_SET): test_set.o set.o printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_LIST): test_list.o list.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_GRAPHREADER): test_graphReader.o mapFile.o graph.o graphReader.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_VARIABLERANGE): test_variableRange.o variableRange.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_READBUFFER): test_readBuffer.o readBuffer.o printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_ARRAYMINCOVERAGE): test_arrayMinCoverage.o arrayMinCoverage.o array.o
	$(CC) -o $@ $^ $(LDFLAGS)

# *** General compile rules ***
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

test_array.o: ../array.h
test_multiColumn.o: ../multiColumn.h ../base.h
test_inputParser.o: ../inputParser.h ../base.h
test_mapFile.o: ../mapFile.h ../base.h
test_graph.o: ../graph.h ../graphPrintDot.h ../base.h
test_ugraph.o: ../ugraph.h ../graphPrintDot.h ../base.h
test_subGraphIsomorphism.o: ../subGraphIsomorphism.h ../graph.h ../graphPrintDot.h ../array.h ../base.h
test_graphMinPath.o: ../graphMinPath.h ../graph.h  ../graphPrintDot.h ../array.h ../base.h
test_dagPartialOrder.o: ../dagPartialOrder.h ../graph.h  ../graphPrintDot.h ../base.h
test_set.o: ../set.h ../base.h ../printBuffer.h
test_list.o: ../list.h ../base.h
test_graphReader.o: ../graphReader.h ../mapFile.h ../graph.h ../graphPrintDot.h ../base.h
test_variableRange.o: ../variableRange.h
test_readBuffer.o: ../readBuffer.h ../printBuffer.h ../readBuffer.h ../base.h
test_arrayMinCoverage.o: ../arrayMinCoverage.h ../array.h ../base.h
array.o: ../array.h ../base.h
multiColumn.o: ../multiColumn.h
inputParser.o: ../inputParser.h ../array.h ../multiColumn.h ../termReader.h  ../base.h
printBuffer.o: ../printBuffer.h
readBuffer.o: ../readBuffer.h ../base.h
mapFile.o: ../mapFile.h ../base.h
graph.o: ../graph.h ../base.h
ugraph.o: ../ugraph.h ../base.h
graphPrintDot.o: ../graph.h ../ugraph.h ../graphPrintDot.h ../base.h
subGraphIsomorphism.o: ../graph.h ../subGraphIsomorphism.h ../array.h ../base.h
graphMinPath.o: ../graph.h ../graphMinPath.h ../array.h ../base.h
dagPartialOrder.o: ../graph.h ../dagPartialOrder.h ../base.h
set.o: ../set.h ../base.h
list.o: ../list.h ../base.h
graphReader.o: ../graphReader.h ../base.h
variableRange.o: ../variableRange.h ../base.h
arrayMinCoverage.o: ../arrayMinCoverage.h ../array.h ../base.h

clean:
	@ rm -f *.o
	@ rm -f *.dot
	@ rm -f *.png
	@ rm -f $(TEST_ARRAY)
	@ rm -f $(TEST_MULTICOLUMN)
	@ rm -f $(TEST_INPUTPARSER)
	@ rm -f $(TEST_MAPFILE)
	@ rm -f $(TEST_GRAPH)
	@ rm -f $(TEST_UGRAPH)
	@ rm -f $(TEST_SUBGRAPHISO)
	@ rm -f $(TEST_GRAPHMINPATH)
	@ rm -f $(TEST_DAGPARTIALORDER)
	@ rm -f $(TEST_SET)
	@ rm -f $(TEST_LIST)
	@ rm -f $(TEST_GRAPHREADER)
	@ rm -f $(TEST_VARIABLERANGE)
	@ rm -f $(TEST_READBUFFER)
	@ rm -f $(TEST_ARRAYMINCOVERAGE)
