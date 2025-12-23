# Compiler
CXX      := clang++
CXXFLAGS := -std=c++20 -O0 -g -Wall -Wextra -pedantic

# LLVM
LLVM_CONFIG := llvm-config
LLVM_CXXFLAGS := $(filter-out -std=%,$(shell llvm-config --cxxflags))
LLVM_LDFLAGS  := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS     := $(shell $(LLVM_CONFIG) --libs core support)
LLVM_SYS_LIBS := $(shell $(LLVM_CONFIG) --system-libs)

# Target
TARGET := mycompiler

# Sources
SRC := \
  main.cpp \
  lexer/lexer.cpp \
  parser/parser.cpp \
  ast/ast.cpp \
  sema/sema.cpp \
  llvm/llvm_codegen.cpp

OBJ := $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LLVM_LDFLAGS) $(LLVM_LIBS) $(LLVM_SYS_LIBS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
