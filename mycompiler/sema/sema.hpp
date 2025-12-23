#pragma once

#include "../ast/ast.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

enum class Type { Int };

enum class SymbolKind { Var, Func };

struct Symbol {
    SymbolKind kind{SymbolKind::Var};
    Type type{Type::Int};
    std::size_t arity{0}; 
};

// Semantic analysis pass:
// - scope & symbol table management
// - undefined/duplicate identifiers
// - function call existence + arity
// - basic misuse checks (e.g. assigning to a function name)
class Sema final : public ASTVisitor {
private:
    std::vector<std::map<std::string, Symbol>> scopes_;
    bool hasError_{false};

    void enterScope();
    void leaveScope();

    bool declareInCurrentScope(const std::string& name, const Symbol& sym);
    const Symbol* lookup(const std::string& name) const;

    void error(const std::string& msg);

public:
    void analyze(const Program& prog);
    bool hadError() const { return hasError_; }

    // Visitor overrides (must match ASTVisitor signatures exactly)
    void visit(const NumberExpr& expr) override;
    void visit(const IdentExpr& expr) override;
    void visit(const BinaryExpr& expr) override;
    void visit(const CallExpr& expr) override;

    void visit(const VarDeclStmt& stmt) override;
    void visit(const AssignStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const BlockStmt& stmt) override;

    void visit(const Function& func) override;
    void visit(const Program& prog) override;
};