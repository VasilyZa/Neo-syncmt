# Makefile for syncmt

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -march=haswell
LDFLAGS := -pthread

# Version info
VERSION ?= 1.6.0
GIT_COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_DATE ?= $(shell date +%Y-%m-%d)

DEFINES := -DVERSION=\"$(VERSION)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\" -DBUILD_DATE=\"$(BUILD_DATE)\"

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := bin

# Include path
INCLUDES := -I$(INC_DIR)

# Source files (exclude async_io.cpp - no longer used)
SRCS := $(filter-out $(SRC_DIR)/async_io.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target
TARGET := $(BIN_DIR)/syncmt

# Default target
.PHONY: all
all: $(TARGET)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

# Dependencies
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(INC_DIR)/common.h $(INC_DIR)/file_copier.h
$(BUILD_DIR)/common.o: $(SRC_DIR)/common.cpp $(INC_DIR)/common.h
$(BUILD_DIR)/thread_pool.o: $(SRC_DIR)/thread_pool.cpp $(INC_DIR)/thread_pool.h $(INC_DIR)/common.h $(INC_DIR)/file_descriptor.h
$(BUILD_DIR)/file_copier.o: $(SRC_DIR)/file_copier.cpp $(INC_DIR)/file_copier.h $(INC_DIR)/common.h $(INC_DIR)/thread_pool.h

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Debug build
.PHONY: debug
debug: CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0 -DDEBUG
debug: clean $(TARGET)

# Release build with static linking
.PHONY: release
release: LDFLAGS := -pthread -static
release: clean $(TARGET)

# Install
.PHONY: install
install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/

# Uninstall
.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/usr/local/bin/syncmt

# Help
.PHONY: help
help:
	@echo "syncmt Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the project (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build with static linking"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  VERSION=$(VERSION)"
	@echo "  GIT_COMMIT=$(GIT_COMMIT)"
	@echo "  BUILD_DATE=$(BUILD_DATE)"
