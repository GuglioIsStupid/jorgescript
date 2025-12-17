#include "Lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <iostream>

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

    pos++;

    switch (c) {
        case '=': return {TokenType::EQUAL, "="};
        case '(': return {TokenType::LPAREN, "("};
        case ')': return {TokenType::RPAREN, ")"};
        case '{': return {TokenType::LBRACE, "{"};
        case '}': return {TokenType::RBRACE, "}"};
        case ';': return {TokenType::SEMICOLON, ";"};
        case '+': return {TokenType::PLUS, "+"};
        case ',': return {TokenType::COMMA, ","};
        case '&': return {TokenType::AMPERSAND, "&"};
        case '*': return {TokenType::ASTERISK, "*"};
    }

    throw std::runtime_error("Unknown character");
}

char Lexer::peek() const {
    return (pos + 1 < src.size()) ? src[pos + 1] : '\0';
}

void Lexer::skipWhitespace() {
    while (pos < src.size() && std::isspace(src[pos]))
        pos++;
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

    std::cout << "Identifier found: " << word << (bang ? "!" : "") << "\n";

    if (word == "TRUE") {
        if (!bang)
            throw std::runtime_error("TRUE must be TRUE!");
        return {TokenType::TRUE, "TRUE!"};
    }
    if (word == "Untrue...") {
        if (bang)
            throw std::runtime_error("Unexpected !");
        return {TokenType::FALSE, "Untrue..."};
    }
    if (word == "NOTHING") {
        return {TokenType::NOTHING, "NOTHING"};
    }

    if (!allowLowercase)
        for (char ch : word)
            if (std::islower((unsigned char)ch))
                throw std::runtime_error("Lowercase not allowed");

    allowLowercase = false;

    if (word == "IF") return {TokenType::IF, word};
    if (word == "THEN") return {TokenType::THEN, word};
    if (word == "OR") return {TokenType::OR, word};
    if (word == "IS") return {TokenType::IS, word};
    if (word == "ISNOT") return {TokenType::ISNOT, word};
    if (word == "SET") return {TokenType::SET, word};
    if (word == "TO") return {TokenType::TO, word};
    if (word == "ALWAYS") return {TokenType::ALWAYS, word};
    if (word == "PRINT") return {TokenType::PRINT, word};
    if (word == "AS") return {TokenType::AS, word};
    if (word == "LOADDLL") return {TokenType::LOADDLL_TOKEN, word};
    if (word == "CALL") return {TokenType::CALL_TOKEN, word};
    if (word == "INSIDE") return {TokenType::INSIDE, word};
    if (word == "SUMMON") return {TokenType::SUMMON, word};
    if (word == "FOR") return {TokenType::FOR, word};
    if (word == "STEP") return {TokenType::STEP, word};
    if (word == "WHILE") return {TokenType::WHILE, word};

    if (bang)
        throw std::runtime_error("Unexpected !");
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
    std::string value = src.substr(start, pos - start);
    pos++;
    return {TokenType::STRING, value};
}
