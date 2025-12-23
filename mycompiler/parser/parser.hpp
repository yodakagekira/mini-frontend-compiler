#pragma once

#include "../lexer/lexer.hpp"
#include "../ast/ast.hpp"

#include <memory>

class Parser {
private:
    Lexer& lexer;
    Token currentToken;
    bool isError{false};

    void advance();
    bool consume(TokenType expected);

    // Expression parsing (precedence levels)
    std::unique_ptr<Expr> parseExpr();        // entry
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAddSub();
    std::unique_ptr<Expr> parseMulDiv();
    std::unique_ptr<Expr> parsePrimary();

    // Statement parsing
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Stmt> parseDeclStmt();
    std::unique_ptr<Stmt> parseAssignStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<BlockStmt> parseBlock();

    // Function and program
    std::unique_ptr<Function> parseFunction();

public:
    explicit Parser(Lexer& lex);
    bool hasError() const { return isError; }
    std::unique_ptr<Program> parseProgram();
};