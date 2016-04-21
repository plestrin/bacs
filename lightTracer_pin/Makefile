PIN_ROOT 		:= /home/pierre/Documents/tool/pin-3.0-76991-gcc-linux/
TOOLS_ROOT 		:= $(PIN_ROOT)source/tools/
CONFIG_ROOT 	:= $(TOOLS_ROOT)Config/
KIT 			:= 1
COMPILER 		:= gcc
TARGET_OS 		:= linux
SOURCE_DIR 		:= src/
MISC_DIR		:= ../misc/
SHARED_DIR 		:= ../shared/
INCLUDE_SRC 	:= -I$(MISC_DIR) -I$(SHARED_DIR)
INCLUDE_MISC 	:=
INCLUDE_SHARED 	:= -I$(MISC_DIR)
TOOL_ROOTS 		:= lightTracer
OBJECT_ROOTS 	:= whiteList codeMap multiColumn traceFile
ADD_DEFINE		:= -DARCH_32 -D_FILE_OFFSET_BITS=64

ifeq ($(OS),Windows_NT)
	# Add custom code when the time'll come
else
    include $(CONFIG_ROOT)unix.vars
    include $(CONFIG_ROOT)makefile.unix.config
endif

TOOLS 			:= $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX)) $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
OBJECTS 		:= $(OBJECT_ROOTS:%=$(OBJDIR)%$(OBJ_SUFFIX))


.PHONY: all clean

all: dir $(TOOLS) $(OBJECTS)

###### Create the output directory ######

dir: $(OBJDIR)

obj-%:
	mkdir -p $@

###### Cleanup ######

clean:
	@ rm -rf $(OBJDIR) *.out* *.log* *.xml *.makefile.copy *.core core.*


###### Default build rules for tools ######

$(OBJDIR)%$(OBJ_SUFFIX): $(SOURCE_DIR)%.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(INCLUDE_SRC) $(ADD_DEFINE) $(COMP_OBJ)$@ $<

$(OBJDIR)%$(OBJ_SUFFIX): $(MISC_DIR)%.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(INCLUDE_MISC) $(ADD_DEFINE) $(COMP_OBJ)$@ $<

$(OBJDIR)%$(OBJ_SUFFIX): $(SHARED_DIR)%.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(INCLUDE_SHARED) $(ADD_DEFINE) $(COMP_OBJ)$@ $<

$(OBJDIR)%$(PINTOOL_SUFFIX): $(OBJDIR)%$(OBJ_SUFFIX) $(OBJECTS)
	$(LINKER) $(TOOL_LDFLAGS) $(LINK_EXE)$@ $^ $(TOOL_LPATHS) $(TOOL_LIBS)
