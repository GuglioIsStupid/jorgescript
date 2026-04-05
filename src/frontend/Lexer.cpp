#include "Lexer.hpp"
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {
std::string formatChar(char c) {
    switch (c) {
        case '\n': return "'\\n'";
        case '\r': return "'\\r'";
        case '\t': return "'\\t'";
        case '\0': return "'\\0'";
        default: return std::string("'") + c + "'";
    }
}

std::string formatLocation(const std::string& source, size_t absolutePos) {
    size_t line = 1;
    size_t column = 1;

    for (size_t i = 0; i < absolutePos && i < source.size(); ++i) {
        if (source[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }

    std::ostringstream out;
    out << "line " << line << ", column " << column;
    return out.str();
}

[[noreturn]] void throwLexerError(const std::string& source, size_t absolutePos, const std::string& message) {
    throw std::runtime_error("Lexer error at " + formatLocation(source, absolutePos) + ": " + message);
}
}

Lexer::Lexer(const std::string& src) : src(src) {}

Token Lexer::next() {
    skipWhitespace();

    if (pos >= src.size())
        return {TokenType::END, ""};

    char c = src[pos];

    if (std::isalpha(c))
        return identifier();

    if (std::isdigit(c))
        return number();

    if (c == '"')
        return string();

    if (c == ':' && peek() == ':') {
        pos += 2;
        allowLowercase = true;
        return {TokenType::COLONCOLON, "::"};
    }

    if (c == '/' && peek() == '/') {
        pos += 2;
        return {TokenType::FLOORDIV, "//"};
    }

    if (c == '=' && peek() == '=') {
        pos += 2;
        return {TokenType::EQEQ, "=="};
    }

    if (c == '!' && peek() == '=') {
        pos += 2;
        return {TokenType::NOTEQUAL, "!="};
    }

    if (c == '>' && peek() == '=') {
        pos += 2;
        return {TokenType::GREATEREQ, ">="};
    }

    if (c == '<' && peek() == '=') {
        pos += 2;
        return {TokenType::LESSEQ, "<="};
    }

    pos++;

    switch (c) {
        case '=': return {TokenType::EQUAL, "="};
        case '>': return {TokenType::GREATER, ">"};
        case '<': return {TokenType::LESS, "<"};
        case '[': return {TokenType::LBRACKET, "["};
        case ']': return {TokenType::RBRACKET, "]"};
        case '(': return {TokenType::LPAREN, "("};
        case ')': return {TokenType::RPAREN, ")"};
        case '{': return {TokenType::LBRACE, "{"};
        case '}': return {TokenType::RBRACE, "}"};
        case ';': return {TokenType::SEMICOLON, ";"};
        case '+': return {TokenType::PLUS, "+"};
        case '-': return {TokenType::MINUS, "-"};
        case '*': return {TokenType::ASTERISK, "*"};
        case '/': return {TokenType::SLASH, "/"};
        case '%': return {TokenType::PERCENT, "%"};
        case ',': return {TokenType::COMMA, ","};
        case '&': return {TokenType::AMPERSAND, "&"};
    }

    throwLexerError(src, pos - 1, "unknown character " + formatChar(c));
}

char Lexer::peek() const {
    return (pos + 1 < src.size()) ? src[pos + 1] : '\0';
}

void Lexer::skipWhitespace() {
    while (pos < src.size()) {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
            pos++;

        if (pos < src.size() && src[pos] == '#') {
            pos++;
            while (pos < src.size() && src[pos] != '\n' && src[pos] != '\r')
                pos++;
            continue;
        }

        break;
    }
}

Token Lexer::identifier() {
    size_t start = pos;
    while (pos < src.size() && (std::isalnum(src[pos]) || src[pos]=='.' || src[pos]=='_'))
        pos++;

    std::string word = src.substr(start, pos - start);

    bool bang = false;
    if (pos < src.size() && src[pos] == '!') {
        bang = true;
        pos++;
    }

    if (word == "TRUE") {
        if (!bang)
            throwLexerError(src, start, "TRUE must be written as TRUE!");
        return {TokenType::TRUE, "TRUE!"};
    }
    if (word == "Untrue...") {
        if (bang)
            throwLexerError(src, start, "unexpected '!' after Untrue...");
        return {TokenType::FALSE, "Untrue..."};
    }
    if (word == "NOTHING") {
        return {TokenType::NOTHING, "NOTHING"};
    }
    if (word == "POTENTIONABLY") {
        return {TokenType::POTENTIONABLY, "POTENTIONABLY"};
    }

    if (!allowLowercase)
        for (char ch : word)
            if (std::islower((unsigned char)ch))
                throwLexerError(src, start, "lowercase is not allowed here: '" + word + "'");

    allowLowercase = false;

    if (word == "IF") return {TokenType::IF, word};
    if (word == "THEN") return {TokenType::THEN, word};
    if (word == "OR") return {TokenType::OR, word};
    if (word == "IS") return {TokenType::IS, word};
    if (word == "ISNOT") return {TokenType::ISNOT, word};
    if (word == "FUNCTION") return {TokenType::FUNCTION_TOKEN, word};
    if (word == "RETURN") return {TokenType::RETURN_TOKEN, word};
    if (word == "SET") return {TokenType::SET, word};
    if (word == "TO") return {TokenType::TO, word};
    if (word == "ALWAYS") return {TokenType::ALWAYS, word};
    if (word == "PRINT") return {TokenType::PRINT, word};
    if (word == "AS") return {TokenType::AS, word};
    if (word == "LOADCLIB" || word == "LOADDLL") return {TokenType::LOADCLIB_TOKEN, word};
    if (word == "CALLCLIB" || word == "CALL") return {TokenType::CALLCLIB_TOKEN, word};
    if (word == "CLIBRETURNTYPE" || word == "RETURNTYPE") return {TokenType::CLIBRETURNTYPE_TOKEN, word};
    if (word == "INSIDE") return {TokenType::INSIDE, word};
    if (word == "SUMMON") return {TokenType::SUMMON, word};
    if (word == "FOR") return {TokenType::FOR, word};
    if (word == "STEP") return {TokenType::STEP, word};
    if (word == "WHILE") return {TokenType::WHILE, word};

    if (bang)
        throwLexerError(src, start, "unexpected '!' after identifier '" + word + "'");
    return {TokenType::IDENT, word};
}

Token Lexer::number() {
    size_t start = pos;
    if (src[pos] == '0' && (pos+1 < src.size()) && (src[pos+1] == 'x' || src[pos+1] == 'X')) {
        pos += 2;
        while (pos < src.size() && std::isxdigit(src[pos])) pos++;
        return {TokenType::NUMBER, src.substr(start, pos - start)};
    }
    while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.')) pos++;
    return {TokenType::NUMBER, src.substr(start, pos - start)};
}

Token Lexer::string() {
    pos++;
    size_t start = pos;
    while (pos < src.size() && src[pos] != '"')
        pos++;

    if (pos >= src.size()) {
        throwLexerError(src, start - 1, "unterminated string literal");
    }

    std::string value = src.substr(start, pos - start);
    pos++;
    return {TokenType::STRING, value};
}
