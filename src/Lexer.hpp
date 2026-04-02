#pragma once
#include <string>

enum class TokenType {
    IF, THEN, OR,
    IS, ISNOT,

    SET, TO, ALWAYS,
    PLUS, MINUS, ASTERISK, SLASH, PERCENT, PRINT,
    AS, COMMA,
    LOADDLL_TOKEN, CALL_TOKEN,
    RETURNTYPE_TOKEN,
    AMPERSAND,

    TRUE, FALSE, NOTHING,
    NUMBER,
    STRING,
    IDENT,

    COLONCOLON,
    EQUAL, NOTEQUAL,
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    SEMICOLON,

    FOR, STEP, WHILE,

    INSIDE, SUMMON,

    END
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
public:
    explicit Lexer(const std::string& src);
    Token next();

    bool allowLowercase = false;

private:
    std::string src;
    size_t pos = 0;

    char peek() const;
    void skipWhitespace();
    Token identifier();
    Token number();
    Token string();

};
