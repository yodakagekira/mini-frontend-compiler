#include "ast.hpp"
#include <iostream>

// Simple debug printer for the AST
class ASTPrinter : public ASTVisitor {
private:
    int indent = 0;

    void printIndent() const {
        for (int i = 0; i < indent; ++i)
            std::cout << "  ";
    }

public:
    void visit(const NumberExpr& expr) override {
        printIndent();
        std::cout << "Number(" << expr.getValue() << ")\n";
    }

    void visit(const IdentExpr& expr) override {
        printIndent();
        std::cout << "Ident(" << expr.getName() << ")\n";
    }

    void visit(const BinaryExpr& expr) override {
        printIndent();
        std::cout << "BinaryOp(";
        switch (expr.getOp()) {
            case TokenType::plus:  std::cout << "+"; break;
            case TokenType::minus: std::cout << "-"; break;
            case TokenType::star:  std::cout << "*"; break;
            case TokenType::slash: std::cout << "/"; break;
            default:               std::cout << "?"; break;
        }
        std::cout << ")\n";
        indent++;
        expr.getLHS()->accept(*this);
        expr.getRHS()->accept(*this);
        indent--;
    }

    void visit(const CallExpr& expr) override {
        printIndent();
        std::cout << "Call(" << expr.getCallee() << ")\n";
        indent++;
        for (const auto& arg : expr.getArgs()) {
            arg->accept(*this);
        }
        indent--;
    }

    void visit(const VarDeclStmt& stmt) override {
        printIndent();
        std::cout << "VarDecl(" << stmt.getName() << ")\n";
        if (stmt.getInit()) {
            indent++;
            stmt.getInit()->accept(*this);
            indent--;
        }
    }

    void visit(const AssignStmt& stmt) override {
        printIndent();
        std::cout << "Assign(" << stmt.getName() << ")\n";
        indent++;
        stmt.getValue()->accept(*this);
        indent--;
    }

    void visit(const ReturnStmt& stmt) override {
        printIndent();
        std::cout << "Return\n";
        if (stmt.getExpr()) {
            indent++;
            stmt.getExpr()->accept(*this);
            indent--;
        }
    }

    void visit(const IfStmt& stmt) override {
        printIndent();
        std::cout << "If\n";
        indent++;
        stmt.getCond()->accept(*this);
        stmt.getThen()->accept(*this);
        if (stmt.getElse()) {
            printIndent();
            std::cout << "Else\n";
            stmt.getElse()->accept(*this);
        }
        indent--;
    }

    void visit(const WhileStmt& stmt) override {
        printIndent();
        std::cout << "While\n";
        indent++;
        stmt.getCond()->accept(*this);
        stmt.getBody()->accept(*this);
        indent--;
    }

    void visit(const BlockStmt& stmt) override {
        printIndent();
        std::cout << "Block\n";
        indent++;
        for (const auto& s : stmt.getStmts()) {
            s->accept(*this);
        }
        indent--;
    }

    void visit(const Function& func) override {
        printIndent();
        std::cout << "Function(" << func.getName() << ")\n";
        indent++;
        std::cout << "Params: ";
        for (const auto& p : func.getParams()) {
            std::cout << p << " ";
        }
        std::cout << "\n";
        func.getBody()->accept(*this);
        indent--;
    }

    void visit(const Program& prog) override {
        printIndent();
        std::cout << "Program\n";
        indent++;
        for (const auto& f : prog.getFunctions()) {
            f->accept(*this);
        }
        indent--;
    }
};