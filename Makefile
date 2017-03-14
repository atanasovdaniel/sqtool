
nothing:
	echo Use make target

CC:=gcc
CXX:=g++

include ./etc/make_lib.inc

SQUIRREL_ROOT:=./squirrel

OBJ_DIR:=./build/obj
LIB_DIR:=./build/lib
EXE_DIR:=./build

LIBRARY_FILES:=

ifeq ($(OS),Windows_NT)
DEFINES:=-DUNICODE -D_UNICODE
CFLAGS:=-static-libgcc -static-libstdc++
else
endif

#CFLAGS+=-O2
CFLAGS+=-g
CFLAGS+=-Wall -fno-strict-aliasing
CXXFLAGS:=-fno-exceptions -fno-rtti

#DEFINES+=
INCLUDES+=-MMD
INCLUDES+=-I./include -I./sqtool -I$(SQUIRREL_ROOT)/include

#######
# Base libraryes

CXX_SOURCE_FILES+=$(SQUIRREL_ROOT)/sqstdlib/sqstdmath.cpp
$(eval $(call MAKE_LIBRARY,libsqstdmath.a))

CXX_SOURCE_FILES+=$(SQUIRREL_ROOT)/sqstdlib/sqstdstring.cpp
CXX_SOURCE_FILES+=$(SQUIRREL_ROOT)/sqstdlib/sqstdrex.cpp
$(eval $(call MAKE_LIBRARY,libsqstdstring.a))

CXX_SOURCE_FILES+=./sqtlib/sqstdsystem.cpp
$(eval $(call MAKE_LIBRARY,libsqstdsystem.a))

CXX_SOURCE_FILES+=./sqtlib/sqstdio.cpp
CXX_SOURCE_FILES+=./sqtlib/sqstdstream.cpp
CXX_SOURCE_FILES+=./sqtlib/sqstdblob.cpp
CXX_SOURCE_FILES+=./sqtlib/sqt_streamreader.cpp
CXX_SOURCE_FILES+=./sqtlib/sqt_squirrelio.cpp
$(eval $(call MAKE_LIBRARY,libsqstdio.a))

CXX_SOURCE_FILES+=./sqtlib/sqt_decl.c
CXX_SOURCE_FILES+=./sqtlib/sqt_wstr.c
CXX_SOURCE_FILES+=./sqtlib/sqt_wstr.c
$(eval $(call MAKE_LIBRARY,libsqtool.a))

#######
# Extension libraryes

$(eval $(call SOURCE_FOLDER,./fs))
$(eval $(call MAKE_LIBRARY,libfs.a))

#######
# List of libraryes

$(LIB_DIR)/symbols.txt: $(LIBRARY_FILES)
	nm -C $^ > $@ || rm $@

$(LIB_DIR)/registered.h: $(LIB_DIR)/symbols.txt
	sed -n -f ./etc/get_registered.sed $< > $@ || rm $@

INCLUDES+=-I$(LIB_DIR)
./sqtool/sqtool.c: $(LIB_DIR)/registered.h

#######
# The SQUIRREL

$(eval $(call SOURCE_FOLDER,$(SQUIRREL_ROOT)/squirrel))
$(eval $(call MAKE_LIBRARY,libsquirrel.a))

#######
# The SQTool

$(eval $(call SOURCE_FOLDER,./sqtool))
CXX_SOURCE_FILES+=$(SQUIRREL_ROOT)/sqstdlib/sqstdaux.cpp
$(eval $(call MAKE_EXE, sqtool))

all: OUT_DIRS $(LIBRARY_FILES) $(EXECUTABLE_FILES)
	echo Done

OUT_DIRS:
	mkdir -p $(sort $(OUTPUT_DIRS))

clean:
	rm -rf $(EXE_DIR)
	
.PHONY: nothing all clean OUT_DIRS
