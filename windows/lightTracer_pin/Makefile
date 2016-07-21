# set PATH=%PATH%;C:\cygwin\bin;

PIN_ROOT 		:= C:\pin-2.14-71313-msvc12-windows
CONFIG_ROOT 	:= $(PIN_ROOT)\source\tools\Config
SOURCE_DIR 		:= src/
DEFINE			:= /DARCH_32 /D__PIN__

include $(CONFIG_ROOT)/makefile.config

##############################################################
#
# Test targets
#
##############################################################

###### Place all generic definitions here ######

# This defines tests which run tools of the same name.  This is simply for convenience to avoid
# defining the test name twice (once in TOOL_ROOTS and again in TEST_ROOTS).
# Tests defined here should not be defined in TOOL_ROOTS and TEST_ROOTS.
TEST_TOOL_ROOTS := lightTracer

# This defines the tests to be run that were not already defined in TEST_TOOL_ROOTS.
TEST_ROOTS :=

# This defines a list of tests that should run in the "short" sanity. Tests in this list must also
# appear either in the TEST_TOOL_ROOTS or the TEST_ROOTS list.
# If the entire directory should be tested in sanity, assign TEST_TOOL_ROOTS and TEST_ROOTS to the
# SANITY_SUBSET variable in the tests section below (see example in makefile.rules.tmpl).
SANITY_SUBSET :=

# This defines the tools which will be run during the the tests, and were not already defined in
# TEST_TOOL_ROOTS.
TOOL_ROOTS :=

# This defines the static analysis tools which will be run during the the tests. They should not
# be defined in TEST_TOOL_ROOTS. If a test with the same name exists, it should be defined in
# TEST_ROOTS.
# Note: Static analysis tools are in fact executables linked with the Pin Static Analysis Library.
# This library provides a subset of the Pin APIs which allows the tool to perform static analysis
# of an application or dll. Pin itself is not used when this tool runs.
SA_TOOL_ROOTS :=

# This defines all the applications that will be run during the tests.
APP_ROOTS :=

# This defines any additional object files that need to be compiled.
OBJECT_ROOTS :=

# This defines any additional dlls (shared objects), other than the pintools, that need to be compiled.
DLL_ROOTS :=

# This defines any static libraries (archives), that need to be built.
LIB_ROOTS :=


##############################################################
#
# Test recipes
#
##############################################################

# This section contains recipes for tests other than the default.
# See makefile.default.rules for the default test rules.
# All tests in this section should adhere to the naming convention: <testname>.test


##############################################################
#
# Build rules
#
##############################################################

# This section contains the build rules for all binaries that have special build rules.
# See makefile.default.rules for the default build rules.

$(OBJDIR)%$(OBJ_SUFFIX): $(SOURCE_DIR)%.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(DEFINE) $(COMP_OBJ)$@ $<

$(OBJDIR)lightTracer$(PINTOOL_SUFFIX): $(OBJDIR)lightTracer$(OBJ_SUFFIX) $(OBJDIR)whiteList$(OBJ_SUFFIX) $(OBJDIR)traceFile$(OBJ_SUFFIX) $(OBJDIR)codeMap$(OBJ_SUFFIX) $(OBJDIR)windowsComp$(OBJ_SUFFIX)
	$(LINKER) $(TOOL_LDFLAGS) $(LINK_EXE)$@ $^ $(TOOL_LPATHS) $(TOOL_LIBS)

include $(CONFIG_ROOT)/makefile.default.rules