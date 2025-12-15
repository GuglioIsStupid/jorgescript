#include "AST.hpp"
#include "Runtime.hpp"
#include <stdexcept>

Value VariableExpr::evaluate() {
    Variable* v = findVariable(name);
    if(!v) throw std::runtime_error("Undefined variable: " + name);
    return v->value;
}

Value BinaryExpr::evaluate() {
    Value l = left->evaluate();
    Value r = right->evaluate();

    if(op == '+') {
        if(l.type == ValueType::STRING || r.type == ValueType::STRING) {
            std::string ls = (l.type==ValueType::STRING)?l.string:(l.type==ValueType::NUMBER)?std::to_string(l.number):(l.boolean?"TRUE!":"FALSE!");
            std::string rs = (r.type==ValueType::STRING)?r.string:(r.type==ValueType::NUMBER)?std::to_string(r.number):(r.boolean?"TRUE!":"FALSE!");
            return {ValueType::STRING, 0, ls+rs};
        }
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER)
            return {ValueType::NUMBER, l.number + r.number};
        throw std::runtime_error("Invalid types for +");
    }
    else if(op == '=') { // equality
        if(l.type != r.type) return {ValueType::BOOLEAN,false};
        if(l.type==ValueType::NUMBER) return {ValueType::BOOLEAN,0,"", l.number==r.number};
        if(l.type==ValueType::STRING) return {ValueType::BOOLEAN,0,"", l.string==r.string};
        if(l.type==ValueType::BOOLEAN) return {ValueType::BOOLEAN,0,"", l.boolean==r.boolean};
    }

    throw std::runtime_error("Unsupported binary op");
}

void SetStatement::execute() {
    auto& scope = ScopeStack.back();
    auto it = scope.find(name);
    if(it != scope.end() && it->second.constant)
        throw std::runtime_error("Cannot modify constant: " + name);

    Value val = expr->evaluate();
    scope[name] = {val, constant};
}

void PrintStatement::execute() {
    Value val = expr->evaluate();
    switch(val.type){
        case ValueType::STRING:  std::cout << val.string; break;
        case ValueType::NUMBER:  std::cout << val.number; break;
        case ValueType::BOOLEAN: std::cout << (val.boolean?"TRUE!":"Untrue..."); break;
    }
    std::cout << "\n";
}

void IfStatement::execute() {
    Value cond = condition->evaluate();
    if(cond.type != ValueType::BOOLEAN)
        throw std::runtime_error("IF condition must be boolean");
    if(cond.boolean) {
        for(auto& stmt : body)
            stmt->execute();
    }
}