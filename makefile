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
	rm -rf $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR) -G "Unix Makefiles" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_COMPILER=$(CXX)
	cmake --build $(BUILD_DIR) --parallel
	cmake -E copy $(BUILD_DIR)/$(TARGET) $(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)