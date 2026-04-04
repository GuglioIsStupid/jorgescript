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
    std::unique_ptr<Statement> parseExprStatement();
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAddSubtract();
    std::unique_ptr<Expr> parseMultDivMod();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parsePostfix(std::unique_ptr<Expr> left);
    std::unique_ptr<Statement> parseIf();
    std::unique_ptr<Statement> parseLoadClib();
    std::unique_ptr<Statement> parseCallClib();
    std::unique_ptr<Statement> parseClibReturnType();
    std::unique_ptr<Statement> parseLocalSet();
    std::unique_ptr<Statement> parseSummon();
    std::unique_ptr<Statement> parseWhile();
    std::unique_ptr<Statement> parseFor();
    std::unique_ptr<Statement> parseFunctionDecl();
    std::unique_ptr<Statement> parseReturn();

};
