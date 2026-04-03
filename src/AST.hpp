#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cstdint>

struct Expr;
struct Statement;
struct ArrayObject;

enum class ValueType { NOTHING, NUMBER, STRING, BOOLEAN, POTENTIAL_BOOLEAN, ARRAY, POINTER, IDK };

struct Value {
    ValueType type = ValueType::NOTHING;
    double number = 0;
    std::string string;
    bool boolean = false;
    std::shared_ptr<ArrayObject> array;
    std::uintptr_t pointer = 0;
    std::string customType;

    Value() : type(ValueType::NOTHING) {}
    
    Value(double n) : type(ValueType::NUMBER), number(n) {}
    Value(const std::string& s) : type(ValueType::STRING), string(s) {}
    Value(bool b) : type(ValueType::BOOLEAN), boolean(b) {}
    Value(std::shared_ptr<ArrayObject> a) : type(ValueType::ARRAY), array(std::move(a)) {}
    Value(std::uintptr_t p, const std::string& t = "POINTER")
        : type(ValueType::POINTER), pointer(p), customType(t) {}
};

struct ArrayObject {
    std::vector<Value> indexed;
    std::unordered_map<std::string, Value> named;
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
    std::string op;
    Value evaluate() override;
};

struct ArrayLiteralExpr : Expr {
    std::vector<std::unique_ptr<Expr>> elements;
    Value evaluate() override;
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    Value evaluate() override;
};

struct Statement {
    virtual ~Statement() = default;
    virtual void execute() = 0;
};

struct FunctionDefinition {
    std::vector<std::string> params;
    std::vector<std::unique_ptr<Statement>> body;
};

struct SetStatement : Statement {
    std::string name;
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Expr> indexExpr;
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

struct SetDllReturnTypeStatement : Statement {
    std::string alias;
    std::string function;
    std::string returnType;
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

struct FunctionDeclStatement : Statement {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<Statement>> body;
    void execute() override;
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expr> expr;
    void execute() override;
};

struct CallExpr : Expr {
    std::string namespaceName;
    std::string function;
    std::vector<std::unique_ptr<Expr>> args;

    Value evaluate() override;
};