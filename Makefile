# TermuxMusic95 Makefile

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2 -Iinclude -I/data/data/com.termux/files/usr/include
LDFLAGS  := -L/data/data/com.termux/files/usr/lib -lX11 -lmpg123 -lpulse -lpulse-simple -lpthread

# Directory Structure
SRC_DIR := src
INC_DIR := include
OBJ_DIR := build/obj
BIN_DIR := build/bin

# Files
TARGET  := $(BIN_DIR)/TermuxMusic95
SRCS    := $(wildcard $(SRC_DIR)/*.cpp)
OBJS    := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Build Rules
all: directories $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build Complete: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

clean:
	@rm -rf build
	@echo "Cleaned build artifacts."

.PHONY: all clean directories
