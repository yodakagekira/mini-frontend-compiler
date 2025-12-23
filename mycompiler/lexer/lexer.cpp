#include "lexer.hpp"

#include <cctype>
#include <string_view>

static inline unsigned char uch(char c) {
    return static_cast<unsigned char>(c);
}

Lexer::Lexer(std::string buffer)
    : buffer_(std::move(buffer)) {
    buffStart_ = buffer_.c_str();
    buffPtr_   = buffStart_;
}

Token Lexer::makeToken(TokenType type, const char* tokenEnd) {
    if (tokenEnd <= buffPtr_) {
        return Token(TokenType::Unknown, "error");
    }
    std::string text(buffPtr_, static_cast<size_t>(tokenEnd - buffPtr_));
    buffPtr_ = tokenEnd;
    return Token(type, std::move(text));
}

Token Lexer::peekToken() {
    const char* saved = buffPtr_;
    Token t = nextToken();
    buffPtr_ = saved;
    return t;
}

Token Lexer::nextToken() {
    // Skip whitespace
    while (*buffPtr_ && std::isspace(uch(*buffPtr_))) {
        ++buffPtr_;
    }

    if (*buffPtr_ == '\0') {
        return Token(TokenType::EoF, "");
    }

    // Identifiers / keywords
    if (std::isalpha(uch(*buffPtr_)) || *buffPtr_ == '_') {
        const char* start = buffPtr_;
        while (std::isalnum(uch(*buffPtr_)) || *buffPtr_ == '_') {
            ++buffPtr_;
        }
        std::string_view text(start, static_cast<size_t>(buffPtr_ - start));

        if (text == "with")   return Token(TokenType::kw_with,   std::string(text));
        if (text == "int")    return Token(TokenType::kw_int,    std::string(text));
        if (text == "if")     return Token(TokenType::kw_if,     std::string(text));
        if (text == "else")   return Token(TokenType::kw_else,   std::string(text));
        if (text == "while")  return Token(TokenType::kw_while,  std::string(text));
        if (text == "return") return Token(TokenType::kw_return, std::string(text));

        return Token(TokenType::ident, std::string(text));
    }

    // Numbers
    if (std::isdigit(uch(*buffPtr_))) {
        const char* start = buffPtr_;
        while (std::isdigit(uch(*buffPtr_))) {
            ++buffPtr_;
        }
        return Token(TokenType::number, std::string(start, static_cast<size_t>(buffPtr_ - start)));
    }

    // Two-char operators first
    if (*buffPtr_ == '=' && *(buffPtr_ + 1) == '=') {
        return makeToken(TokenType::eqeq, buffPtr_ + 2);
    }
    if (*buffPtr_ == '!' && *(buffPtr_ + 1) == '=') {
        return makeToken(TokenType::noteq, buffPtr_ + 2);
    }
    if (*buffPtr_ == '<' && *(buffPtr_ + 1) == '=') {
        return makeToken(TokenType::lte, buffPtr_ + 2);
    }
    if (*buffPtr_ == '>' && *(buffPtr_ + 1) == '=') {
        return makeToken(TokenType::gte, buffPtr_ + 2);
    }

    // Single-character tokens
    switch (*buffPtr_) {
        case '+': return makeToken(TokenType::plus, buffPtr_ + 1);
        case '-': return makeToken(TokenType::minus, buffPtr_ + 1);
        case '*': return makeToken(TokenType::star, buffPtr_ + 1);
        case '/': return makeToken(TokenType::slash, buffPtr_ + 1);

        case '<': return makeToken(TokenType::lt, buffPtr_ + 1);
        case '>': return makeToken(TokenType::gt, buffPtr_ + 1);

        case ',': return makeToken(TokenType::comma, buffPtr_ + 1);
        case ':': return makeToken(TokenType::colon, buffPtr_ + 1);
        case '=': return makeToken(TokenType::equal, buffPtr_ + 1);
        case ';': return makeToken(TokenType::semicolon, buffPtr_ + 1);
        case '(': return makeToken(TokenType::l_paren, buffPtr_ + 1);
        case ')': return makeToken(TokenType::r_paren, buffPtr_ + 1);
        case '{': return makeToken(TokenType::l_brace, buffPtr_ + 1);
        case '}': return makeToken(TokenType::r_brace, buffPtr_ + 1);

        default:  return makeToken(TokenType::Unknown, buffPtr_ + 1);
    }
}