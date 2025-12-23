#pragma once

#include "../lexer/token.hpp"  

#include <memory>
#include <string>
#include <vector>

class ASTVisitor;

// Base AST node for expressions
class Expr {
public:
    virtual ~Expr() = default;
    virtual void accept(ASTVisitor& visitor) const = 0;
};

// Number literal
class NumberExpr : public Expr {
private:
    int value;

public:
    explicit NumberExpr(int value) : value(value) {}
    int getValue() const { return value; }
    void accept(ASTVisitor& visitor) const override;
};

// Identifier
class IdentExpr : public Expr {
private:
    std::string name;

public:
    explicit IdentExpr(std::string name) : name(std::move(name)) {}
    const std::string& getName() const { return name; }
    void accept(ASTVisitor& visitor) const override;
};

// Binary expression
class BinaryExpr : public Expr {
private:
    TokenType op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

public:
    BinaryExpr(TokenType op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    TokenType getOp() const { return op; }
    const Expr* getLHS() const { return lhs.get(); }
    const Expr* getRHS() const { return rhs.get(); }
    void accept(ASTVisitor& visitor) const override;
};

// Function call expression
class CallExpr : public Expr {
private:
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;

public:
    CallExpr(std::string callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), args(std::move(args)) {}
    const std::string& getCallee() const { return callee; }
    const std::vector<std::unique_ptr<Expr>>& getArgs() const { return args; }
    void accept(ASTVisitor& visitor) const override;
};

// Base for statements
class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(ASTVisitor& visitor) const = 0;
};

// Variable declaration (evolved from WithDecl)
class VarDeclStmt : public Stmt {
private:
    std::string name;
    std::unique_ptr<Expr> init;  // Optional initializer

public:
    VarDeclStmt(std::string name, std::unique_ptr<Expr> init)
        : name(std::move(name)), init(std::move(init)) {}
    const std::string& getName() const { return name; }
    const Expr* getInit() const { return init.get(); }
    void accept(ASTVisitor& visitor) const override;
};

// Assignment statement
class AssignStmt : public Stmt {
private:
    std::string name;
    std::unique_ptr<Expr> value;

public:
    AssignStmt(std::string name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}
    const std::string& getName() const { return name; }
    const Expr* getValue() const { return value.get(); }
    void accept(ASTVisitor& visitor) const override;
};

// Return statement
class ReturnStmt : public Stmt {
private:
    std::unique_ptr<Expr> expr;

public:
    explicit ReturnStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
    const Expr* getExpr() const { return expr.get(); }
    void accept(ASTVisitor& visitor) const override;
};

// If statement
class IfStmt : public Stmt {
private:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;  // Optional

public:
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch = nullptr)
        : cond(std::move(cond)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
    const Expr* getCond() const { return cond.get(); }
    const Stmt* getThen() const { return thenBranch.get(); }
    const Stmt* getElse() const { return elseBranch.get(); }
    void accept(ASTVisitor& visitor) const override;
};

// While statement
class WhileStmt : public Stmt {
private:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;

public:
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : cond(std::move(cond)), body(std::move(body)) {}
    const Expr* getCond() const { return cond.get(); }
    const Stmt* getBody() const { return body.get(); }
    void accept(ASTVisitor& visitor) const override;
};

// Block statement (compound)
class BlockStmt : public Stmt {
private:
    std::vector<std::unique_ptr<Stmt>> stmts;

public:
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) : stmts(std::move(stmts)) {}
    const std::vector<std::unique_ptr<Stmt>>& getStmts() const { return stmts; }
    void accept(ASTVisitor& visitor) const override;
};

// Function declaration
class Function {
private:
    std::string name;
    std::vector<std::string> params;  // Param names (all int)
    std::unique_ptr<BlockStmt> body;

public:
    Function(std::string name, std::vector<std::string> params, std::unique_ptr<BlockStmt> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
    const std::string& getName() const { return name; }
    const std::vector<std::string>& getParams() const { return params; }
    const BlockStmt* getBody() const { return body.get(); }
    void accept(ASTVisitor& visitor) const;
};

// Program (top-level)
class Program {
private:
    std::vector<std::unique_ptr<Function>> functions;

public:
    explicit Program(std::vector<std::unique_ptr<Function>> functions) : functions(std::move(functions)) {}
    const std::vector<std::unique_ptr<Function>>& getFunctions() const { return functions; }
    void accept(ASTVisitor& visitor) const;
};

// Visitor interface
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(const NumberExpr& expr) = 0;
    virtual void visit(const IdentExpr& expr) = 0;
    virtual void visit(const BinaryExpr& expr) = 0;
    virtual void visit(const CallExpr& expr) = 0;

    virtual void visit(const VarDeclStmt& stmt) = 0;
    virtual void visit(const AssignStmt& stmt) = 0;
    virtual void visit(const ReturnStmt& stmt) = 0;
    virtual void visit(const IfStmt& stmt) = 0;
    virtual void visit(const WhileStmt& stmt) = 0;
    virtual void visit(const BlockStmt& stmt) = 0;

    virtual void visit(const Function& func) = 0;
    virtual void visit(const Program& prog) = 0;
};