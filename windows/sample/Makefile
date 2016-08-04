# set PATH=%PATH%;C:\cygwin\bin;

CXX 		:= CL.exe
CXXFLAGS 	:= /c /nologo /Wall
LINKER 		:= link.exe
LDFLAGS 	:= /nologo /debug
SOURCE_DIR 	:= src/
BUILD_DIR 	:= build
TRACE_DIR 	:= trace/
WHL_DIR 	:= trace\whiteList
PIN 		:= C:\pin-2.14-71313-msvc12-windows\pin
TOOL 		:= E:\lightTracer_pin\obj-ia32\lightTracer.dll
TARGETS 	:= TEST_TEA TEST_RC4 TEST_SHA1 TEST_MD5 TEST_AES GLAD_AES_V0 GLAD_AES_V1 GLAD_AES_V2 GLAD_AES_V3
TARGETS_MEM := TEST_CBC_TEA TEST_CBC_AES TEST_CTR_TEA TEST_CTR_AES
TARGETS_ALL := $(foreach target,$(TARGETS) $(TARGETS_MEM),$(target)_WD) $(foreach target,$(TARGETS) $(TARGETS_MEM),$(target)_WR)

all: build $(foreach target,$(TARGETS_ALL),$(BUILD_DIR)/$(shell echo $(target) | tr A-Z a-z).exe $(TRACE_DIR)trace$(target))

build:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.wd.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /Od /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.wr.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /Ox /Fo$(COMP_OBJ)$@ $<

# Specific rules for GLAD_AES
$(BUILD_DIR)/%.v0.wd.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /Od /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v0.wr.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /Ox /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v1.wd.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /DENC_UNROLL=PARTIAL /DDEC_UNROLL=PARTIAL /Od /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v1.wr.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /DENC_UNROLL=PARTIAL /DDEC_UNROLL=PARTIAL /Ox /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v2.wd.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /DENC_UNROLL=NONE /DDEC_UNROLL=NONE /DNO_ENC_KS_UNROLL /DNO_DEC_KS_UNROLL /Od /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v2.wr.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /DENC_UNROLL=NONE /DDEC_UNROLL=NONE /DNO_ENC_KS_UNROLL /DNO_DEC_KS_UNROLL /Ox /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v3.wd.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) -DENC_ROUND=ONE_TABLE /DLAST_ENC_ROUND=ONE_TABLE /DDEC_ROUND=ONE_TABLE /DLAST_DEC_ROUND=ONE_TABLE /DKEY_SCHED=ONE_TABLE /Od /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.v3.wr.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) -DENC_ROUND=ONE_TABLE /DLAST_ENC_ROUND=ONE_TABLE /DDEC_ROUND=ONE_TABLE /DLAST_DEC_ROUND=ONE_TABLE /DKEY_SCHED=ONE_TABLE /Ox /Fo$(COMP_OBJ)$@ $<

define BUILD_template
$$(BUILD_DIR)/$(1)_wd.exe: $$(foreach obj,$(2),$$(BUILD_DIR)/$$(obj).wd.obj)
	$$(LINKER) $$(LDFLAGS) /out:$$@ $$^
$$(BUILD_DIR)/$(1)_wr.exe: $$(foreach obj,$(2),$$(BUILD_DIR)/$$(obj).wr.obj)
	$$(LINKER) $$(LDFLAGS) /out:$$@ $$^
endef

define TRACE_template
$$(TRACE_DIR)trace$(1): $$(BUILD_DIR)/$(2).exe $$(WHL_DIR)\$(1).lst $$(TOOL)
	mkdir -p $$@
	rm -f $$@/*
	$$(PIN) -t $$(TOOL) -w $$(WHL_DIR)\$(1).lst -o $$@ -- $$(BUILD_DIR)\$(2).exe
endef

define TRACE_MEM_template
$$(TRACE_DIR)trace$(1): $$(BUILD_DIR)/$(2).exe $$(WHL_DIR)\$(1).lst $$(TOOL)
	mkdir -p $$@
	rm -f $$@/*
	$$(PIN) -t $$(TOOL) -m -w $$(WHL_DIR)\$(1).lst -o $$@ -- $$(BUILD_DIR)\$(2).exe
endef

$(eval $(call BUILD_template,test_tea,TEA test_tea printBuffer))
$(eval $(call BUILD_template,test_rc4,RC4 test_rc4 printBuffer))
$(eval $(call BUILD_template,test_sha1,SHA1 test_sha1 printBuffer))
$(eval $(call BUILD_template,test_md5,MD5 test_md5 printBuffer))
$(eval $(call BUILD_template,test_aes,AES test_aes printBuffer))
$(eval $(call BUILD_template,glad_aes_v0,aescrypt.v0 aeskey.v0 aes_modes.v0 aestab.v0 glad_aes.v0 printBuffer))
$(eval $(call BUILD_template,glad_aes_v1,aescrypt.v1 aeskey.v1 aes_modes.v1 aestab.v1 glad_aes.v1 printBuffer))
$(eval $(call BUILD_template,glad_aes_v2,aescrypt.v2 aeskey.v2 aes_modes.v2 aestab.v2 glad_aes.v2 printBuffer))
$(eval $(call BUILD_template,glad_aes_v3,aescrypt.v3 aeskey.v3 aes_modes.v3 aestab.v3 glad_aes.v3 printBuffer))
$(eval $(call BUILD_template,test_cbc_tea,TEA mode test_cbc_tea printBuffer))
$(eval $(call BUILD_template,test_cbc_aes,AES mode test_cbc_aes printBuffer))
$(eval $(call BUILD_template,test_ctr_tea,TEA mode test_ctr_tea printBuffer))
$(eval $(call BUILD_template,test_ctr_aes,AES mode test_ctr_aes printBuffer))

$(foreach target,$(foreach target,$(TARGETS),$(target)_WD) $(foreach target,$(TARGETS),$(target)_WR),$(eval $(call TRACE_template,$(target),$(shell echo $(target) | tr A-Z a-z))))
$(foreach target,$(foreach target,$(TARGETS_MEM),$(target)_WD) $(foreach target,$(TARGETS_MEM),$(target)_WR),$(eval $(call TRACE_MEM_template,$(target),$(shell echo $(target) | tr A-Z a-z))))

clean:
	@ rm -rf $(BUILD_DIR)
	@ $(foreach target,$(TARGETS_ALL), rm -rf $(TRACE_DIR)trace$(target))
	@ rm -f pintool.log