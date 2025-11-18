# Makefile for QuicheEngine C API Adapter
#
# This builds the C API wrapper library that bridges FFmpeg (C) to QuicheEngine (C++)
#
# Dependencies:
#   - quiche/engine/include/quiche_engine.h
#   - lib/macos/x86_64/libquicheengine.a (or appropriate platform)

# Platform detection
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
    ifeq ($(UNAME_M),arm64)
        ARCH := arm64
    else
        ARCH := x86_64
    endif
else ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
    ARCH := x86_64
else
    $(error Unsupported platform: $(UNAME_S))
endif

# Directories
PLAYER_DIR := $(shell cd ../.. && pwd)
THIRD_PARTY_DIR := $(PLAYER_DIR)/3rd
ENGINE_INCLUDE := $(THIRD_PARTY_DIR)/include
QUICHE_LIB_DIR := $(THIRD_PARTY_DIR)/lib
QUICHE_LIB := $(QUICHE_LIB_DIR)/libquicheengine.a

BUILD_DIR := build
OUTPUT_LIB := $(BUILD_DIR)/libh3adapter.a

# Compiler settings
CXX := clang++
AR := ar
CXXFLAGS := -std=c++11 -O2 -Wall -fPIC
INCLUDES := -I. -I$(ENGINE_INCLUDE)

# Source files
SRCS := quiche_engine_c_api.cpp
OBJS := $(BUILD_DIR)/quiche_engine_c_api.o

# Targets
.PHONY: all clean check-deps

all: check-deps $(OUTPUT_LIB)

check-deps:
	@echo "Checking dependencies..."
	@echo "Player directory: $(PLAYER_DIR)"
	@echo "3rd party directory: $(THIRD_PARTY_DIR)"
	@echo "Engine include: $(ENGINE_INCLUDE)"
	@echo "QuicheEngine library: $(QUICHE_LIB)"
	@if [ ! -f "$(ENGINE_INCLUDE)/quiche_engine.h" ]; then \
		echo "ERROR: quiche_engine.h not found at $(ENGINE_INCLUDE)"; \
		echo "Please copy header file:"; \
		echo "  cp /path/to/quiche/engine/include/quiche_engine.h $(ENGINE_INCLUDE)/"; \
		exit 1; \
	fi
	@if [ ! -f "$(QUICHE_LIB)" ]; then \
		echo "ERROR: libquicheengine.a not found at $(QUICHE_LIB)"; \
		echo "Please copy library file:"; \
		echo "  cp /path/to/lib/macos/x86_64/libquicheengine.a $(QUICHE_LIB_DIR)/"; \
		exit 1; \
	fi
	@echo "✓ All dependencies found"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/quiche_engine_c_api.o: quiche_engine_c_api.cpp quiche_engine_c_api.h | $(BUILD_DIR)
	@echo "Compiling C API wrapper..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OUTPUT_LIB): $(OBJS)
	@echo "Creating static library..."
	$(AR) rcs $@ $(OBJS)
	@echo "✓ Built: $(OUTPUT_LIB)"
	@ls -lh $(OUTPUT_LIB)

clean:
	rm -rf $(BUILD_DIR)

# Help target
help:
	@echo "QuicheEngine C API Adapter Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all (default)  - Build the C API adapter library"
	@echo "  clean          - Remove build artifacts"
	@echo "  check-deps     - Verify dependencies are available"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Output:"
	@echo "  $(OUTPUT_LIB)"
	@echo ""
	@echo "Platform: $(PLATFORM) / $(ARCH)"
