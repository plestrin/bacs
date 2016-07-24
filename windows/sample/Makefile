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
TARGETS 	:= TEST_TEA TEST_RC4 TEST_SHA1
TARGETS_D 	:= $(foreach  target,$(TARGETS),$(target)_WD)
TARGETS_R 	:= $(foreach  target,$(TARGETS),$(target)_WR)

all: build $(foreach target,$(TARGETS_D) $(TARGETS_R),$(BUILD_DIR)/$(shell echo $(target) | tr A-Z a-z).exe $(TRACE_DIR)trace$(target))

build:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.wd.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /Od /Fo$(COMP_OBJ)$@ $<

$(BUILD_DIR)/%.wr.obj: $(SOURCE_DIR)%.c
	$(CXX) $(CXXFLAGS) /Ox /Fo$(COMP_OBJ)$@ $<

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

$(eval $(call BUILD_template,test_tea,TEA test_tea printBuffer))
$(eval $(call BUILD_template,test_rc4,RC4 test_rc4 printBuffer))
$(eval $(call BUILD_template,test_sha1,SHA1 test_sha1 printBuffer))

$(foreach target,$(TARGETS_D) $(TARGETS_R),$(eval $(call TRACE_template,$(target),$(shell echo $(target) | tr A-Z a-z))))

clean:
	@ rm -rf $(BUILD_DIR)
	@ $(foreach target,$(TARGETS_D) $(TARGETS_R), rm -rf $(TRACE_DIR)trace$(target))
	@ rm -f pintool.log