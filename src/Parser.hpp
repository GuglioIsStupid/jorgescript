#pragma once
#include "Lexer.hpp"
#include "AST.hpp"
#include <memory>
#include <vector>

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::vector<std::unique_ptr<Statement>> parseProgram();

private:
    Lexer& lexer;
    Token current;

    void advance();
    void expect(TokenType type);

    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseSet();
    std::unique_ptr<Statement> parsePrint();
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Statement> parseIf();

};
