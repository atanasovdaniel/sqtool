
CXX:=g++
CXX_FLAGS:=-g -fno-rtti -std=c++0x -fno-exceptions -fno-strict-aliasing -Wall -Wextra -pedantic -Wcast-qual

#CXX_FLAGS+=-O3

CXX_FLAGS+=-ftest-coverage -fprofile-arcs

TESTROOT:=../..
SQROOT:=$(TESTROOT)/../../../squirrel

OUTDIR:=./out
OBJDIR:=$(OUTDIR)/obj

INCLUDES:=-I$(TESTROOT)/..
INCLUDES+=-I$(TESTROOT)/include
INCLUDES+=-I$(SQROOT)/include

DEFINES:=\
	-DTEST_WO_SQAPI

all: mk_test8 mk_test16 mk_test32

mk_test8: $(OUTDIR)
	$(MAKE) -C$(OUTDIR)/8 -f $(TESTROOT)/Makefile TEST_BITS=8 test

mk_test16: $(OUTDIR)
	$(MAKE) -C$(OUTDIR)/16 -f $(TESTROOT)/Makefile TEST_BITS=16 test

mk_test32: $(OUTDIR)
	$(MAKE) -C$(OUTDIR)/32 -f $(TESTROOT)/Makefile TEST_BITS=32 test

test: $(TESTROOT)/../sqstdtextrw.cpp $(TESTROOT)/src/env.c $(TESTROOT)/src/main.c
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(DEFINES) -DTEST_UTF$(TEST_BITS) $^ -o $@
	./test > out.txt 2> err.txt
	gcov sqstdtextrw.cpp

$(OUTDIR):
	mkdir $(OUTDIR)
	mkdir $(OUTDIR)/8
	mkdir $(OUTDIR)/16
	mkdir $(OUTDIR)/32

clean:
	rm -rf $(OUTDIR)

.PHONY:all clean mk_test8 mk_test16 mk_test32
