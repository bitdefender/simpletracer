mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

annotated_tracer := tracer.annotated
CC := gcc
CXX := g++
CPP_FILES := BinLog.cpp annotated.tracer.cpp
OBJ_FILES := BinLog.o annotated.tracer.o
LD_PATHS := -L$(makefile_dir)/../libs

LD_FLAGS := -lsymbolicenvironment -lexecution -lpthread -ldl
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall="" -I../river.sdk/include/
NO_SSE := -mno-mmx -mno-sse -march=i386
CXX_FLAGS += -m32 $(NO_SSE) -std=c++11 $(CC_FLAGS_CROSS)
CC_FLAGS += -m32 $(NO_SSE) $(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(annotated_tracer)

install: $(annotated_tracer)
	install -m 0755 $(annotated_tracer) -t $(prefix)/bin -D

$(annotated_tracer): $(OBJ_FILES)
	$(CXX) $(LD_PATHS) $(CXX_FLAGS) -o $@ $(OBJ_FILES) $(LD_FLAGS)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) -r $(annotated_tracer) $(OBJ_FILES_SO) $(OBJ_FILES) ./inst

