#include "Parser.hpp"
#include <stdexcept>
#include <memory>

namespace {
std::string tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::OR: return "OR";
        case TokenType::IS: return "IS";
        case TokenType::ISNOT: return "ISNOT";
        case TokenType::FUNCTION_TOKEN: return "FUNCTION";
        case TokenType::RETURN_TOKEN: return "RETURN";
        case TokenType::CLASS_TOKEN: return "CLASS";
        case TokenType::OPERATOR_TOKEN: return "OPERATOR";
        case TokenType::INHERITS_TOKEN: return "INHERITS";
        case TokenType::IMPLEMENTS_TOKEN: return "IMPLEMENTS";
        case TokenType::STRUCT_TOKEN: return "STRUCT";
        case TokenType::SET: return "SET";
        case TokenType::TO: return "TO";
        case TokenType::ALWAYS: return "ALWAYS";
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::ASTERISK: return "*";
        case TokenType::SLASH: return "/";
        case TokenType::FLOORDIV: return "//";
        case TokenType::PERCENT: return "%";
        case TokenType::PRINT: return "PRINT";
        case TokenType::AS: return "AS";
        case TokenType::COMMA: return ",";
        case TokenType::LOADCLIB_TOKEN: return "LOADCLIB";
        case TokenType::CALLCLIB_TOKEN: return "CALLCLIB";
        case TokenType::CLIBRETURNTYPE_TOKEN: return "CLIBRETURNTYPE";
        case TokenType::AMPERSAND: return "&";
        case TokenType::TRUE: return "TRUE!";
        case TokenType::FALSE: return "Untrue...";
        case TokenType::NOTHING: return "NOTHING";
        case TokenType::POTENTIONABLY: return "POTENTIONABLY";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENT: return "IDENT";
        case TokenType::COLONCOLON: return "::";
        case TokenType::COLON: return ":";
        case TokenType::EQUAL: return "=";
        case TokenType::EQEQ: return "==";
        case TokenType::NOTEQUAL: return "!=";
        case TokenType::GREATER: return ">";
        case TokenType::LESS: return "<";
        case TokenType::GREATEREQ: return ">=";
        case TokenType::LESSEQ: return "<=";
        case TokenType::LBRACKET: return "[";
        case TokenType::RBRACKET: return "]";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::LBRACE: return "{";
        case TokenType::RBRACE: return "}";
        case TokenType::SEMICOLON: return ";";
        case TokenType::FOR: return "FOR";
        case TokenType::STEP: return "STEP";
        case TokenType::WHILE: return "WHILE";
        case TokenType::INSIDE: return "INSIDE";
        case TokenType::SUMMON: return "SUMMON";
        case TokenType::END: return "END";
    }
    return "UNKNOWN";
}

std::string describeToken(const Token& token) {
    std::string result = tokenTypeName(token.type);
    if (!token.value.empty()) {
        result += " ('" + token.value + "')";
    }
    return result;
}

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
    if (current.type != type) {
        throw std::runtime_error(
            "Parse error: expected " + tokenTypeName(type) + ", got " + describeToken(current));
    }
    advance();
}

std::vector<std::unique_ptr<Statement>> Parser::parseProgram() {
    std::vector<std::unique_ptr<Statement>> stmts;
    while (current.type != TokenType::END)
        stmts.push_back(parseStatement());
    return stmts;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if(current.type == TokenType::CLASS_TOKEN)
        return parseClassDecl();

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

    throw std::runtime_error("Parse error: unknown statement starting at " + describeToken(current));
}

std::unique_ptr<Statement> Parser::parseExprStatement() {
    auto stmt = std::make_unique<ExprStatement>();
    stmt->expr = parseExpr();
    expect(TokenType::SEMICOLON);
    return stmt;
}

std::unique_ptr<Statement> Parser::parseIf() {
    auto parseCondition = [this]() -> std::unique_ptr<Expr> {
        if (current.type == TokenType::LPAREN) {
            advance();
            auto condition = parseExpr();
            expect(TokenType::RPAREN);
            return condition;
        }

        return parseExpr();
    };

    auto parseBlock = [this](std::vector<std::unique_ptr<Statement>>& body) {
        expect(TokenType::LBRACE);
        while(current.type != TokenType::RBRACE)
            body.push_back(parseStatement());
        expect(TokenType::RBRACE);
    };

    expect(TokenType::IF);

    auto stmt = std::make_unique<IfStatement>();

    IfBranch firstBranch;
    firstBranch.condition = parseCondition();
    expect(TokenType::THEN);
    parseBlock(firstBranch.body);
    stmt->branches.push_back(std::move(firstBranch));

    while (current.type == TokenType::ELSE) {
        advance();

        if (current.type == TokenType::IF) {
            advance();

            IfBranch elseIfBranch;
            elseIfBranch.condition = parseCondition();
            expect(TokenType::THEN);
            parseBlock(elseIfBranch.body);
            stmt->branches.push_back(std::move(elseIfBranch));
            continue;
        }

        parseBlock(stmt->elseBody);
        break;
    }

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
    else if (current.type == TokenType::STRUCT_TOKEN) {
        advance();
        expect(TokenType::LBRACE);

        auto obj = std::make_unique<StructLiteralExpr>();

        if (current.type != TokenType::RBRACE) {
            while (true) {
                std::string fieldName = current.value;
                expect(TokenType::IDENT);
                expect(TokenType::COLON);
                obj->fields.push_back({fieldName, parseExpr()});

                if (current.type != TokenType::COMMA) {
                    break;
                }
                advance();
            }
        }

        expect(TokenType::RBRACE);
        left = std::move(obj);
    }
    else if(current.type == TokenType::IDENT) {
        const std::string firstIdent = current.value;
        auto var = std::make_unique<VariableExpr>();
        var->name = firstIdent;
        left = std::move(var);
        advance();

        if (firstIdent == "NEW" && current.type == TokenType::IDENT) {
            std::string className = current.value;
            advance();

            expect(TokenType::LPAREN);

            auto callExpr = std::make_unique<CallExpr>();
            callExpr->namespaceName = className;
            callExpr->function = "NEW";

            if(current.type != TokenType::RPAREN) {
                callExpr->args.push_back(parseExpr());
                while(current.type == TokenType::COMMA) {
                    advance();
                    callExpr->args.push_back(parseExpr());
                }
            }

            expect(TokenType::RPAREN);
            left = std::move(callExpr);
            return parsePostfix(std::move(left));
        }

        if (current.type == TokenType::LPAREN) {
            advance();

            auto callExpr = std::make_unique<CallExpr>();
            callExpr->namespaceName = "";
            callExpr->function = firstIdent;

            if (current.type != TokenType::RPAREN) {
                callExpr->args.push_back(parseExpr());
                while (current.type == TokenType::COMMA) {
                    advance();
                    callExpr->args.push_back(parseExpr());
                }
            }

            expect(TokenType::RPAREN);
            left = std::move(callExpr);
            return parsePostfix(std::move(left));
        }

        if(current.type == TokenType::COLONCOLON) {
            std::vector<std::string> segments;
            segments.push_back(firstIdent);

            while (current.type == TokenType::COLONCOLON) {
                advance();
                if(current.type != TokenType::IDENT && current.type != TokenType::IS && current.type != TokenType::ISNOT)
                    throw std::runtime_error("Parse error: expected identifier after ::, got " + describeToken(current));
                segments.push_back(current.value);
                advance();
            }

            if (segments.size() < 2)
                throw std::runtime_error("Parse error: expected function name after namespace qualifier");

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

            // Keep compatibility with the historical X::IS(...) / X::ISNOT(...) condition syntax.
            if ((callExpr->function == "IS" || callExpr->function == "ISNOT") &&
                segments.size() == 2 && callExpr->args.size() == 1) {
                auto varExpr = std::make_unique<VariableExpr>();
                varExpr->name = segments.front();

                auto bin = std::make_unique<BinaryExpr>();
                bin->left = std::move(varExpr);
                bin->right = std::move(callExpr->args[0]);
                bin->op = (callExpr->function == "IS") ? "==" : "!=";
                left = std::move(bin);
            } else {
                left = std::move(callExpr);
            }
        }
    }
    else if(current.type == TokenType::AMPERSAND) {
        advance();
        if(current.type != TokenType::IDENT)
            throw std::runtime_error("Parse error: expected identifier after &, got " + describeToken(current));
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
            throw std::runtime_error("Parse error: expected identifier after *, got " + describeToken(current));
        auto var = std::make_unique<VariableExpr>();
        var->name = current.value;
        left = std::move(var);
        advance();
    }
    else {
        throw std::runtime_error("Parse error: unexpected token in expression: " + describeToken(current));
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
        throw std::runtime_error("Parse error: expected type name after AS, got " + describeToken(current));
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

std::unique_ptr<Statement> Parser::parseClassDecl() {
    expect(TokenType::CLASS_TOKEN);

    std::string name = current.value;
    expect(TokenType::IDENT);

    std::string baseClass;
    std::vector<std::string> implementedInterfaces;

    bool hasInheritanceClause = false;
    bool hasImplementsClause = false;
    while (current.type == TokenType::INHERITS_TOKEN || current.type == TokenType::IMPLEMENTS_TOKEN) {
        if (current.type == TokenType::INHERITS_TOKEN) {
            if (hasInheritanceClause) {
                throw std::runtime_error("Parse error: duplicate INHERITS clause in class declaration " + name);
            }

            hasInheritanceClause = true;
            advance();
            baseClass = current.value;
            expect(TokenType::IDENT);
            continue;
        }

        if (hasImplementsClause) {
            throw std::runtime_error("Parse error: duplicate IMPLEMENTS clause in class declaration " + name);
        }

        hasImplementsClause = true;
        advance();

        implementedInterfaces.push_back(current.value);
        expect(TokenType::IDENT);
        while (current.type == TokenType::COMMA) {
            advance();
            implementedInterfaces.push_back(current.value);
            expect(TokenType::IDENT);
        }
    }

    expect(TokenType::LBRACE);

    auto stmt = std::make_unique<ClassDeclStatement>();
    stmt->name = name;
    stmt->baseClass = std::move(baseClass);
    stmt->implementedInterfaces = std::move(implementedInterfaces);

    while (current.type != TokenType::RBRACE) {
        if (current.type == TokenType::FUNCTION_TOKEN) {
            stmt->body.push_back(parseFunctionDecl());
            continue;
        }
        if (current.type == TokenType::OPERATOR_TOKEN) {
            stmt->body.push_back(parseOperatorDecl());
            continue;
        }

        throw std::runtime_error(
            "Parse error: class body only supports FUNCTION and OPERATOR declarations, got " + describeToken(current));
    }

    expect(TokenType::RBRACE);

    if(current.type == TokenType::SEMICOLON) advance();

    return stmt;
}

std::unique_ptr<Statement> Parser::parseOperatorDecl() {
    expect(TokenType::OPERATOR_TOKEN);

    std::string op;
    if (current.type == TokenType::PLUS) op = "+";
    else if (current.type == TokenType::MINUS) op = "-";
    else if (current.type == TokenType::ASTERISK) op = "*";
    else if (current.type == TokenType::SLASH) op = "/";
    else if (current.type == TokenType::FLOORDIV) op = "//";
    else if (current.type == TokenType::PERCENT) op = "%";
    else if (current.type == TokenType::EQEQ) op = "==";
    else if (current.type == TokenType::NOTEQUAL) op = "!=";
    else if (current.type == TokenType::GREATER) op = ">";
    else if (current.type == TokenType::LESS) op = "<";
    else if (current.type == TokenType::GREATEREQ) op = ">=";
    else if (current.type == TokenType::LESSEQ) op = "<=";
    else throw std::runtime_error("Parse error: unsupported OPERATOR token " + describeToken(current));
    advance();

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

    auto stmt = std::make_unique<OperatorDeclStatement>();
    stmt->opSymbol = op;
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
    if (current.type == TokenType::SEMICOLON) {
        auto lit = std::make_unique<LiteralExpr>();
        lit->value = Value();
        stmt->expr = std::move(lit);
    } else {
        stmt->expr = parseExpr();
    }

    expect(TokenType::SEMICOLON);
    return stmt;
}
