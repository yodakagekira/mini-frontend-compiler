#include "test_common.hpp"

#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../ast/ast.hpp"

#include <typeinfo>

static std::unique_ptr<Program> parseProgramOrDie(const std::string& src) {
    Lexer lex(src);
    Parser p(lex);
    auto prog = p.parseProgram();
    TEST_ASSERT(prog != nullptr);
    TEST_ASSERT(!p.hasError());
    return prog;
}

static void test_parse_single_function() {
    const std::string src =
        "int main() {"
        "  int x = 3;"
        "  x = x + 2;"
        "  return x;"
        "}";

    auto prog = parseProgramOrDie(src);
    TEST_ASSERT(prog->getFunctions().size() == 1);

    const auto& fn = *prog->getFunctions()[0];
    TEST_ASSERT(fn.getName() == "main");
    TEST_ASSERT(fn.getParams().empty());

    const auto& stmts = fn.getBody()->getStmts();
    TEST_ASSERT(stmts.size() == 3);

    TEST_ASSERT(dynamic_cast<VarDeclStmt*>(stmts[0].get()) != nullptr);
    TEST_ASSERT(dynamic_cast<AssignStmt*>(stmts[1].get()) != nullptr);
    TEST_ASSERT(dynamic_cast<ReturnStmt*>(stmts[2].get()) != nullptr);

    auto* decl = dynamic_cast<VarDeclStmt*>(stmts[0].get());
    TEST_ASSERT(decl->getName() == "x");

    auto* ret = dynamic_cast<ReturnStmt*>(stmts[2].get());
    TEST_ASSERT(ret != nullptr);
    TEST_ASSERT(dynamic_cast<const IdentExpr*>(ret->getExpr()) != nullptr);
}

static void test_parse_calls_and_params() {
    const std::string src =
        "int add(int a, int b) { return a + b; }"
        "int main() { return add(2, 3); }";

    auto prog = parseProgramOrDie(src);
    TEST_ASSERT(prog->getFunctions().size() == 2);

    const auto& mainFn = *prog->getFunctions()[1];
    TEST_ASSERT(mainFn.getName() == "main");

    const auto& stmts = mainFn.getBody()->getStmts();
    TEST_ASSERT(stmts.size() == 1);

    auto* ret = dynamic_cast<ReturnStmt*>(stmts[0].get());
    TEST_ASSERT(ret != nullptr);

    auto* call = dynamic_cast<const CallExpr*>(ret->getExpr());
    TEST_ASSERT(call != nullptr);
    TEST_ASSERT(call->getCallee() == "add");
    TEST_ASSERT(call->getArgs().size() == 2);
}

int main() {
    int fails = 0;
    fails += run_test("parser: single function", &test_parse_single_function);
    fails += run_test("parser: calls + params", &test_parse_calls_and_params);
    return fails;
}
