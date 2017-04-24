MISC_DIRECTORY 				:= ../../misc/
GCC_CC						:= gcc
GCC_CFLAGS 					:= -W -Wall -pedantic -Wextra -std=c99
GCC_CFLAGS_LIB 				:= -w -std=c99
CLANG_CC 					:= clang
CLANG_CFLAGS 				:= -pedantic -Weverything -Wno-cast-align -Wno-newline-eof -Wno-unused-macros
CLANG_CFLAGS_LIB 			:=

AES_NAME 					:= glad_aes
CBC_AES_NAME 				:= glad_cbc_aes
CTR_AES_NAME 				:= glad_ctr_aes
PROGRAM_VERSIONS 			:= v0 v1 v2 v3 v4 v5
GCC_OPTIMIZATION_LEVEL 		:= 0 1 2 3 s
CLANG_OPTIMIZATION_LEVEL 	:= 0 1 2 3 z
PROGRAM_NAMES 				:= $(foreach optim, $(GCC_OPTIMIZATION_LEVEL),$(foreach version,$(PROGRAM_VERSIONS),build/$(AES_NAME)_$(version)_go$(optim) build/$(CBC_AES_NAME)_$(version)_go$(optim) build/$(CTR_AES_NAME)_$(version)_go$(optim))) $(foreach optim, $(CLANG_OPTIMIZATION_LEVEL),$(foreach version,$(PROGRAM_VERSIONS),build/$(AES_NAME)_$(version)_co$(optim) build/$(CBC_AES_NAME)_$(version)_co$(optim) build/$(CTR_AES_NAME)_$(version)_co$(optim)))
AES_VARIABLE_OBJECTS 		:= glad_aes aeskey aestab aescrypt aes_modes
CBC_AES_VARIABLE_OBJECTS 	:= glad_cbc_aes aeskey aestab aescrypt aes_modes
CTR_AES_VARIABLE_OBJECTS 	:= glad_ctr_aes aeskey aestab aescrypt aes_modes
MISC_OBJECTS 				:= printBuffer

v0_DEFINE 					:=
v1_DEFINE					:= -DENC_UNROLL=PARTIAL -DDEC_UNROLL=PARTIAL
v2_DEFINE					:= -DENC_UNROLL=NONE -DDEC_UNROLL=NONE -DNO_ENC_KS_UNROLL -DNO_DEC_KS_UNROLL
v3_DEFINE					:= -DENC_ROUND=ONE_TABLE -DLAST_ENC_ROUND=ONE_TABLE -DDEC_ROUND=ONE_TABLE -DLAST_DEC_ROUND=ONE_TABLE -DKEY_SCHED=ONE_TABLE
v4_DEFINE					:= -DENC_ROUND=NO_TABLES -DLAST_ENC_ROUND=NO_TABLES -DDEC_ROUND=NO_TABLES -DLAST_DEC_ROUND=NO_TABLES -DKEY_SCHED=NO_TABLES
v5_DEFINE					:= -DNO_FIXED_TABLES


define PROGRAM_template
ifeq ($(2),g)
build/$$(AES_NAME)_$(1)_$(2)o$(3): $$(foreach obj,$$(AES_VARIABLE_OBJECTS),build/$$(obj).$(1)$(2)o$(3)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj).$(2)o$(3))
	$$(GCC_CC) -o $$@ $$^

build/$$(CBC_AES_NAME)_$(1)_$(2)o$(3): $$(foreach obj,$$(CBC_AES_VARIABLE_OBJECTS),build/$$(obj).$(1)$(2)o$(3)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj).$(2)o$(3))
	$$(GCC_CC) -o $$@ $$^

build/$$(CTR_AES_NAME)_$(1)_$(2)o$(3): $$(foreach obj,$$(CTR_AES_VARIABLE_OBJECTS),build/$$(obj).$(1)$(2)o$(3)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj).$(2)o$(3))
	$$(GCC_CC) -o $$@ $$^

build/%.$(1)$(2)o$(3): src/%.c
	$(GCC_CC) -o $$@ -c $$< $$(GCC_CFLAGS) -O$(3) -I$$(MISC_DIRECTORY) -I./lib/ $$($(1)_DEFINE)

build/%.$(1)$(2)o$(3): lib/%.c
	$(GCC_CC) -o $$@ -c $$< $$(GCC_CFLAGS_LIB) -O$(3) $$($(1)_DEFINE)
else
build/$$(AES_NAME)_$(1)_$(2)o$(3): $$(foreach obj,$$(AES_VARIABLE_OBJECTS),build/$$(obj).$(1)$(2)o$(3)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj).$(2)o$(3))
	$$(CLANG_CC) -o $$@ $$^

build/$$(CBC_AES_NAME)_$(1)_$(2)o$(3): $$(foreach obj,$$(CBC_AES_VARIABLE_OBJECTS),build/$$(obj).$(1)$(2)o$(3)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj).$(2)o$(3))
	$$(CLANG_CC) -o $$@ $$^

build/$$(CTR_AES_NAME)_$(1)_$(2)o$(3): $$(foreach obj,$$(CTR_AES_VARIABLE_OBJECTS),build/$$(obj).$(1)$(2)o$(3)) $$(foreach obj,$$(MISC_OBJECTS),build/$$(obj).$(2)o$(3))
	$$(CLANG_CC) -o $$@ $$^

build/%.$(1)$(2)o$(3): src/%.c
	$(CLANG_CC) -o $$@ -c $$< $$(CLANG_CFLAGS) -O$(3) -I$$(MISC_DIRECTORY) -I./lib/ $$($(1)_DEFINE)

build/%.$(1)$(2)o$(3): lib/%.c
	$(CLANG_CC) -o $$@ -c $$< $$(CLANG_CFLAGS_LIB) -O$(3) $$($(1)_DEFINE)
endif
endef

all: build $(PROGRAM_NAMES)

build:
	mkdir -p build

$(foreach optim, $(GCC_OPTIMIZATION_LEVEL),$(foreach version,$(PROGRAM_VERSIONS),$(eval $(call PROGRAM_template,$(version),g,$(optim)))))
$(foreach optim, $(CLANG_OPTIMIZATION_LEVEL),$(foreach version,$(PROGRAM_VERSIONS),$(eval $(call PROGRAM_template,$(version),c,$(optim)))))

build/%.go0: $(MISC_DIRECTORY)%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O0

build/%.go1: $(MISC_DIRECTORY)%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O1

build/%.go2: $(MISC_DIRECTORY)%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O2

build/%.go3: $(MISC_DIRECTORY)%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O3

build/%.gos: $(MISC_DIRECTORY)%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -Os

build/%.co0: $(MISC_DIRECTORY)%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O0

build/%.co1: $(MISC_DIRECTORY)%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O1

build/%.co2: $(MISC_DIRECTORY)%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O2

build/%.co3: $(MISC_DIRECTORY)%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O3

build/%.coz: $(MISC_DIRECTORY)%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -Oz

clean:
	@ rm -rf build/*

.PHONY: all clean
