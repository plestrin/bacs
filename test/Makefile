BIN_DIR 	:= build/
SRC_DIR 	:= ../staticSignature/
LOG_DIR 	:= log/
RECIPE_DIR 	:= recipe/
RECIPES 	:= SIG_tea.xml
EXE 		:= signature

define TRACE_template
trace_$(1): $$(RECIPE_DIR)$(1)
	@ if [ ! -d $$(LOG_DIR) ]; then mkdir -p $$(LOG_DIR); fi
	./test.py $$^ TRACE
endef

define SEARCH_template
search_$(1): $$(RECIPE_DIR)$(1)
	@ if [ ! -d $$(LOG_DIR) ]; then mkdir -p $$(LOG_DIR); fi
	./test.py $$^ SEARCH
endef

all: $(EXE)

$(EXE):
	@ if [ ! -d $(BIN_DIR) ]; then mkdir -p $(BIN_DIR); fi
	@ cd $(SRC_DIR) && $(MAKE) VERBOSE=0 DEBUG=0 BUILD_DIR=../test/$(BIN_DIR)

trace: $(foreach recipe,$(RECIPES),trace_$(recipe))

$(foreach recipe,$(RECIPES),$(eval $(call TRACE_template,$(recipe))))

search: $(EXE) $(foreach recipe,$(RECIPES),search_$(recipe))

$(foreach recipe,$(RECIPES),$(eval $(call SEARCH_template,$(recipe))))

clean:
	@ rm -rf $(BIN_DIR)
	@ rm -f $(EXE)

cleaner: clean
	@ rm -r trace*
	@ rm -rf $(LOG_DIR)