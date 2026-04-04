#include "AST.hpp"
#include "ClibRuntime.hpp"
#include "Runtime.hpp"
#include "ValueUtils.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>

Value VariableExpr::evaluate() {
    Variable* v = findVariable(name);
    if (!v) throw std::runtime_error("Undefined variable: " + name);
    if (v->value.type == ValueType::POTENTIAL_BOOLEAN) {
        return Value(randomBoolean());
    }
    return v->value;
}

Value ArrayLiteralExpr::evaluate() {
    auto arr = std::make_shared<ArrayObject>();
    for (auto& elem : elements) {
        arr->indexed.push_back(elem->evaluate());
    }
    return Value(arr);
}

Value IndexExpr::evaluate() {
    Value targetValue = target->evaluate();
    if (targetValue.type != ValueType::ARRAY || !targetValue.array) {
        throw std::runtime_error("Index access requires an array");
    }

    Value idx = index->evaluate();
    if (idx.type == ValueType::STRING) {
        auto it = targetValue.array->named.find(idx.string);
        if (it == targetValue.array->named.end()) {
            return Value();
        }
        return it->second;
    }

    if (idx.type != ValueType::NUMBER) {
        throw std::runtime_error("Array index must be number or string");
    }

    if (std::trunc(idx.number) != idx.number) {
        throw std::runtime_error("Array numeric index must be an integer");
    }

    const long long logicalIndex = static_cast<long long>(idx.number);
    const long long storageIndex = logicalIndex + 1;
    if (storageIndex < 0 || static_cast<size_t>(storageIndex) >= targetValue.array->indexed.size()) {
        return Value();
    }

    return targetValue.array->indexed[static_cast<size_t>(storageIndex)];
}

Value BinaryExpr::evaluate() {
    Value l = resolvePotentialBoolean(left->evaluate());
    Value r = resolvePotentialBoolean(right->evaluate());

    if (op == "+") {
        if (l.type == ValueType::STRING || r.type == ValueType::STRING) {
            Value val;
            val.type = ValueType::STRING;
            val.string = valueToString(l) + valueToString(r);
            return val;
        }
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number + r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for +");
    }
    if (op == "-") {
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number - r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for -");
    }
    if (op == "*") {
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number * r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for *");
    }
    if (op == "/") {
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            if (r.number == 0)
                throw std::runtime_error("Division by zero");
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number / r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for /");
    }
    if (op == "//") {
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            if (r.number == 0)
                throw std::runtime_error("Division by zero");
            Value val;
            val.type = ValueType::NUMBER;
            val.number = std::floor(l.number / r.number);
            return val;
        }
        throw std::runtime_error("Invalid types for //");
    }
    if (op == "%") {
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            if (r.number == 0)
                throw std::runtime_error("Modulo by zero");
            Value val;
            val.type = ValueType::NUMBER;
            val.number = static_cast<double>(static_cast<long long>(l.number) % static_cast<long long>(r.number));
            return val;
        }
        throw std::runtime_error("Invalid types for %");
    }

    auto makeBoolean = [](bool b) {
        Value val;
        val.type = ValueType::BOOLEAN;
        val.boolean = b;
        return val;
    };

    auto valueEquals = [](const Value& a, const Value& b) {
        if (a.type != b.type) return false;
        switch (a.type) {
            case ValueType::NUMBER: return a.number == b.number;
            case ValueType::STRING: return a.string == b.string;
            case ValueType::BOOLEAN: return a.boolean == b.boolean;
            case ValueType::POINTER: return a.pointer == b.pointer;
            case ValueType::NOTHING: return true;
            default: return false;
        }
    };

    if (op == "==") {
        return makeBoolean(valueEquals(l, r));
    }

    if (op == "!=") {
        return makeBoolean(!valueEquals(l, r));
    }

    if (op == ">" || op == "<" || op == ">=" || op == "<=") {
        if (l.type == ValueType::NUMBER && r.type == ValueType::NUMBER) {
            if (op == ">") return makeBoolean(l.number > r.number);
            if (op == "<") return makeBoolean(l.number < r.number);
            if (op == ">=") return makeBoolean(l.number >= r.number);
            return makeBoolean(l.number <= r.number);
        }

        if (l.type == ValueType::STRING && r.type == ValueType::STRING) {
            if (op == ">") return makeBoolean(l.string > r.string);
            if (op == "<") return makeBoolean(l.string < r.string);
            if (op == ">=") return makeBoolean(l.string >= r.string);
            return makeBoolean(l.string <= r.string);
        }

        throw std::runtime_error("Invalid types for comparison operator " + op);
    }

    if (op == "=") {
        Value val;
        val.type = ValueType::BOOLEAN;
        if (l.type != r.type) {
            val.boolean = false;
        } else if (l.type == ValueType::NUMBER) {
            val.boolean = (l.number == r.number);
        } else if (l.type == ValueType::STRING) {
            val.boolean = (l.string == r.string);
        } else if (l.type == ValueType::BOOLEAN) {
            val.boolean = (l.boolean == r.boolean);
        } else if (l.type == ValueType::POINTER) {
            val.boolean = (l.pointer == r.pointer);
        } else {
            val.boolean = false;
        }
        return val;
    }

    throw std::runtime_error("Unsupported binary op");
}

Value CallExpr::evaluate() {
    if (LoadedCLIBs.find(namespaceName) != LoadedCLIBs.end()) {
        return invokeClibFunction(namespaceName, function, args);
    }

    if (FileScopes.find(namespaceName) != FileScopes.end()) {
        return invokeNamespaceFunction(namespaceName, function, args);
    }

    throw std::runtime_error("Unknown call namespace: " + namespaceName);
}
