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
    while (pos < src.size() && (std::isalnum(src[pos]) || src[pos]=='.'))
        pos++;

    std::string word = src.substr(start, pos - start);

    bool bang = false;
    if (pos < src.size() && src[pos] == '!') {
        bang = true;
        pos++;
    }

    std::cout << "Identifier found: " << word << (bang ? "!" : "") << "\n";
    if (word == "Untrue...") {
        return {TokenType::FALSE, "Untrue..."};
    }

    for(char ch : word)
        if(std::islower(ch))
            throw std::runtime_error("Lowercase not allowed");

    if (word == "IF") return {TokenType::IF, word};
    if (word == "THEN") return {TokenType::THEN, word};
    if (word == "OR") return {TokenType::OR, word};
    if (word == "IS") return {TokenType::IS, word};
    if (word == "ISNOT") return {TokenType::ISNOT, word};
    if (word == "SET") return {TokenType::SET, word};
    if (word == "TO") return {TokenType::TO, word};
    if (word == "ALWAYS") return {TokenType::ALWAYS, word};
    if (word == "PRINT") return {TokenType::PRINT, word};

    if (word == "TRUE") {
        if (!bang) throw std::runtime_error("TRUE must be TRUE!");
        return {TokenType::TRUE, "TRUE!"};
    }

    if (bang)
        throw std::runtime_error("Unexpected !");

    return {TokenType::IDENT, word};
}

Token Lexer::number() {
    size_t start = pos;
    while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.'))
        pos++;
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
