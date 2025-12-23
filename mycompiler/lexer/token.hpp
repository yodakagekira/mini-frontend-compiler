#pragma once

#include <initializer_list>
#include <string>

enum class TokenType {
    ident,
    number,

    // arithmetic
    plus,
    minus,
    star,
    slash,

    // comparison
    lt,
    lte,
    gt,
    gte,
    eqeq,
    noteq,

    // punctuation 
    comma,
    colon,
    equal,      
    semicolon,

    l_paren,
    r_paren,
    l_brace,
    r_brace,

    // keywords
    kw_int,
    kw_if,
    kw_else,
    kw_while,
    kw_return,
    kw_with,

    Unknown,
    EoF
};

struct Token {
private:
    TokenType type{TokenType::Unknown};
    std::string text;

public:
    Token() = default;
    Token(TokenType t, std::string s) : type(t), text(std::move(s)) {}

    TokenType getType() const { return type; }
    const std::string& getText() const { return text; }

    bool is(TokenType t) const { return type == t; }
    bool isAny(std::initializer_list<TokenType> types) const {
        for (auto t : types) {
            if (type == t) return true;
        }
        return false;
    }
};