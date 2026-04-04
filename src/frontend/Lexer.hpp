#pragma once
#include <string>

enum class TokenType {
    IF, THEN, OR,
    IS, ISNOT,
    FUNCTION_TOKEN, RETURN_TOKEN,

    SET, TO, ALWAYS,
    PLUS, MINUS, ASTERISK, SLASH, FLOORDIV, PERCENT, PRINT,
    AS, COMMA,
    LOADCLIB_TOKEN, CALLCLIB_TOKEN,
    CLIBRETURNTYPE_TOKEN,
    AMPERSAND,

    TRUE, FALSE, NOTHING, POTENTIONABLY,
    NUMBER,
    STRING,
    IDENT,

    COLONCOLON,
    EQUAL, EQEQ, NOTEQUAL,
    GREATER, LESS, GREATEREQ, LESSEQ,
    LBRACKET, RBRACKET,
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
