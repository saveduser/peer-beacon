CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Isrc
OS_TARGET ?= linux
OBJ_DIR := obj-$(OS_TARGET)
BIN_DIR := bin-$(OS_TARGET)

LIB_SRCS := $(wildcard src/*.cpp src/platform/*.cpp)
LIB_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(LIB_SRCS))

REG_SRC := register/main.cpp
CHECKIN_SRC := checkin/main.cpp
REG_OBJ := $(OBJ_DIR)/register/main.o
CHECKIN_OBJ := $(OBJ_DIR)/checkin/main.o

TEST_SRCS := $(wildcard tests/*.cpp)
TEST_BINS := $(patsubst tests/%.cpp,$(BIN_DIR)/test_%,$(TEST_SRCS))

ifeq ($(OS_TARGET),windows)
    CXX := x86_64-w64-mingw32-g++
    CXXFLAGS += -static -static-libgcc -static-libstdc++
    LDFLAGS += -lws2_32 -liphlpapi
    REG_TARGET := $(BIN_DIR)/register.exe
    CHECKIN_TARGET := $(BIN_DIR)/checkin.exe
else
    LDFLAGS += -lpthread
    REG_TARGET := $(BIN_DIR)/register_bin
    CHECKIN_TARGET := $(BIN_DIR)/checkin_bin
endif

all: $(REG_TARGET) $(CHECKIN_TARGET)

$(REG_TARGET): $(LIB_OBJS) $(REG_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(CHECKIN_TARGET): $(LIB_OBJS) $(CHECKIN_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_BINS)
	@echo "Running tests..."
	@set -e; for t in $(TEST_BINS); do \
	    echo "--- $$(basename $$t) ---"; \
	    $$t; \
	done

$(BIN_DIR)/test_%: tests/%.cpp $(LIB_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIB_OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean test
