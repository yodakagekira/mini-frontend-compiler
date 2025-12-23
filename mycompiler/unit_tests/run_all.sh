#!/usr/bin/env bash
set -euo pipefail

# Run from project root (where lexer/, ast/, parser/, sema/, unit_test/ live).

CXX=${CXX:-c++}
CXXFLAGS=${CXXFLAGS:-"-std=c++20 -O0 -g -Wall -Wextra -pedantic"}

build_one () {
  local name="$1"
  shift
  echo "== Building $name"
  $CXX $CXXFLAGS "$@" -o "unit_tests/$name"
  echo "== Running $name"
  "unit_tests/$name"
  echo
}

build_one test_lexer \
  unit_tests/test_lexer.cpp lexer/lexer.cpp

build_one test_ast \
  unit_tests/test_ast.cpp ast/ast.cpp

build_one test_parser \
  unit_tests/test_parser.cpp lexer/lexer.cpp ast/ast.cpp parser/parser.cpp

build_one test_sema \
  unit_tests/test_sema.cpp lexer/lexer.cpp ast/ast.cpp parser/parser.cpp sema/sema.cpp

echo "All unit tests passed."
