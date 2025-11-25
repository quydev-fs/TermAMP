CXX      := $(shell echo $${CXX:-g++})
CXXFLAGS := -std=c++17 -Wall -O2 \
            $(CPPFLAGS) $(CFLAGS) \
            $(shell pkg-config --cflags gtk+-3.0 gstreamer-1.0) 

LDFLAGS  := $(shell pkg-config --libs gtk+-3.0 gstreamer-1.0) \
            $(LDFLAGS)

PREFIX   ?= /usr
DESTDIR  ?= 

SRC_DIR := src
INC_DIR := include
OBJ_DIR := build/obj
BIN_DIR := build/bin

TARGET  := $(BIN_DIR)/TermAMP
SRCS    := $(wildcard $(SRC_DIR)/*.cpp)
OBJS    := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

TOTAL := $(words $(SRCS))
CURRENT = $(words $(filter %.o,$(wildcard $(OBJ_DIR)/*.o)))

all: directories compile-all link

compile-all: $(OBJS)

link: $(OBJS)
	@echo "[CHORE] Linking $(TARGET)..."
	@$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Done compiling!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@BUILT=$$(ls $(OBJ_DIR)/*.o 2>/dev/null | wc -l); \
	CURRENT=$$(($$BUILT + 1)); \
	echo "[$$CURRENT/$(TOTAL)] Compiling $<..."; \
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

directories:
	@echo "[CHORE] Initializing build directories"
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# --- INSTALL TARGET UPDATED FOR /etc/TermAMP ---
install: all
	@echo "[INSTALL] Installing binary..."
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/TermAMP
	
	@echo "[INSTALL] Installing assets to $(DESTDIR)$(PREFIX)/etc/TermAMP..."
	mkdir -p $(DESTDIR)$(PREFIX)/etc/TermAMP
	cp -r assets/* $(DESTDIR)$(PREFIX)/etc/TermAMP/

clean:
	@rm -rf build
	@echo "[CLEAN] Done cleaning build artifacts"

.PHONY: all compile-all link clean directories install
