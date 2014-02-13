LIB_MD5_NAME	:= ref_md5.so
LIB_TEA_NAME 	:= ref_tea.so
LIB_AES_NAME 	:= ref_aes.so
LIB_RC4_NAME 	:= ref_rc4.so
CC				:= gcc
CFLAGS			:= -W -Wall -pedantic -Wextra -O3
SRC_DIR 		:= src/
BUILD_DIR 		:= build/
LIB_DIR 		:= lib/
CRYPTO_DIR 		:= ../algoCrypto/
INCLUDE_SRC		:= -I$(CRYPTO_DIR)


all: $(LIB_DIR)$(LIB_MD5_NAME) $(LIB_DIR)$(LIB_TEA_NAME) $(LIB_DIR)$(LIB_AES_NAME) $(LIB_DIR)$(LIB_RC4_NAME)

$(LIB_DIR)$(LIB_MD5_NAME): $(BUILD_DIR)MD5.o $(BUILD_DIR)ref_md5.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(LIB_DIR)$(LIB_TEA_NAME): $(BUILD_DIR)TEA.o $(BUILD_DIR)ref_tea.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(LIB_DIR)$(LIB_AES_NAME): $(BUILD_DIR)AES.o $(BUILD_DIR)ref_aes.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(LIB_DIR)$(LIB_RC4_NAME): $(BUILD_DIR)RC4.o $(BUILD_DIR)ref_rc4.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# *** General compile rules ***
$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(INCLUDE_SRC) -o $@ -c $< $(CFLAGS)

$(BUILD_DIR)%.o: $(CRYPTO_DIR)%.c
	$(CC) -DANALYSIS_REFERENCE_IMPLEMENTATION -o $@ -c $< $(CFLAGS)
	
.PHONY: clean

clean:
	@ rm -f $(BUILD_DIR)*.o
	@ rm -f $(LIB_DIR)$(LIB_MD5_NAME)
	@ rm -f $(LIB_DIR)$(LIB_TEA_NAME)
	@ rm -f $(LIB_DIR)$(LIB_AES_NAME)
	@ rm -f $(LIB_DIR)$(LIB_RC4_NAME)
