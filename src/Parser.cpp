#include "Parser.hpp"
#include <stdexcept>
#include <memory>

Parser::Parser(Lexer& lexer) : lexer(lexer) {
    advance();
}

void Parser::advance() {
    current = lexer.next();
}

void Parser::expect(TokenType type) {
    if (current.type != type)
        throw std::runtime_error("Unexpected token");
    advance();
}

std::vector<std::unique_ptr<Statement>> Parser::parseProgram() {
    std::vector<std::unique_ptr<Statement>> stmts;
    while (current.type != TokenType::END)
        stmts.push_back(parseStatement());
    return stmts;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if(current.type == TokenType::SET || current.type == TokenType::ALWAYS)
        return parseSet();

    if(current.type == TokenType::PRINT)
        return parsePrint();

    if(current.type == TokenType::IF)
        return parseIf();

    throw std::runtime_error("Unknown statement");
}

std::unique_ptr<Statement> Parser::parseIf() {
    expect(TokenType::IF);

    std::string ident = current.value;
    expect(TokenType::IDENT);
    expect(TokenType::COLONCOLON);
    expect(TokenType::IS);
    expect(TokenType::LPAREN);

    auto condExpr = parseExpr();

    expect(TokenType::RPAREN);
    expect(TokenType::THEN);
    expect(TokenType::LBRACE);

    auto stmt = std::make_unique<IfStatement>();
    
    auto varExpr = std::make_unique<VariableExpr>();
    varExpr->name = ident;

    auto bin = std::make_unique<BinaryExpr>();
    bin->left = std::move(varExpr);
    bin->right = std::move(condExpr);
    bin->op = '=';
    stmt->condition = std::move(bin);

    while(current.type != TokenType::RBRACE)
        stmt->body.push_back(parseStatement());

    expect(TokenType::RBRACE);

    if(current.type == TokenType::SEMICOLON) advance();

    return stmt;
}

std::unique_ptr<Expr> Parser::parseExpr() {
    std::unique_ptr<Expr> left;

    if(current.type == TokenType::STRING) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::STRING;
        lit->value.string = current.value;
        left = std::move(lit);
        advance();
    } else if(current.type == TokenType::NUMBER) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::NUMBER;
        lit->value.number = std::stod(current.value);
        left = std::move(lit);
        advance();
    } else if(current.type == TokenType::TRUE || current.type == TokenType::FALSE) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::BOOLEAN;
        lit->value.boolean = (current.type == TokenType::TRUE);
        left = std::move(lit);
        advance();
    } else if(current.type == TokenType::IDENT) {
        auto var = std::make_unique<VariableExpr>();
        var->name = current.value;
        left = std::move(var);
        advance();
    } 
    else {
        throw std::runtime_error("Unexpected token in expression");
    }

    while(current.type == TokenType::PLUS) {
        advance();
        auto right = parseExpr();
        /* left = std::make_unique<BinaryExpr>(BinaryExpr{std::move(left), std::move(right), '+'}); */
        auto bin = std::make_unique<BinaryExpr>();
        bin->left = std::move(left);
        bin->right = std::move(right);
        bin->op = '+';
        left = std::move(bin);
    }

    return left;
}

std::unique_ptr<Statement> Parser::parseSet() {
    bool constant = false;
    if(current.type == TokenType::ALWAYS){ constant=true; advance(); }

    expect(TokenType::SET);
    std::string name = current.value;
    expect(TokenType::IDENT);
    expect(TokenType::TO);

    auto stmt = std::make_unique<SetStatement>();
    stmt->name = name;
    stmt->constant = constant;
    stmt->expr = parseExpr();

    expect(TokenType::SEMICOLON);
    return stmt;
}

std::unique_ptr<Statement> Parser::parsePrint() {
    expect(TokenType::PRINT);

    auto stmt = std::make_unique<PrintStatement>();
    stmt->expr = parseExpr();

    expect(TokenType::SEMICOLON);
    return stmt;
}
