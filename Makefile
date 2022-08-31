#ifndef
UNAME := $(shell uname)
ifdef ENV_DEBUG
	CONDITIONAL_CXX = -g -ffast-math
else
	CONDITIONAL_CXX = -DNDEBUG -g #-fprofile-use=./prof/out_single.pgo #-lgcov
ifneq ($(UNAME), Darwin)
	CONDITIONAL_CXX += #-fsanitize=address -fno-omit-frame-pointer
endif
endif


CXX = g++-11
CXXFLAGSCOMMON = -std=c++20 -Wall -Iinclude -Ithird_party -ffast-math $(ENV_CXXFLAGS) $(CONDITIONAL_CXX)  -O3 -MP -MD
CXXFLAGS = $(CXXFLAGSCOMMON)
CXXFLAGSTEST = $(CXXFLAGSCOMMON)
LIBS := -ltbb #-lprofiler $(ENV_LIBFLAGS) #-lgcov -fprofile-use=./prof/out_single2.pgo
ifeq ($(UNAME), Darwin)
LIBS := #-lprofiler -L/opt/homebrew/Cellar/gperftools/2.9.1_1/lib -L/opt/homebrew/Cellar/tbb/2021.5.0/lib
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
INCLUDES = $(wildcard include/*.h)

LDFLAGS := $(LIBS)
DEPS := $(OBJ_FILES_NO_TEST:.o=.d)
TEST_DEPS := $(OBJ_FILES_NO_MAIN:.o=.d)

$(BIN_DIR)/solve: $(OBJ_FILES_NO_TEST)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/test: $(OBJ_FILES_NO_MAIN)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

-include $(DEPS)
-include $(TEST_DEPS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< $(LIBS)

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGSTEST) -c -o $@ $< $(LIBS)

clean:
	- rm -r $(OBJ_DIR)/* $(BIN_DIR)/*

all: $(BIN_DIR)/solve

# debug:
# 	@echo $(OBJ_FILES_NO_TEST)
