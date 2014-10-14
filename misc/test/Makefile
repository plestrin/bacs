TEST_ARRAY				:= test_array
TEST_WORKPERCENT 		:= test_workPercent
TEST_MULTICOLUMN 		:= test_multiColumn
TEST_INPUTPARSER 		:= test_inputParser
TEST_READBUFFER			:= test_readBuffer
TEST_PERMUTATION 		:= test_permutation
TEST_WORKQUEUE 			:= test_workQueue
TEST_MULTIWORKPERCENT 	:= test_multiWorkPercent
TEST_MAPFILE 			:= test_mapFile
TEST_PAGETABLE 			:= test_pageTable
TEST_GRAPH 				:= test_graph
TEST_SUBGRAPHISO 		:= test_subGraphIso
TEST_DIJKSTRA 			:= test_dijkstra
TEST_DAGPARTIALORDER 	:= test_dagPartialOrder

CC						:= gcc
CFLAGS					:= -W -Wall -pedantic -Wextra -g -DINTERACTIVE -DVERBOSE
LDFLAGS					:=

all: 										\
	$(TEST_ARRAY) $(TEST_WORKPERCENT) 		\
	$(TEST_MULTICOLUMN) 					\
	$(TEST_INPUTPARSER) 					\
	$(TEST_READBUFFER) 						\
	$(TEST_PERMUTATION) 					\
	$(TEST_WORKQUEUE) 						\
	$(TEST_MULTIWORKPERCENT) 				\
	$(TEST_MAPFILE) 						\
	$(TEST_PAGETABLE) 						\
	$(TEST_GRAPH) 							\
	$(TEST_SUBGRAPHISO) 					\
	$(TEST_DIJKSTRA) 						\
	$(TEST_DAGPARTIALORDER)

# *** Test compile rules ***
$(TEST_ARRAY): array.o test_array.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_WORKPERCENT): workPercent.o test_workPercent.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_MULTICOLUMN): multiColumn.o test_multiColumn.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_INPUTPARSER): test_inputParser.o inputParser.o array.o multiColumn.o termReader.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_READBUFFER): test_readBuffer.o readBuffer.o printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_PERMUTATION): test_permutation.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_WORKQUEUE): test_workQueue.o workQueue.o
	$(CC) -o $@ $^ $(LDFLAGS) -pthread

$(TEST_MULTIWORKPERCENT): test_multiWorkPercent.o multiWorkPercent.o workQueue.o workPercent.o multiColumn.o
	$(CC) -o $@ $^ $(LDFLAGS) -pthread

$(TEST_MAPFILE): test_mapFile.o mapFile.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_PAGETABLE): test_pageTable.o pageTable.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_GRAPH): test_graph.o graph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_SUBGRAPHISO): test_subGraphIsomorphism.o subGraphIsomorphism.o graph.o graphPrintDot.o array.o dijkstra.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_DIJKSTRA): test_dijkstra.o dijkstra.o graph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_DAGPARTIALORDER): test_dagPartialOrder.o dagPartialOrder.o graph.o graphPrintDot.o
	$(CC) -o $@ $^ $(LDFLAGS)


# *** General compile rules ***
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

test_array.o: ../array.h
test_workPercent.o: ../workPercent.h
test_multiColumn.o: ../multiColumn.h
test_inputParser.o: ../inputParser.h
test_readBuffer.o: ../readBuffer.h ../printBuffer.h
test_permutation.o: ../permutation.h
test_workQueue.o: ../workQueue.h
test_multiWorkPercent.o: ../multiWorkPercent.h ../workQueue.h
test_mapFile.o: ../mapFile.h
test_pageTable.o: ../pageTable.h
test_graph.o: ../graph.h ../graphPrintDot.h
test_subGraphIsomorphism.o: ../subGraphIsomorphism.h ../graph.h ../graphPrintDot.h ../array.h
test_dijkstra.o: ../dijkstra.h ../graph.h  ../graphPrintDot.h
test_dagPartialOrder.o: ../dagPartialOrder.h ../graph.h  ../graphPrintDot.h
array.o: ../array.h
workPercent.o: ../workPercent.h
multiColumn.o: ../multiColumn.h
inputParser.o: ../inputParser.h ../array.h ../multiColumn.h ../termReader.h
readBuffer.o: ../readBuffer.h
printBuffer.o: ../printBuffer.h
workQueue.o: ../workQueue.h
multiWorkPercent.o: ../multiWorkPercent.h ../workPercent.h
pageTable.o: ../pageTable.h
graph.o: ../graph.h
graphPrintDot.o: ../graph.h ../graphPrintDot.h
subGraphIsomorphism.o: ../graph.h ../subGraphIsomorphism.h ../array.h
dijkstra.o: ../graph.h ../dijkstra.h
dagPartialOrder.o: ../graph.h ../dagPartialOrder.h

clean:
	@ rm -f *.o
	@ rm -f *~
	@ rm -f *.dot
	@ rm -f *.png
	@ rm -f $(TEST_ARRAY)
	@ rm -f $(TEST_WORKPERCENT)
	@ rm -f $(TEST_MULTICOLUMN)
	@ rm -f $(TEST_INPUTPARSER)
	@ rm -f $(TEST_READBUFFER)
	@ rm -f $(TEST_PERMUTATION)
	@ rm -f $(TEST_WORKQUEUE)
	@ rm -f $(TEST_MULTIWORKPERCENT)
	@ rm -f $(TEST_MAPFILE)
	@ rm -f $(TEST_PAGETABLE)
	@ rm -f $(TEST_GRAPH)
	@ rm -f $(TEST_SUBGRAPHISO)
	@ rm -f $(TEST_DIJKSTRA)
	@ rm -f $(TEST_DAGPARTIALORDER)
