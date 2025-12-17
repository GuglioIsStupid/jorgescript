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

    if(current.type == TokenType::LOADDLL_TOKEN)
        return parseLoadDll();

    if(current.type == TokenType::CALL_TOKEN)
        return parseCall();

    if (current.type == TokenType::INSIDE)
        return parseLocalSet();

    if (current.type == TokenType::SUMMON)
        return parseSummon();

    if (current.type == TokenType::WHILE)
        return parseWhile();

    if (current.type == TokenType::FOR)
        return parseFor();

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

    // literals
    if(current.type == TokenType::STRING) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::STRING;
        lit->value.string = current.value;
        left = std::move(lit);
        advance();
    } 
    else if(current.type == TokenType::NUMBER) {
        auto lit = std::make_unique<LiteralExpr>();
        std::string s = current.value;
        if (s.size() > 2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
            lit->value.type = ValueType::NUMBER;
            lit->value.number = std::stoll(s, nullptr, 16);
        } else {
            lit->value.type = ValueType::NUMBER;
            lit->value.number = std::stod(s);
        }
        left = std::move(lit);
        advance();
    } 
    else if(current.type == TokenType::TRUE || current.type == TokenType::FALSE) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::BOOLEAN;
        lit->value.boolean = (current.type == TokenType::TRUE);
        left = std::move(lit);
        advance();
    } 
    else if(current.type == TokenType::IDENT) {
        auto var = std::make_unique<VariableExpr>();
        var->name = current.value;
        left = std::move(var);
        advance();

        if(current.type == TokenType::COLONCOLON) {
            advance();

            if(current.type != TokenType::IDENT && current.type != TokenType::IS)
                throw std::runtime_error("Expected function name after ::");
            std::string funcName = current.value;
            advance();

            expect(TokenType::LPAREN);

            auto callExpr = std::make_unique<CallExpr>();
            callExpr->object = std::move(left);
            callExpr->function = funcName;

            if(current.type != TokenType::RPAREN) {
                callExpr->args.push_back(parseExpr());
                while(current.type == TokenType::COMMA) {
                    advance();
                    callExpr->args.push_back(parseExpr());
                }
            }

            expect(TokenType::RPAREN);
            left = std::move(callExpr);
        }
    }
    else if(current.type == TokenType::AMPERSAND) {
        advance();
        if(current.type != TokenType::IDENT)
            throw std::runtime_error("Expected identifier after &");
        auto var = std::make_unique<VariableExpr>();
        var->name = current.value;
        left = std::move(var);
        advance();
    }
    else if (current.type == TokenType::ASTERISK) {
        advance();
        if(current.type != TokenType::IDENT)
            throw std::runtime_error("Expected identifier after *");
        auto var = std::make_unique<VariableExpr>();
        var->name = current.value;
        left = std::move(var);
        advance();
    }
    else {
        throw std::runtime_error("Unexpected token in expression");
    }

    // handle + operator for strings/numbers
    while(current.type == TokenType::PLUS) {
        advance();
        auto right = parseExpr();
        auto bin = std::make_unique<BinaryExpr>();
        bin->left = std::move(left);
        bin->right = std::move(right);
        bin->op = '+';
        left = std::move(bin);
    }

    return left;
}

std::unique_ptr<Statement> Parser::parseSet() {
    bool isconstant = false;
    if(current.type == TokenType::ALWAYS){ isconstant=true; advance(); }

    expect(TokenType::SET);
    std::string name = current.value;
    expect(TokenType::IDENT);
    expect(TokenType::TO);

    auto stmt = std::make_unique<SetStatement>();
    stmt->name = name;
    stmt->isconstant = isconstant;
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

std::unique_ptr<Statement> Parser::parseLoadDll() {
    expect(TokenType::LOADDLL_TOKEN);

    std::string dll = current.value;
    expect(TokenType::STRING);

    expect(TokenType::AS);

    std::string alias = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::SEMICOLON);

    auto stmt = std::make_unique<LoadDllStatement>();
    stmt->dllName = dll;
    stmt->alias = alias;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseCall() {
    expect(TokenType::CALL_TOKEN);

    std::string alias = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::COLONCOLON);

    std::string func = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::LPAREN);

    auto stmt = std::make_unique<CallDllStatement>();
    stmt->alias = alias;
    stmt->function = func;

    if(current.type != TokenType::RPAREN) {
        stmt->args.push_back(parseExpr());
        while(current.type == TokenType::COMMA) {
            advance();
            stmt->args.push_back(parseExpr());
        }
    }

    expect(TokenType::RPAREN);
    expect(TokenType::SEMICOLON);

    return stmt;
}

std::unique_ptr<Statement> Parser::parseLocalSet() {
    expect(TokenType::INSIDE);

    bool isconstant = false;
    if (current.type == TokenType::ALWAYS) {
        isconstant = true;
        advance();
    }

    expect(TokenType::SET);

    std::string name = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::TO);

    auto stmt = std::make_unique<SetStatement>();
    stmt->name = name;
    stmt->isconstant = isconstant;
    stmt->isLocal = true;
    stmt->expr = parseExpr();

    expect(TokenType::SEMICOLON);
    return stmt;
}

std::unique_ptr<Statement> Parser::parseSummon() {
    expect(TokenType::SUMMON);

    std::string filename = current.value;
    expect(TokenType::STRING);

    std::string alias;
    if (current.type == TokenType::AS) {
        advance();
        alias = current.value;
        expect(TokenType::IDENT);
    }

    expect(TokenType::SEMICOLON);

    auto stmt = std::make_unique<SummonStatement>();
    stmt->filename = filename;
    stmt->alias = alias;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseWhile() {
    expect(TokenType::WHILE);

    auto condExpr = parseExpr();

    if (current.type == TokenType::THEN)
        advance();

    expect(TokenType::LBRACE);

    auto stmt = std::make_unique<WhileStatement>();
    stmt->condition = std::move(condExpr);

    while(current.type != TokenType::RBRACE)
        stmt->body.push_back(parseStatement());

    expect(TokenType::RBRACE);

    if(current.type == TokenType::SEMICOLON) advance();

    return stmt;
}

std::unique_ptr<Statement> Parser::parseFor() {
    expect(TokenType::FOR);

    std::string varName = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::EQUAL);

    auto start = parseExpr();

    expect(TokenType::TO);

    auto end = parseExpr();

    std::unique_ptr<Expr> step = nullptr;
    if(current.type == TokenType::STEP) {
        advance();
        step = parseExpr();
    }

    expect(TokenType::LBRACE);

    auto stmt = std::make_unique<ForStatement>();
    stmt->varName = varName;
    stmt->startExpr = std::move(start);
    stmt->endExpr = std::move(end);
    stmt->stepExpr = std::move(step);

    while(current.type != TokenType::RBRACE)
        stmt->body.push_back(parseStatement());

    expect(TokenType::RBRACE);

    if(current.type == TokenType::SEMICOLON) advance();

    return stmt;
}
