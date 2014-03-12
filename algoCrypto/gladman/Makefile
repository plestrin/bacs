MISC_DIRECTORY 				:= /home/pierre/Documents/bacs/misc/
CC							:= gcc
CFLAGS 						:= -W -Wall -pedantic -Wextra -g -std=c99
LDFLAGS						:=

AES_NAME 					:= glad_aes
PROGRAM_VERSIONS 			:= v0 v1 v2 v3 v4 v5
PROGRAM_NAMES 				:= $(foreach version,$(PROGRAM_VERSIONS),build/$(AES_NAME)_$(version))
VARIABLE_OBJECTS 			:= glad_aes.o aeskey.o aestab.o aescrypt.o aes_modes.o
MISC_OBJECTS 				:= multiColumn.o printBuffer.o

v0_DEFINE 					:=
v1_DEFINE					:= -DENC_UNROLL=PARTIAL -DDEC_UNROLL=PARTIAL
v2_DEFINE					:= -DENC_UNROLL=NONE -DDEC_UNROLL=NONE -DNO_ENC_KS_UNROLL -DNO_DEC_KS_UNROLL
v3_DEFINE					:= -DENC_ROUND=ONE_TABLE -DLAST_ENC_ROUND=ONE_TABLE -DDEC_ROUND=ONE_TABLE -DLAST_DEC_ROUND=ONE_TABLE -DKEY_SCHED=ONE_TABLE
v4_DEFINE					:= -DENC_ROUND=NO_TABLES -DLAST_ENC_ROUND=NO_TABLES -DDEC_ROUND=NO_TABLES -DLAST_DEC_ROUND=NO_TABLES -DKEY_SCHED=NO_TABLES
v5_DEFINE					:= -DNO_FIXED_TABLES


define PROGRAM_template
build/$$(AES_NAME)_$(1): $$(foreach obj,$$(VARIABLE_OBJECTS),build/$$(obj)$(1)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj))
	$$(CC) -o $$@ $$^ $$(LDFLAGS)

build/%.o$(1): src/%.c
	$(CC) -o $$@ -c $$< $$(CFLAGS) -I$$(MISC_DIRECTORY) -I../ -I./lib/ $$($(1)_DEFINE)

build/%.o$(1): lib/%.c
	$(CC) -o $$@ -c $$< $$(CFLAGS) $$($(1)_DEFINE)
endef

all: $(PROGRAM_NAMES)

$(foreach version,$(PROGRAM_VERSIONS),$(eval $(call PROGRAM_template,$(version))))

build/%.o: $(MISC_DIRECTORY)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean