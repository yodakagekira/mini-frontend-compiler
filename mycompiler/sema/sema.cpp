#include "sema.hpp"

#include <iostream>

void Sema::error(const std::string& msg) {
    std::cerr << "[sema] " << msg << "\n";
    hasError_ = true;
}

void Sema::enterScope() {
    scopes_.push_back({});
}

void Sema::leaveScope() {
    if (scopes_.empty()) {
        error("internal: attempt to leave scope when no scopes exist");
        return;
    }
    scopes_.pop_back();
}

bool Sema::declareInCurrentScope(const std::string& name, const Symbol& sym) {
    if (scopes_.empty()) {
        error("internal: declare called with no active scope");
        return false;
    }

    auto& cur = scopes_.back();
    if (cur.find(name) != cur.end()) {
        error("duplicate declaration: " + name);
        return false;
    }

    cur.emplace(name, sym);
    return true;
}

const Symbol* Sema::lookup(const std::string& name) const {
    // Search from innermost scope to outermost.
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        const auto& scope = *it;
        auto found = scope.find(name);
        if (found != scope.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

void Sema::analyze(const Program& prog) {
    hasError_ = false;
    scopes_.clear();

    enterScope(); // global
    prog.accept(*this);
    leaveScope();
}

// ---------------- Expressions ----------------

void Sema::visit(const NumberExpr&) {
    // always ok
}

void Sema::visit(const IdentExpr& expr) {
    const Symbol* sym = lookup(expr.getName());
    if (!sym) {
        error("undefined identifier: " + expr.getName());
        return;
    }
    if (sym->kind != SymbolKind::Var) {
        error("identifier used as a variable but declared as a function: " + expr.getName());
    }
}

void Sema::visit(const BinaryExpr& expr) {
    if (expr.getLHS()) expr.getLHS()->accept(*this);
    if (expr.getRHS()) expr.getRHS()->accept(*this);
}

void Sema::visit(const CallExpr& expr) {
    const Symbol* sym = lookup(expr.getCallee());
    if (!sym) {
        error("undefined function: " + expr.getCallee());
    } else if (sym->kind != SymbolKind::Func) {
        error("identifier called as a function but declared as a variable: " + expr.getCallee());
    }

    // Check args
    const auto& args = expr.getArgs();
    for (const auto& arg : args) {
        if (arg) arg->accept(*this);
    }

    if (sym && sym->kind == SymbolKind::Func) {
        if (args.size() != sym->arity) {
            error("wrong number of arguments in call to '" + expr.getCallee() + "': expected " +
                  std::to_string(sym->arity) + ", got " + std::to_string(args.size()));
        }
    }
}

// ---------------- Statements ----------------

void Sema::visit(const VarDeclStmt& stmt) {
    if (stmt.getInit()) stmt.getInit()->accept(*this);

    Symbol sym;
    sym.kind = SymbolKind::Var;
    sym.type = Type::Int;
    declareInCurrentScope(stmt.getName(), sym);
}

void Sema::visit(const AssignStmt& stmt) {
    const Symbol* sym = lookup(stmt.getName());
    if (!sym) {
        error("assignment to undefined variable: " + stmt.getName());
    } else if (sym->kind != SymbolKind::Var) {
        error("cannot assign to function name: " + stmt.getName());
    }

    if (stmt.getValue()) stmt.getValue()->accept(*this);
}

void Sema::visit(const ReturnStmt& stmt) {
    if (stmt.getExpr()) stmt.getExpr()->accept(*this);
}

void Sema::visit(const IfStmt& stmt) {
    if (stmt.getCond()) stmt.getCond()->accept(*this);
    if (stmt.getThen()) stmt.getThen()->accept(*this);
    if (stmt.getElse()) stmt.getElse()->accept(*this);
}

void Sema::visit(const WhileStmt& stmt) {
    if (stmt.getCond()) stmt.getCond()->accept(*this);
    if (stmt.getBody()) stmt.getBody()->accept(*this);
}

void Sema::visit(const BlockStmt& stmt) {
    enterScope();
    for (const auto& s : stmt.getStmts()) {
        if (s) s->accept(*this);
    }
    leaveScope();
}

// ---------------- Top-level ----------------

void Sema::visit(const Function& func) {.
    // Here we just analyze its body in a new scope with parameters.
    enterScope();

    for (const auto& param : func.getParams()) {
        Symbol ps;
        ps.kind = SymbolKind::Var;
        ps.type = Type::Int;
        declareInCurrentScope(param, ps);
    }

    if (func.getBody()) func.getBody()->accept(*this);

    leaveScope();
}

void Sema::visit(const Program& prog) {
    // 1) declare all functions so calls work regardless of order.
    // 2) analyze all function bodies.

    // Pass 1: function declarations
    for (const auto& f : prog.getFunctions()) {
        if (!f) continue;
        Symbol fs;
        fs.kind = SymbolKind::Func;
        fs.type = Type::Int;
        fs.arity = f->getParams().size();
        declareInCurrentScope(f->getName(), fs);
    }

    // Pass 2: function bodies
    for (const auto& f : prog.getFunctions()) {
        if (f) f->accept(*this);
    }
}
