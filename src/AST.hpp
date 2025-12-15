#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

enum class ValueType { NUMBER, STRING, BOOLEAN };

struct Value {
    ValueType type;
    double number = 0;
    std::string string;
    bool boolean = false;
};

struct Variable {
    Value value;
    bool constant = false;
};

struct Expr;
struct Statement;

struct Expr {
    virtual ~Expr() = default;
    virtual Value evaluate() = 0;
};

struct LiteralExpr : Expr {
    Value value;
    Value evaluate() override { return value; }
};

struct VariableExpr : Expr {
    std::string name;
    Value evaluate();
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    char op;
    Value evaluate() override;
};

struct Statement {
    virtual ~Statement() = default;
    virtual void execute() = 0;
};

struct SetStatement : Statement {
    std::string name;
    std::unique_ptr<Expr> expr;
    bool constant = false;
    void execute() override;
};

struct PrintStatement : Statement {
    std::unique_ptr<Expr> expr;
    void execute() override;
};

struct IfStatement : Statement {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Statement>> body;
    void execute() override;
};