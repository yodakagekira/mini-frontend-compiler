#pragma once

#include "token.hpp"
#include <string>

class Lexer {
public:
    explicit Lexer(std::string buffer);

    Token nextToken();
    Token peekToken();

private:
    std::string buffer_;
    const char* buffStart_{nullptr};
    const char* buffPtr_{nullptr};

    Token makeToken(TokenType type, const char* tokenEnd);
};