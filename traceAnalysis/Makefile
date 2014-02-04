PROG_NAME		:= analysis
DEBUG 			:= 1
ARCHI			:= ARCH_32
CC				:= gcc
ifeq ($(DEBUG),1)
CFLAGS			:= -W -Wall -pedantic -Wextra -g -DVERBOSE -D$(ARCHI)
else
CFLAGS			:= -W -Wall -pedantic -Wextra -DVERBOSE -D$(ARCHI) -O2
endif
CRYPTO_CFLAGS	:= -W -Wall -pedantic -Wextra -O3
LDFLAGS			:= -lyajl -pthread -lm
SRC_DIR 		:= src/
BUILD_DIR 		:= build/
MISC_DIR 		:= ../misc/
SHARED_DIR 		:= ../shared/
CRYPTO_DIR 		:= ../algoCrypto/
INCLUDE_SRC		:= -I$(MISC_DIR) -I$(SHARED_DIR) -I$(CRYPTO_DIR)
INCLUDE_SHARED	:= -I$(MISC_DIR)


all: $(PROG_NAME)

$(PROG_NAME):							\
	$(BUILD_DIR)codeMap.o				\
	$(BUILD_DIR)instruction.o			\
	$(BUILD_DIR)analysis.o 				\
	$(BUILD_DIR)trace.o 				\
	$(BUILD_DIR)traceReaderJSON.o 		\
	$(BUILD_DIR)cmReaderJSON.o 			\
	$(BUILD_DIR)simpleTraceStat.o		\
	$(BUILD_DIR)traceFragment.o			\
	$(BUILD_DIR)argBuffer.o 			\
	$(BUILD_DIR)argSet.o 				\
	$(BUILD_DIR)argSetGraph.o 			\
	$(BUILD_DIR)primitiveReference.o 	\
	$(BUILD_DIR)ioChecker.o 			\
	$(BUILD_DIR)loop.o 					\
	$(BUILD_DIR)memAccess.o 			\
	$(BUILD_DIR)regAccess.o 			\
	$(BUILD_DIR)multiColumn.o 			\
	$(BUILD_DIR)array.o 				\
	$(BUILD_DIR)graph.o 				\
	$(BUILD_DIR)graphPrintDot.o 		\
	$(BUILD_DIR)printBuffer.o			\
	$(BUILD_DIR)readBuffer.o			\
	$(BUILD_DIR)inputParser.o			\
	$(BUILD_DIR)workPercent.o			\
	$(BUILD_DIR)multiWorkPercent.o		\
	$(BUILD_DIR)workQueue.o				\
	$(BUILD_DIR)MD5.o					\
	$(BUILD_DIR)TEA.o 					\
	$(BUILD_DIR)RC4.o 					\
	$(BUILD_DIR)AES.o 					\

	$(CC) -o $(PROG_NAME) $^ $(LDFLAGS)

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(INCLUDE_SRC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(MISC_DIR)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(SHARED_DIR)%.c
	$(CC) $(INCLUDE_SHARED) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(CRYPTO_DIR)%.c
	$(CC) -DANALYSIS_REFERENCE_IMPLEMENTATION -o $@ -c $< $(CRYPTO_CFLAGS)
	
.PHONY: clean

clean:
	@ rm -f $(BUILD_DIR)*.o
	@ rm -f $(PROG_NAME)
	@ rm -f *.dot
	@ rm -f *.png
