SUB_DIRECTORY := asm syntheticSample staticSignature lightTracer_pin misc/test doc

define BUILD_template
build_$(1):
	@ cd $(1)/ && $$(MAKE)
endef

define CLEAN_template
clean_$(1):
	@ cd $(1)/ && $$(MAKE) clean
endef

all: $(foreach sub_dir,$(SUB_DIRECTORY),build_$(sub_dir))

$(foreach sub_dir,$(SUB_DIRECTORY),$(eval $(call BUILD_template,$(sub_dir))))

clean: $(foreach sub_dir,$(SUB_DIRECTORY),clean_$(sub_dir))

$(foreach sub_dir,$(SUB_DIRECTORY),$(eval $(call CLEAN_template,$(sub_dir))))

.PHONY: all clean