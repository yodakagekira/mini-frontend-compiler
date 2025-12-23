#include "test_common.hpp"

#include "../ast/ast.hpp"
#include "../lexer/token.hpp"

#include <string>
#include <vector>

struct CountingVisitor final : ASTVisitor {
    int numbers = 0;
    int idents  = 0;
    int binaries = 0;
    int calls = 0;

    int decls = 0;
    int assigns = 0;
    int returns = 0;
    int ifs = 0;
    int whiles = 0;
    int blocks = 0;

    int funcs = 0;
    int programs = 0;

    std::vector<TokenType> binOps;
    std::vector<std::string> names;

    void visit(const NumberExpr& expr) override { (void)expr; numbers++; }
    void visit(const IdentExpr& expr) override  { idents++; names.push_back(expr.getName()); }
    void visit(const BinaryExpr& expr) override {
        binaries++;
        binOps.push_back(expr.getOp());
        expr.getLHS()->accept(*this);
        expr.getRHS()->accept(*this);
    }
    void visit(const CallExpr& expr) override {
        calls++;
        names.push_back(expr.getCallee());
        for (auto& a : expr.getArgs()) a->accept(*this);
    }

    void visit(const VarDeclStmt& stmt) override {
        decls++;
        names.push_back(stmt.getName());
        stmt.getInit()->accept(*this);
    }
    void visit(const AssignStmt& stmt) override {
        assigns++;
        names.push_back(stmt.getName());
        stmt.getValue()->accept(*this);
    }
    void visit(const ReturnStmt& stmt) override {
        returns++;
        stmt.getExpr()->accept(*this);
    }
    void visit(const IfStmt& stmt) override {
        ifs++;
        stmt.getCond()->accept(*this);
        stmt.getThen()->accept(*this);
        if (stmt.getElse()) stmt.getElse()->accept(*this);
    }
    void visit(const WhileStmt& stmt) override {
        whiles++;
        stmt.getCond()->accept(*this);
        stmt.getBody()->accept(*this);
    }
    void visit(const BlockStmt& stmt) override {
        blocks++;
        for (auto& s : stmt.getStmts()) s->accept(*this);
    }

    void visit(const Function& func) override {
        funcs++;
        names.push_back(func.getName());
        func.getBody()->accept(*this);
    }
    void visit(const Program& prog) override {
        programs++;
        for (auto& f : prog.getFunctions()) f->accept(*this);
    }
};

static void test_ast_construction_and_traversal() {
    // int main(){ int x=3; x=x+2; return x; }
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclStmt>(
        "x", std::make_unique<NumberExpr>(3)));

    stmts.push_back(std::make_unique<AssignStmt>(
        "x",
        std::make_unique<BinaryExpr>(
            TokenType::plus,
            std::make_unique<IdentExpr>("x"),
            std::make_unique<NumberExpr>(2))));

    stmts.push_back(std::make_unique<ReturnStmt>(std::make_unique<IdentExpr>("x")));

    auto body = std::make_unique<BlockStmt>(std::move(stmts));
    std::vector<std::string> params;
    auto fn = std::make_unique<Function>("main", std::move(params), std::move(body));

    std::vector<std::unique_ptr<Function>> fns;
    fns.push_back(std::move(fn));
    Program prog(std::move(fns));

    CountingVisitor v;
    prog.accept(v);

    TEST_ASSERT(v.programs == 1);
    TEST_ASSERT(v.funcs == 1);
    TEST_ASSERT(v.blocks == 1);

    TEST_ASSERT(v.decls == 1);
    TEST_ASSERT(v.assigns == 1);
    TEST_ASSERT(v.returns == 1);

    TEST_ASSERT(v.binaries == 1);
    TEST_ASSERT(v.binOps.size() == 1 && v.binOps[0] == TokenType::plus);

    // Numbers: 3 and 2
    TEST_ASSERT(v.numbers == 2);
    TEST_ASSERT(!v.names.empty());
}

int main() {
    int fails = 0;
    fails += run_test("ast: construction + traversal", &test_ast_construction_and_traversal);
    return fails;
}
