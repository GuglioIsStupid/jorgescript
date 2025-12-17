#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

struct Expr;
struct Statement;

enum class ValueType { NOTHING, NUMBER, STRING, BOOLEAN, IDK };

struct Value {
    ValueType type = ValueType::NOTHING;
    double number = 0;
    std::string string;
    bool boolean = false;

    Value() : type(ValueType::NOTHING) {}
    
    Value(double n) : type(ValueType::NUMBER), number(n) {}
    Value(const std::string& s) : type(ValueType::STRING), string(s) {}
    Value(bool b) : type(ValueType::BOOLEAN), boolean(b) {}
};

struct Variable {
    Value value;
    bool isconstant = false;
};

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
    bool isconstant = false;
    bool isLocal = false;
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

struct LoadDllStatement : Statement {
    std::string dllName;
    std::string alias;
    void execute() override;
};

struct CallDllStatement : Statement {
    std::string alias;
    std::string function;
    std::vector<std::unique_ptr<Expr>> args;
    void execute() override;
};

struct SummonStatement : Statement {
    std::string filename;
    std::string alias;
    void execute() override;
};

struct WhileStatement : Statement {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Statement>> body;
    void execute() override;
};

struct ForStatement : Statement {
    std::string varName;
    std::unique_ptr<Expr> startExpr;
    std::unique_ptr<Expr> endExpr;
    std::unique_ptr<Expr> stepExpr;
    std::vector<std::unique_ptr<Statement>> body;
    void execute() override;
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> object;
    std::string function;
    std::vector<std::unique_ptr<Expr>> args;

    Value evaluate() override {
        std::cout << "CallExpr: " << function << "()" << std::endl;
        return Value();
    }
};