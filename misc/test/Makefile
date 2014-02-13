TEST_ARRAY				:= test_array
TEST_WORKPERCENT 		:= test_workPercent
TEST_MULTICOLUMN 		:= test_multiColumn
TEST_INPUTPARSER 		:= test_inputParser
TEST_GRAPH 				:= test_graph
TEST_READBUFFER			:= test_readBuffer
TEST_PERMUTATION 		:= test_permutation
TEST_WORKQUEUE 			:= test_workQueue
TEST_MULTIWORKPERCENT 	:= test_multiWorkPercent

CC					:= gcc
CFLAGS				:= -W -Wall -pedantic -Wextra -g
LDFLAGS				:=


all: test_array test_workPercent test_multiColumn test_inputParser test_graph test_readBuffer test_permutation test_workQueue test_multiWorkPercent

# *** Test compile rules ***

test_array: array.o test_array.o
	$(CC) -o $(TEST_ARRAY) $^ $(LDFLAGS)

test_workPercent: workPercent.o test_workPercent.o
	$(CC) -o $(TEST_WORKPERCENT) $^ $(LDFLAGS)

test_multiColumn: multiColumn.o test_multiColumn.o
	$(CC) -o $(TEST_MULTICOLUMN) $^ $(LDFLAGS)

test_inputParser: inputParser.o array.o multiColumn.o test_inputParser.o
	$(CC) -o $(TEST_INPUTPARSER) $^ $(LDFLAGS)

test_graph: test_graph.o graph.o array.o graphPrintDot.o bsearch.o
	$(CC) -o $(TEST_GRAPH) $^ $(LDFLAGS)

test_readBuffer: test_readBuffer.o readBuffer.o printBuffer.o
	$(CC) -o $(TEST_READBUFFER) $^ $(LDFLAGS)

test_permutation: test_permutation.o
	$(CC) -o $(TEST_PERMUTATION) $^ $(LDFLAGS)

test_workQueue: test_workQueue.o workQueue.o
	$(CC) -o $(TEST_WORKQUEUE) $^ $(LDFLAGS) -pthread

test_multiWorkPercent: test_multiWorkPercent.o multiWorkPercent.o workQueue.o workPercent.o multiColumn.o
	$(CC) -o $(TEST_MULTIWORKPERCENT) $^ $(LDFLAGS) -pthread


# *** General compile rules ***
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

test_array.o: ../array.h
test_workPercent.o: ../workPercent.h
test_multiColumn.o: ../multiColumn.h
test_inputParser.o: ../inputParser.h
test_graph.o: ../graph.h ../graphPrintDot.h
test_readBuffer.o: ../readBuffer.h ../printBuffer.h
test_permutation.o: ../permutation.h
test_workQueue.o: ../workQueue.h
test_multiWorkPercent.o: ../multiWorkPercent.h ../workQueue.h
array.o: ../array.h
workPercent.o: ../workPercent.h
multiColumn.o: ../multiColumn.h
inputParser.o: ../inputParser.h ../array.h ../multiColumn.h
graph.o: ../graph.h ../bsearch.h ../array.h
graphPrintDot.o: ../graph.h ../graphPrintDot.h
readBuffer.o: ../readBuffer.h
printBuffer.o: ../printBuffer.h
workQueue.o: ../workQueue.h
multiWorkPercent.o: ../multiWorkPercent.h ../workPercent.h

clean:
	@ rm -f *.o
	@ rm -f *~
	@ rm -f *.dot
	@ rm -f *.png
	@ rm -f $(TEST_ARRAY)
	@ rm -f $(TEST_WORKPERCENT)
	@ rm -f $(TEST_MULTICOLUMN)
	@ rm -f $(TEST_INPUTPARSER)
	@ rm -f $(TEST_GRAPH)
	@ rm -f $(TEST_READBUFFER)
	@ rm -f $(TEST_PERMUTATION)
	@ rm -f $(TEST_WORKQUEUE)
	@ rm -f $(TEST_MULTIWORKPERCENT)
