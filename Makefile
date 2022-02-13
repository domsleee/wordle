#ifndef
ifdef ENV_DEBUG
	CONDITIONAL_CXX = -g
else
	CONDITIONAL_CXX = -O3 -g
endif


UNAME := $(shell uname)
CXX = g++-11
CXXFLAGS = -std=c++20 -Wall $(CONDITIONAL_CXX)
CXXFLAGSTEST = -std=c++20 -Wall -g
LIBS := -ltbb -lprofiler
ifeq ($(UNAME), Darwin)
LIBS := -lprofiler -L/opt/homebrew/Cellar/gperftools/2.9.1_1/lib -L/opt/homebrew/Cellar/tbb/2021.5.0/lib
endif

SRC_DIR := src
TEST_DIR := test
OBJ_DIR := obj
BIN_DIR := bin
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
SRC_FILES += $(wildcard $(TEST_DIR)/*.cpp)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
OBJ_FILES_NO_TEST := $(filter-out $(wildcard $(TEST_DIR)/*), $(OBJ_FILES))
OBJ_FILES := $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(OBJ_FILES))
OBJ_FILES_NO_MAIN := $(filter-out $(OBJ_DIR)/main.o, $(OBJ_FILES))

LDFLAGS := $(LIBS)

$(BIN_DIR)/solve: $(OBJ_FILES_NO_TEST)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/test: $(OBJ_FILES_NO_MAIN)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ENV_CXXFLAGS) -c -o $@ $< $(LIBS)

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGSTEST) -c -o $@ $< $(LIBS)

clean:
	- rm -r $(OBJ_DIR)/* $(BIN_DIR)/*
