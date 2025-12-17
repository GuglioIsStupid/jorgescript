#pragma once
#include <memory>
#include <vector>

#include "AST.hpp"
#include "Lexer.hpp"

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
    std::unique_ptr<Statement> parseLoadDll();
    std::unique_ptr<Statement> parseCall();
    std::unique_ptr<Statement> parseLocalSet();
    std::unique_ptr<Statement> parseSummon();
    std::unique_ptr<Statement> parseWhile();
    std::unique_ptr<Statement> parseFor();

};
