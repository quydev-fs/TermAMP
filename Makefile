CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2 `pkg-config --cflags gtk+-3.0 gstreamer-1.0`
LDFLAGS  := `pkg-config --libs gtk+-3.0 gstreamer-1.0`

SRC_DIR := src
INC_DIR := include
OBJ_DIR := build/obj
BIN_DIR := build/bin

TARGET  := $(BIN_DIR)/TermuxMusic95
SRCS    := $(wildcard $(SRC_DIR)/*.cpp)
OBJS    := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

TOTAL := $(words $(SRCS))
CURRENT = $(words $(filter %.o,$(wildcard $(OBJ_DIR)/*.o)))

all: directories compile-all link

compile-all: $(OBJS)

link: $(OBJS)
	@echo "[CHORE] Linking..."
	@$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Done compiling!"
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@BUILT=$$(ls $(OBJ_DIR)/*.o 2>/dev/null | wc -l); \
	CURRENT=$$(($$BUILT + 1)); \
	echo "[$$CURRENT/$(TOTAL)] Compiling $<..."; \
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

directories:
	@echo "[CHORE] initalizing things for build process"
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

clean:
	rm -rf build

.PHONY: all compile-all link clean directories
