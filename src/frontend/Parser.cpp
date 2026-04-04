#include "Parser.hpp"
#include <stdexcept>
#include <memory>

namespace {
std::string joinNamespace(const std::vector<std::string>& segments, size_t endExclusive) {
    std::string result;
    for (size_t i = 0; i < endExclusive; ++i) {
        if (i > 0) result += "::";
        result += segments[i];
    }
    return result;
}
}

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
    if(current.type == TokenType::FUNCTION_TOKEN)
        return parseFunctionDecl();

    if(current.type == TokenType::RETURN_TOKEN)
        return parseReturn();

    if(current.type == TokenType::SET || current.type == TokenType::ALWAYS)
        return parseSet();

    if(current.type == TokenType::PRINT)
        return parsePrint();

    if(current.type == TokenType::IF)
        return parseIf();

    if(current.type == TokenType::LOADCLIB_TOKEN)
        return parseLoadClib();

    if(current.type == TokenType::CALLCLIB_TOKEN)
        return parseCallClib();

    if(current.type == TokenType::CLIBRETURNTYPE_TOKEN)
        return parseClibReturnType();

    if (current.type == TokenType::INSIDE)
        return parseLocalSet();

    if (current.type == TokenType::SUMMON)
        return parseSummon();

    if (current.type == TokenType::WHILE)
        return parseWhile();

    if (current.type == TokenType::FOR)
        return parseFor();

    if (current.type == TokenType::IDENT)
        return parseExprStatement();

    throw std::runtime_error("Unknown statement");
}

std::unique_ptr<Statement> Parser::parseExprStatement() {
    auto stmt = std::make_unique<ExprStatement>();
    stmt->expr = parseExpr();
    expect(TokenType::SEMICOLON);
    return stmt;
}

std::unique_ptr<Statement> Parser::parseIf() {
    expect(TokenType::IF);

    std::unique_ptr<Expr> conditionExpr;

    if (current.type == TokenType::LPAREN) {
        advance();
        conditionExpr = parseExpr();
        expect(TokenType::RPAREN);
    } else if (current.type == TokenType::IDENT) {
        std::string ident = current.value;
        advance();

        if (current.type == TokenType::COLONCOLON) {
            advance();
            expect(TokenType::IS);
            expect(TokenType::LPAREN);

            auto condExpr = parseExpr();

            expect(TokenType::RPAREN);

            auto varExpr = std::make_unique<VariableExpr>();
            varExpr->name = ident;

            auto bin = std::make_unique<BinaryExpr>();
            bin->left = std::move(varExpr);
            bin->right = std::move(condExpr);
            bin->op = "==";
            conditionExpr = std::move(bin);
        } else {
            std::unique_ptr<Expr> left = std::make_unique<VariableExpr>();
            static_cast<VariableExpr*>(left.get())->name = ident;

            while (current.type == TokenType::EQEQ || current.type == TokenType::NOTEQUAL ||
                   current.type == TokenType::GREATER || current.type == TokenType::LESS ||
                   current.type == TokenType::GREATEREQ || current.type == TokenType::LESSEQ) {
                std::string op;
                if (current.type == TokenType::EQEQ) op = "==";
                else if (current.type == TokenType::NOTEQUAL) op = "!=";
                else if (current.type == TokenType::GREATER) op = ">";
                else if (current.type == TokenType::LESS) op = "<";
                else if (current.type == TokenType::GREATEREQ) op = ">=";
                else if (current.type == TokenType::LESSEQ) op = "<=";

                advance();
                auto right = parseAddSubtract();

                auto bin = std::make_unique<BinaryExpr>();
                bin->left = std::move(left);
                bin->right = std::move(right);
                bin->op = op;
                left = std::move(bin);
            }

            conditionExpr = std::move(left);
        }
    } else {
        conditionExpr = parseExpr();
    }

    expect(TokenType::THEN);
    expect(TokenType::LBRACE);

    auto stmt = std::make_unique<IfStatement>();
    stmt->condition = std::move(conditionExpr);

    while(current.type != TokenType::RBRACE)
        stmt->body.push_back(parseStatement());

    expect(TokenType::RBRACE);

    if(current.type == TokenType::SEMICOLON) advance();

    return stmt;
}

std::unique_ptr<Expr> Parser::parseExpr() {
    return parseComparison();
}

std::unique_ptr<Expr> Parser::parseComparison() {
    std::unique_ptr<Expr> left = parseAddSubtract();

    while (current.type == TokenType::EQEQ || current.type == TokenType::NOTEQUAL ||
           current.type == TokenType::GREATER || current.type == TokenType::LESS ||
           current.type == TokenType::GREATEREQ || current.type == TokenType::LESSEQ) {
        std::string op;
        if (current.type == TokenType::EQEQ) op = "==";
        else if (current.type == TokenType::NOTEQUAL) op = "!=";
        else if (current.type == TokenType::GREATER) op = ">";
        else if (current.type == TokenType::LESS) op = "<";
        else if (current.type == TokenType::GREATEREQ) op = ">=";
        else if (current.type == TokenType::LESSEQ) op = "<=";

        advance();
        auto right = parseAddSubtract();
        auto bin = std::make_unique<BinaryExpr>();
        bin->left = std::move(left);
        bin->right = std::move(right);
        bin->op = op;
        left = std::move(bin);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseAddSubtract() {
    std::unique_ptr<Expr> left = parseMultDivMod();

    while(current.type == TokenType::PLUS || current.type == TokenType::MINUS) {
        std::string op = (current.type == TokenType::PLUS) ? "+" : "-";
        advance();
        auto right = parseMultDivMod();
        auto bin = std::make_unique<BinaryExpr>();
        bin->left = std::move(left);
        bin->right = std::move(right);
        bin->op = op;
        left = std::move(bin);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseMultDivMod() {
    std::unique_ptr<Expr> left = parsePrimary();

    while(current.type == TokenType::ASTERISK || current.type == TokenType::SLASH || current.type == TokenType::FLOORDIV || current.type == TokenType::PERCENT) {
        std::string op = (current.type == TokenType::ASTERISK) ? "*" : 
                         (current.type == TokenType::SLASH) ? "/" :
                         (current.type == TokenType::FLOORDIV) ? "//" : "%";
        advance();
        auto right = parsePrimary();
        auto bin = std::make_unique<BinaryExpr>();
        bin->left = std::move(left);
        bin->right = std::move(right);
        bin->op = op;
        left = std::move(bin);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    std::unique_ptr<Expr> left;

    // literals
    if(current.type == TokenType::LPAREN) {
        advance();
        left = parseExpr();
        expect(TokenType::RPAREN);
    }
    else if(current.type == TokenType::STRING) {
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
    else if(current.type == TokenType::NOTHING) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value = Value();
        left = std::move(lit);
        advance();
    }
    else if(current.type == TokenType::POTENTIONABLY) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::POTENTIAL_BOOLEAN;
        left = std::move(lit);
        advance();
    }
    else if(current.type == TokenType::LBRACKET) {
        advance();

        auto arr = std::make_unique<ArrayLiteralExpr>();
        if (current.type != TokenType::RBRACKET) {
            arr->elements.push_back(parseExpr());
            while (current.type == TokenType::COMMA) {
                advance();
                arr->elements.push_back(parseExpr());
            }
        }

        expect(TokenType::RBRACKET);
        left = std::move(arr);
    }
    else if(current.type == TokenType::IDENT) {
        const std::string firstIdent = current.value;
        auto var = std::make_unique<VariableExpr>();
        var->name = firstIdent;
        left = std::move(var);
        advance();

        if(current.type == TokenType::COLONCOLON) {
            std::vector<std::string> segments;
            segments.push_back(firstIdent);

            while (current.type == TokenType::COLONCOLON) {
                advance();
                if(current.type != TokenType::IDENT && current.type != TokenType::IS)
                    throw std::runtime_error("Expected identifier after ::");
                segments.push_back(current.value);
                advance();
            }

            if (segments.size() < 2)
                throw std::runtime_error("Expected function name after namespace");

            expect(TokenType::LPAREN);

            auto callExpr = std::make_unique<CallExpr>();
            callExpr->namespaceName = joinNamespace(segments, segments.size() - 1);
            callExpr->function = segments.back();

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
    else if (current.type == TokenType::MINUS) {
        advance();
        auto expr = parsePrimary();
        auto lit = std::make_unique<LiteralExpr>();
        lit->value.type = ValueType::NUMBER;
        lit->value.number = 0;
        auto bin = std::make_unique<BinaryExpr>();
        bin->left = std::move(lit);
        bin->right = std::move(expr);
        bin->op = "-";
        left = std::move(bin);
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

    return parsePostfix(std::move(left));
}

std::unique_ptr<Expr> Parser::parsePostfix(std::unique_ptr<Expr> left) {
    while (current.type == TokenType::LBRACKET) {
        advance();

        auto idx = parseExpr();
        expect(TokenType::RBRACKET);

        auto access = std::make_unique<IndexExpr>();
        access->target = std::move(left);
        access->index = std::move(idx);
        left = std::move(access);
    }

    return left;
}

std::unique_ptr<Statement> Parser::parseSet() {
    bool isconstant = false;
    if(current.type == TokenType::ALWAYS){ isconstant=true; advance(); }

    expect(TokenType::SET);
    std::string name = current.value;
    expect(TokenType::IDENT);

    std::unique_ptr<Expr> indexExpr;
    if (current.type == TokenType::LBRACKET) {
        advance();
        indexExpr = parseExpr();
        expect(TokenType::RBRACKET);
    }

    expect(TokenType::TO);

    auto stmt = std::make_unique<SetStatement>();
    stmt->name = name;
    stmt->isconstant = isconstant;
    stmt->indexExpr = std::move(indexExpr);
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

std::unique_ptr<Statement> Parser::parseLoadClib() {
    expect(TokenType::LOADCLIB_TOKEN);

    std::string clib = current.value;
    expect(TokenType::STRING);

    expect(TokenType::AS);

    std::string alias = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::SEMICOLON);

    auto stmt = std::make_unique<LoadClibStatement>();
    stmt->clibName = clib;
    stmt->alias = alias;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseCallClib() {
    expect(TokenType::CALLCLIB_TOKEN);

    std::string alias = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::COLONCOLON);

    std::string func = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::LPAREN);

    auto stmt = std::make_unique<CallClibStatement>();
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

std::unique_ptr<Statement> Parser::parseClibReturnType() {
    expect(TokenType::CLIBRETURNTYPE_TOKEN);

    std::string alias = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::COLONCOLON);

    std::string func = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::AS);

    std::string returnType = current.value;
    if (current.type != TokenType::IDENT && current.type != TokenType::STRING)
        throw std::runtime_error("Expected type name after AS");
    advance();

    expect(TokenType::SEMICOLON);

    auto stmt = std::make_unique<SetClibReturnTypeStatement>();
    stmt->alias = alias;
    stmt->function = func;
    stmt->returnType = returnType;
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

    std::unique_ptr<Expr> condExpr;
    if (current.type == TokenType::LPAREN) {
        advance();
        condExpr = parseExpr();
        expect(TokenType::RPAREN);
    } else {
        condExpr = parseExpr();
    }

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

std::unique_ptr<Statement> Parser::parseFunctionDecl() {
    expect(TokenType::FUNCTION_TOKEN);

    std::string name = current.value;
    expect(TokenType::IDENT);

    expect(TokenType::LPAREN);

    std::vector<std::string> params;
    if (current.type != TokenType::RPAREN) {
        params.push_back(current.value);
        expect(TokenType::IDENT);

        while (current.type == TokenType::COMMA) {
            advance();
            params.push_back(current.value);
            expect(TokenType::IDENT);
        }
    }

    expect(TokenType::RPAREN);
    expect(TokenType::LBRACE);

    auto stmt = std::make_unique<FunctionDeclStatement>();
    stmt->name = name;
    stmt->params = std::move(params);

    while(current.type != TokenType::RBRACE)
        stmt->body.push_back(parseStatement());

    expect(TokenType::RBRACE);

    if(current.type == TokenType::SEMICOLON) advance();

    return stmt;
}

std::unique_ptr<Statement> Parser::parseReturn() {
    expect(TokenType::RETURN_TOKEN);

    auto stmt = std::make_unique<ReturnStatement>();
    stmt->expr = parseExpr();

    expect(TokenType::SEMICOLON);
    return stmt;
}
