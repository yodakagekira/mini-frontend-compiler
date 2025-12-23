#include "test_common.hpp"

#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../sema/sema.hpp"

#include <sstream>

// Helper: parse source into Program
static std::unique_ptr<Program> parseProgramOrDie(const std::string& src) {
    Lexer lex(src);
    Parser p(lex);
    auto prog = p.parseProgram();
    TEST_ASSERT(prog != nullptr);
    TEST_ASSERT(!p.hasError());
    return prog;
}

static bool runSema(const std::string& src, std::string* diagnostics = nullptr) {
    auto prog = parseProgramOrDie(src);

    // Capture stderr diagnostics (so unit tests stay clean)
    std::stringstream captured;
    auto* old = std::cerr.rdbuf(captured.rdbuf());

    Sema sema;
    sema.analyze(*prog);

    std::cerr.rdbuf(old);
    if (diagnostics) *diagnostics = captured.str();
    return sema.hadError();
}

static void test_sema_ok_program() {
    const std::string src =
        "int add(int a, int b) { return a + b; }"
        "int main() {"
        "  int x = 2;"
        "  int y = 3;"
        "  return add(x, y);"
        "}";
    TEST_ASSERT(runSema(src) == false);
}

static void test_sema_undefined_variable() {
    const std::string src =
        "int main() { return x; }";
    std::string diag;
    TEST_ASSERT(runSema(src, &diag) == true);
    TEST_ASSERT(diag.find("Undefined") != std::string::npos || diag.find("undefined") != std::string::npos);
}

static void test_sema_duplicate_decl() {
    const std::string src =
        "int main() { int x = 1; int x = 2; return x; }";
    TEST_ASSERT(runSema(src) == true);
}

static void test_sema_call_forward_decl_ok() {
    // This should pass if you predeclare functions (two-pass over Program)
    const std::string src =
        "int main() { return add(2, 3); }"
        "int add(int a, int b) { return a + b; }";
    // If this fails, your sema is single-pass and needs a predeclare pass.
    TEST_ASSERT(runSema(src) == false);
}

static void test_sema_wrong_arity() {
    const std::string src =
        "int add(int a, int b) { return a + b; }"
        "int main() { return add(1); }";
    TEST_ASSERT(runSema(src) == true);
}

static void test_sema_scope_rules() {
    const std::string src =
        "int main() { { int x = 1; } return x; }";
    TEST_ASSERT(runSema(src) == true);
}

int main() {
    int fails = 0;
    fails += run_test("sema: ok program", &test_sema_ok_program);
    fails += run_test("sema: undefined variable", &test_sema_undefined_variable);
    fails += run_test("sema: duplicate decl", &test_sema_duplicate_decl);
    fails += run_test("sema: forward call ok", &test_sema_call_forward_decl_ok);
    fails += run_test("sema: wrong arity", &test_sema_wrong_arity);
    fails += run_test("sema: scope rules", &test_sema_scope_rules);
    return fails;
}
