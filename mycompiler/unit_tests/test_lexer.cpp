#include "test_common.hpp"

#include "../lexer/lexer.hpp"   
#include "../lexer/token.hpp"

#include <vector>

static std::vector<Token> lexAll(const std::string& src) {
    // IMPORTANT: keep the std::string alive for the whole lexing run
    Lexer lex(src);
    std::vector<Token> out;
    while (true) {
        Token t = lex.nextToken();
        out.push_back(t);
        if (t.is(TokenType::EoF)) break;
        // prevent infinite loops if lexer gets stuck
        TEST_ASSERT(out.size() < 10'000);
    }
    return out;
}

static void test_keywords_and_punct() {
    const std::string src = "with int x = 42; return x + 1;";
    auto toks = lexAll(src);

    const std::vector<TokenType> expectTypes = {
        TokenType::kw_with,
        TokenType::kw_int,
        TokenType::ident,
        TokenType::equal,
        TokenType::number,
        TokenType::semicolon,
        TokenType::kw_return,
        TokenType::ident,
        TokenType::plus,
        TokenType::number,
        TokenType::semicolon,
        TokenType::EoF
    };

    TEST_ASSERT(toks.size() == expectTypes.size());
    for (size_t i = 0; i < expectTypes.size(); ++i) {
        TEST_ASSERT(toks[i].getType() == expectTypes[i]);
    }

    TEST_ASSERT(toks[2].getText() == "x");
    TEST_ASSERT(toks[4].getText() == "42");
    TEST_ASSERT(toks[7].getText() == "x");
    TEST_ASSERT(toks[9].getText() == "1");
}

static void test_braces_and_ops() {
    const std::string src = "if(x){x=x-1;}";
    auto toks = lexAll(src);

    const std::vector<TokenType> expectTypes = {
        TokenType::kw_if,
        TokenType::l_paren,
        TokenType::ident,
        TokenType::r_paren,
        TokenType::l_brace,
        TokenType::ident,
        TokenType::equal,
        TokenType::ident,
        TokenType::minus,
        TokenType::number,
        TokenType::semicolon,
        TokenType::r_brace,
        TokenType::EoF
    };

    TEST_ASSERT(toks.size() == expectTypes.size());
    for (size_t i = 0; i < expectTypes.size(); ++i) {
        TEST_ASSERT(toks[i].getType() == expectTypes[i]);
    }

    TEST_ASSERT(toks[2].getText() == "x");
    TEST_ASSERT(toks[9].getText() == "1");
}

int main() {
    int fails = 0;
    fails += run_test("lexer: keywords + punct", &test_keywords_and_punct);
    fails += run_test("lexer: braces + ops", &test_braces_and_ops);
    return fails;
}
