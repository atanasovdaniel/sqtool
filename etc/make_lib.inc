# Copyright (c) 2016-2017 Daniel Atanasov

# OBJ_DIR:=obj

# AR - Archive-maintaining program; default ‘ar’.
# AS - Program for compiling assembly files; default ‘as’.
# CC - Program for compiling C programs; default ‘cc’.
# CXX - Program for compiling C++ programs; default ‘g++’.
# CPP - Program for running the C preprocessor, with results to standard output; default ‘$(CC) -E’.
#
# ASFLAGS - Extra flags to give to the assembler (when explicitly invoked on a ‘.s’ or ‘.S’ file).
# ARFLAGS - Flags to give the archive-maintaining program; default ‘rv’.
# CFLAGS - Extra flags to give to the C compiler.
# CXXFLAGS - Extra flags to give to the C++ compiler.
# LDFLAGS - Extra flags to give to compilers when they are supposed to invoke the linker, ‘ld’, such as -L. Libraries (-lfoo) should be added to the LDLIBS variable instead.
# LDLIBS - Library flags or names given to compilers when they are supposed to invoke the linker, ‘ld’. LOADLIBES is a deprecated (but still supported) alternative to LDLIBS. Non-library linker flags, such as -L, should go in the LDFLAGS variable.
#
# Compiling C programs
# 	$(CC) $(CPPFLAGS) $(CFLAGS) -c
# Compiling C++ programs
# 	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c
# Assembling and preprocessing assembler programs
# 	for .s files - $(AS) $(ASFLAGS)
# 	for .S files - $(CPP) $(CPPFLAGS)
# Linking a single object file
# 	$(CC) $(LDFLAGS) n.o $(LOADLIBES) $(LDLIBS)

CPPFLAGS=$(INCLUDES) $(DEFINES)

OUTPUT_DIRS:=
OBJECT_FILES:=
LIBRARY_FILES:=

#######################################
#	SECTIONS
#######################################

define SECTION_BEGIN
SECT_PREFIX:=$(SECT_PREFIX)_
$$(info sect-begin [$$(SECT_PREFIX)])
$$(SECT_PREFIX)C_SOURCE_FILES:=$(C_SOURCE_FILES)
C_SOURCE_FILES:=
$$(SECT_PREFIX)CXX_SOURCE_FILES:=$(CXX_SOURCE_FILES)
CXX_SOURCE_FILES:=
$$(SECT_PREFIX)ASM_SOURCE_FILES:=$(ASM_SOURCE_FILES)
ASM_SOURCE_FILES:=
$$(SECT_PREFIX)OBJECT_FILES:=$(OBJECT_FILES)
OBJECT_FILES:=
$$(SECT_PREFIX)LIBRARY_FILES:=$(LIBRARY_FILES)
LIBRARY_FILES:=
$$(SECT_PREFIX)EXE_DIR:=$(EXE_DIR)
$$(SECT_PREFIX)LIB_DIR:=$(LIB_DIR)
$$(SECT_PREFIX)OBJ_DIR:=$(OBJ_DIR)
$$(SECT_PREFIX)INCLUDES:=$(INCLUDES)
$$(SECT_PREFIX)DEFINES:=$(DEFINES)
endef

define SECTION_END
$$(info sect-end [$(SECT_PREFIX)])
C_SOURCE_FILES:=$($(SECT_PREFIX)C_SOURCE_FILES) $(C_SOURCE_FILES)
CXX_SOURCE_FILES:=$($(SECT_PREFIX)CXX_SOURCE_FILES) $(CXX_SOURCE_FILES)
ASM_SOURCE_FILES:=$($(SECT_PREFIX)ASM_SOURCE_FILES) $(ASM_SOURCE_FILES)
OBJECT_FILES:=$($(SECT_PREFIX)OBJECT_FILES) $(OBJECT_FILES)
LIBRARY_FILES:=$($(SECT_PREFIX)LIBRARY_FILES) $(LIBRARY_FILES)
EXE_DIR:=$($(SECT_PREFIX)EXE_DIR)
LIB_DIR:=$($(SECT_PREFIX)LIB_DIR)
OBJ_DIR:=$($(SECT_PREFIX)OBJ_DIR)
INCLUDES:=$($(SECT_PREFIX)INCLUDES)
DEFINES:=$($(SECT_PREFIX)DEFINES)
SECT_PREFIX:=$(SECT_PREFIX:_=)
endef

#######################################
#	SOURCES
#######################################

define SOURCE_FOLDER
C_SOURCE_FILES+=$(wildcard $(1)/*.c)
CXX_SOURCE_FILES+=$(wildcard $(1)/*.cpp)
ASM_SOURCE_FILES+=$(wildcard $(1)/*.asm)
endef

define SOURCE_FOLDERS
#$$(info fld $(1))
$(foreach FLD, $(1), $(eval $(call SOURCE_FOLDER,$(FLD))))
endef

define SOURCE_FOLDERS_REC
#$$(info fld-rec $(1))
$(call SOURCE_FOLDERS,$(shell find $(1) -type d))
endef

#######################################
#	SOURCE BUILD RULES
#######################################

define _COMPILE_C_FILE
$$(info src-c [$(1)])
$(OBJ_DIR)/$(basename $(notdir $(1))).o: $(1)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $$@ $$<

OBJECT_FILES+=$(OBJ_DIR)/$(basename $(notdir $(1))).o
endef

define _COMPILE_CXX_FILE
$$(info src-cpp [$(1)])
$(OBJ_DIR)/$(basename $(notdir $(1))).o: $(1)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $$@ $$<

OBJECT_FILES+=$(OBJ_DIR)/$(basename $(notdir $(1))).o
endef

define _COMPILE_ASM_FILE
$$(info src-cpp [$(1)])
$(OBJ_DIR)/$(basename $(notdir $(1))).o: $(1)
	echo $(AS) $(ASFLAGS) -o $$@ $$<

OBJECT_FILES+=$(OBJ_DIR)/$(basename $(notdir $(1))).o
endef

define COMPILE_SOURCES
$(foreach SRC,$(C_SOURCE_FILES),$(eval $(call _COMPILE_C_FILE,$(SRC))))
$(foreach SRC,$(CXX_SOURCE_FILES),$(eval $(call _COMPILE_CXX_FILE,$(SRC))))
$(foreach SRC,$(ASM_SOURCE_FILES),$(eval $(call _COMPILE_ASM_FILE,$(SRC))))
-include $(OBJECT_FILES:%.o=%.d)
C_SOURCE_FILES:=
CXX_SOURCE_FILES:=
ASM_SOURCE_FILES:=
endef

#######################################
#	OUTPUT DIRS
#######################################

define MAKE_OUTPUT_DIRS
.PHONY: $(strip $(1))
$(strip $(1)):
	echo mkdir -p $(sort $(dir $(2)))
endef

#######################################
#	LIBRARY
#######################################

define MAKE_LIBRARY
$$(info mk-lib [$(1)])
$(eval INCLUDES+=$(addprefix -I,$(sort $(dir $(C_SOURCE_FILES) $(CXX_SOURCE_FILES)))))
$(COMPILE_SOURCES)
OUTPUT_DIRS+=$(sort $(LIB_DIR) $(dir $(OBJECT_FILES)))
$(LIB_DIR)/$(strip $(1)): $(OBJECT_FILES)
	$(AR) $(ARFLAGS) $$@ $$^
OBJECT_FILES:=
LIBRARY_FILES+=$(LIB_DIR)/$(strip $(1))
endef

#######################################
#	EXECUTABLE
#######################################

define MAKE_EXE
$$(info mk-exe [$(1)])
$(eval INCLUDES+=$(addprefix -I,$(sort $(dir $(C_SOURCE_FILES) $(CXX_SOURCE_FILES)))))
$(COMPILE_SOURCES)
OUTPUT_DIRS+=$(sort $(EXE_DIR) $(dir $(OBJECT_FILES)))
$(EXE_DIR)/$(strip $(1)): $(OBJECT_FILES) $(LIBRARY_FILES)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $$@ $$^ $(LDLIBS)
OBJECT_FILES:=
EXECUTABLE_FILES+=$(EXE_DIR)/$(strip $(1))
endef

#######################################
