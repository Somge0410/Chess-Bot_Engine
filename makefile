BUILD_DIR := build-openbench
TARGET := Chess-Bot_Engine
EXE ?= Chess-Bot_Engine

ifeq ($(CC),gcc)
CXX := g++
endif

ifeq ($(CC),clang)
CXX := clang++
endif

CXX ?= g++

all:
	@echo "OpenBench requested EXE=$(EXE)"
	cmake -S . -B build-openbench -G "Unix Makefiles" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_COMPILER=$(CXX)
	cmake --build build-openbench --parallel
	cmake -E copy build-openbench/Chess-Bot_Engine $(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)