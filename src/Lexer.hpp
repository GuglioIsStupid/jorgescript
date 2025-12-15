#pragma once
#include <string>

enum class TokenType {
    IF, THEN, OR,
    IS, ISNOT,

    SET, TO, ALWAYS,
    PLUS, PRINT,

    TRUE, FALSE,
    NUMBER,
    STRING,
    IDENT,

    COLONCOLON,
    EQUAL,
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    SEMICOLON,

    END,
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
public:
    explicit Lexer(const std::string& src);
    Token next();

private:
    std::string src;
    size_t pos = 0;

    char peek() const;
    void skipWhitespace();
    Token identifier();
    Token number();
    Token string();
};
