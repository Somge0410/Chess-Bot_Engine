BUILD_DIR := build-openbench
TARGET := Chess-Bot_Engine
EXE ?= Chess-Bot_Engine
# Keep small OpenBench hosts responsive. Override explicitly on larger machines.
BUILD_JOBS ?= 1

ifeq ($(CC),gcc)
CXX := g++
endif

ifeq ($(CC),clang)
CXX := clang++
endif

CXX ?= g++

.PHONY: all clean

all:
	@echo "OpenBench requested EXE=$(EXE)"
	rm -rf $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR) -G "Unix Makefiles" \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_TESTING=OFF \
		-DENABLE_QSEARCH_DIAGNOSTICS=OFF \
		-DENABLE_SPSA_TUNING=OFF \
		-DCMAKE_CXX_COMPILER=$(CXX)
	cmake --build $(BUILD_DIR) --target $(TARGET) --parallel "$(BUILD_JOBS)"
	cmake -E copy $(BUILD_DIR)/$(TARGET) $(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)
