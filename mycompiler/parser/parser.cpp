#include "../parser/parser.hpp"

#include <iostream>

Parser::Parser(Lexer& lex) : lexer(lex) {
    advance();
}

void Parser::advance() {
    currentToken = lexer.nextToken();
}

bool Parser::consume(TokenType expected) {
    if (currentToken.getType() != expected) {
        std::cerr << "Parser error: expected token " << static_cast<int>(expected)
                  << ", got '" << currentToken.getText() << "'\n";
        isError = true;

        // Panic-mode recovery: advance until we hit a sync token
        while (!currentToken.isAny({TokenType::semicolon, TokenType::r_brace, TokenType::EoF})) {
            advance();
        }
        // If we stopped on a sync token, consume it so we can continue.
        if (currentToken.isAny({TokenType::semicolon, TokenType::r_brace})) {
            advance();
        }
        return false;
    }
    advance();
    return true;
}

std::unique_ptr<Program> Parser::parseProgram() {
    std::vector<std::unique_ptr<Function>> functions;

    while (!currentToken.is(TokenType::EoF)) {
        auto func = parseFunction();
        if (!func) break;
        functions.push_back(std::move(func));
    }

    if (isError) return nullptr;
    return std::make_unique<Program>(std::move(functions));
}

// function → "int" ident "(" params? ")" block
std::unique_ptr<Function> Parser::parseFunction() {
    if (!consume(TokenType::kw_int)) return nullptr;

    if (!currentToken.is(TokenType::ident)) {
        std::cerr << "Expected function name\n";
        isError = true;
        return nullptr;
    }
    std::string name = currentToken.getText();
    advance();

    if (!consume(TokenType::l_paren)) return nullptr;

    std::vector<std::string> params;
    if (!currentToken.is(TokenType::r_paren)) {
        while (true) {
            if (currentToken.is(TokenType::EoF)) {
                std::cerr << "Unexpected EOF while parsing parameter list\n";
                isError = true;
                return nullptr;
            }

            if (!consume(TokenType::kw_int)) return nullptr;

            if (!currentToken.is(TokenType::ident)) {
                std::cerr << "Expected parameter name\n";
                isError = true;
                return nullptr;
            }
            params.push_back(currentToken.getText());
            advance();

            if (currentToken.is(TokenType::comma)) {
                advance();
                continue;
            }
            break;
        }
    }

    if (!consume(TokenType::r_paren)) return nullptr;

    auto body = parseBlock();
    if (!body) return nullptr;

    return std::make_unique<Function>(std::move(name), std::move(params), std::move(body));
}

// block → "{" stmt* "}"
std::unique_ptr<BlockStmt> Parser::parseBlock() {
    if (!consume(TokenType::l_brace)) return nullptr;

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!currentToken.isAny({TokenType::r_brace, TokenType::EoF})) {
        auto s = parseStmt();
        if (!s) break;
        stmts.push_back(std::move(s));
    }

    if (!consume(TokenType::r_brace)) return nullptr;
    return std::make_unique<BlockStmt>(std::move(stmts));
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (currentToken.isAny({TokenType::kw_with, TokenType::kw_int})) {
        return parseDeclStmt();
    }

    if (currentToken.is(TokenType::ident)) {
        // assignment: ident '=' ...
        Token next = lexer.peekToken();
        if (next.is(TokenType::equal)) {
            return parseAssignStmt();
        }

        std::cerr << "Unexpected identifier-starting statement (only assignments supported here): "
                  << currentToken.getText() << "\n";
        isError = true;
        return nullptr;
    }

    if (currentToken.is(TokenType::kw_return)) return parseReturnStmt();
    if (currentToken.is(TokenType::kw_if))     return parseIfStmt();
    if (currentToken.is(TokenType::kw_while))  return parseWhileStmt();
    if (currentToken.is(TokenType::l_brace))   return parseBlock();

    std::cerr << "Unexpected statement token: '" << currentToken.getText() << "'\n";
    isError = true;
    return nullptr;
}

// declStmt → ("with")? "int" ident "=" expr ";"
std::unique_ptr<Stmt> Parser::parseDeclStmt() {
    if (currentToken.is(TokenType::kw_with)) {
        advance();
    }

    if (!consume(TokenType::kw_int)) return nullptr;

    if (!currentToken.is(TokenType::ident)) {
        std::cerr << "Expected identifier after 'int'\n";
        isError = true;
        return nullptr;
    }
    std::string name = currentToken.getText();
    advance();

    if (!consume(TokenType::equal)) return nullptr;

    auto init = parseExpr();
    if (!init) return nullptr;

    if (!consume(TokenType::semicolon)) return nullptr;

    return std::make_unique<VarDeclStmt>(std::move(name), std::move(init));
}

// assignStmt → ident '=' expr ';'
std::unique_ptr<Stmt> Parser::parseAssignStmt() {
    if (!currentToken.is(TokenType::ident)) {
        std::cerr << "Internal parse error: expected identifier at start of assignment\n";
        isError = true;
        return nullptr;
    }

    std::string name = currentToken.getText();
    advance();

    if (!consume(TokenType::equal)) return nullptr;

    auto value = parseExpr();
    if (!value) return nullptr;

    if (!consume(TokenType::semicolon)) return nullptr;

    return std::make_unique<AssignStmt>(std::move(name), std::move(value));
}

// returnStmt → 'return' expr ';'
std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    advance();
    auto e = parseExpr();
    if (!e) return nullptr;
    if (!consume(TokenType::semicolon)) return nullptr;
    return std::make_unique<ReturnStmt>(std::move(e));
}

// ifStmt → 'if' '(' expr ')' stmt ('else' stmt)?
std::unique_ptr<Stmt> Parser::parseIfStmt() {
    advance();
    if (!consume(TokenType::l_paren)) return nullptr;
    auto cond = parseExpr();
    if (!cond) return nullptr;
    if (!consume(TokenType::r_paren)) return nullptr;

    auto thenBranch = parseStmt();
    if (!thenBranch) return nullptr;

    std::unique_ptr<Stmt> elseBranch;
    if (currentToken.is(TokenType::kw_else)) {
        advance();
        elseBranch = parseStmt();
        if (!elseBranch) return nullptr;
    }

    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

// whileStmt → 'while' '(' expr ')' stmt
std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    advance();
    if (!consume(TokenType::l_paren)) return nullptr;
    auto cond = parseExpr();
    if (!cond) return nullptr;
    if (!consume(TokenType::r_paren)) return nullptr;

    auto body = parseStmt();
    if (!body) return nullptr;

    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

// ===== Expressions (precedence climbing) =====

std::unique_ptr<Expr> Parser::parseExpr() {
    return parseEquality();
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseComparison();
    while (currentToken.isAny({TokenType::eqeq, TokenType::noteq})) {
        TokenType op = currentToken.getType();
        advance();
        auto right = parseComparison();
        if (!right) return nullptr;
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseAddSub();
    while (currentToken.isAny({TokenType::lt, TokenType::lte, TokenType::gt, TokenType::gte})) {
        TokenType op = currentToken.getType();
        advance();
        auto right = parseAddSub();
        if (!right) return nullptr;
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseAddSub() {
    auto left = parseMulDiv();
    while (currentToken.isAny({TokenType::plus, TokenType::minus})) {
        TokenType op = currentToken.getType();
        advance();
        auto right = parseMulDiv();
        if (!right) return nullptr;
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseMulDiv() {
    auto left = parsePrimary();
    while (currentToken.isAny({TokenType::star, TokenType::slash})) {
        TokenType op = currentToken.getType();
        advance();
        auto right = parsePrimary();
        if (!right) return nullptr;
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (currentToken.is(TokenType::number)) {
        int value = std::stoi(currentToken.getText());
        advance();
        return std::make_unique<NumberExpr>(value);
    }

    if (currentToken.is(TokenType::ident)) {
        std::string name = currentToken.getText();
        advance();

        // call → ident '(' args? ')'
        if (currentToken.is(TokenType::l_paren)) {
            advance();

            std::vector<std::unique_ptr<Expr>> args;
            if (!currentToken.is(TokenType::r_paren)) {
                while (true) {
                    if (currentToken.is(TokenType::EoF)) {
                        std::cerr << "Unexpected EOF while parsing call arguments\n";
                        isError = true;
                        return nullptr;
                    }

                    auto arg = parseExpr();
                    if (!arg) return nullptr;
                    args.push_back(std::move(arg));

                    if (currentToken.is(TokenType::comma)) {
                        advance();
                        continue;
                    }
                    break;
                }
            }

            if (!consume(TokenType::r_paren)) return nullptr;
            return std::make_unique<CallExpr>(std::move(name), std::move(args));
        }

        return std::make_unique<IdentExpr>(std::move(name));
    }

    if (currentToken.is(TokenType::l_paren)) {
        advance();
        auto e = parseExpr();
        if (!e) return nullptr;
        if (!consume(TokenType::r_paren)) return nullptr;
        return e;
    }

    std::cerr << "Unexpected token in expression: '" << currentToken.getText() << "'\n";
    isError = true;
    return nullptr;
}