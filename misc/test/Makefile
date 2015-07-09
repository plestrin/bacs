TEST_ARRAY				:= test_array
TEST_MULTICOLUMN 		:= test_multiColumn
TEST_INPUTPARSER 		:= test_inputParser
TEST_MAPFILE 			:= test_mapFile
TEST_GRAPH 				:= test_graph
TEST_SUBGRAPHISO 		:= test_subGraphIso
TEST_DIJKSTRA 			:= test_dijkstra
TEST_DAGPARTIALORDER 	:= test_dagPartialOrder
TEST_SET 				:= test_set
TEST_UGRAPH 			:= test_ugraph
TEST_GRAPHREADER		:= test_graphReader

CC						:= gcc
# gcc FLAGS
CFLAGS					:= -W -Wall -pedantic -Wextra -g -std=gnu99 -DINTERACTIVE -DCOLOR -DVERBOSE -D_GNU_SOURCE
# clang FLAGS
# CFLAGS 				:= -pedantic -Weverything -Wno-newline-eof -Wno-reserved-id-macro -Wno-gnu-zero-variadic-macro-arguments -Wno-switch-enum
LDFLAGS					:=

all: 										\
	$(TEST_ARRAY) 					 		\
	$(TEST_MULTICOLUMN) 					\
	$(TEST_INPUTPARSER) 					\
	$(TEST_MAPFILE) 						\
	$(TEST_GRAPH) 							\
	$(TEST_SUBGRAPHISO) 					\
	$(TEST_DIJKSTRA) 						\
	$(TEST_DAGPARTIALORDER) 				\
	$(TEST_SET) 							\
	$(TEST_UGRAPH) 							\
	$(TEST_GRAPHREADER)

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

$(TEST_SUBGRAPHISO): test_subGraphIsomorphism.o subGraphIsomorphism.o graph.o graphPrintDot.o array.o dijkstra.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_DIJKSTRA): test_dijkstra.o dijkstra.o graph.o graphPrintDot.o array.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_DAGPARTIALORDER): test_dagPartialOrder.o dagPartialOrder.o graph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_SET): test_set.o set.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_UGRAPH): test_ugraph.o ugraph.o set.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_GRAPHREADER): test_graphReader.o mapFile.o graph.o graphReader.o graphPrintDot.o
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
test_subGraphIsomorphism.o: ../subGraphIsomorphism.h ../graph.h ../graphPrintDot.h ../array.h ../base.h
test_dijkstra.o: ../dijkstra.h ../graph.h  ../graphPrintDot.h ../array.h ../base.h
test_dagPartialOrder.o: ../dagPartialOrder.h ../graph.h  ../graphPrintDot.h ../base.h
test_set.o: ../set.h ../base.h
test_ugraph.o: ../ugraph.h ../set.h ../base.h
test_graphReader.o: ../graphReader.h ../mapFile.h ../graph.h ../graphPrintDot.h ../base.h
array.o: ../array.h ../base.h
multiColumn.o: ../multiColumn.h
inputParser.o: ../inputParser.h ../array.h ../multiColumn.h ../termReader.h  ../base.h
printBuffer.o: ../printBuffer.h
mapFile.o: ../mapFile.h ../base.h
graph.o: ../graph.h ../base.h
graphPrintDot.o: ../graph.h ../graphPrintDot.h ../base.h
subGraphIsomorphism.o: ../graph.h ../subGraphIsomorphism.h ../array.h ../dijkstra.h ../base.h
dijkstra.o: ../graph.h ../dijkstra.h ../array.h ../base.h
dagPartialOrder.o: ../graph.h ../dagPartialOrder.h ../base.h
set.o: ../set.h ../base.h
ugraph.o: ../ugraph.h ../set.h ../base.h
graphReader.o: ../graphReader.h

clean:
	@ rm -f *.o
	@ rm -f *.dot
	@ rm -f *.png
	@ rm -f $(TEST_ARRAY)
	@ rm -f $(TEST_MULTICOLUMN)
	@ rm -f $(TEST_INPUTPARSER)
	@ rm -f $(TEST_MAPFILE)
	@ rm -f $(TEST_GRAPH)
	@ rm -f $(TEST_SUBGRAPHISO)
	@ rm -f $(TEST_DIJKSTRA)
	@ rm -f $(TEST_DAGPARTIALORDER)
	@ rm -f $(TEST_SET)
	@ rm -f $(TEST_UGRAPH)
	@ rm -f $(TEST_GRAPHREADER)