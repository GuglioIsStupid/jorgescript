#include "AST.hpp"
#include "ClibRuntime.hpp"
#include "Runtime.hpp"
#include "ValueUtils.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>

Value VariableExpr::evaluate() {
    Variable* v = findVariable(name);
    if (!v) {
        if (DefinedClasses.find(name) != DefinedClasses.end()) {
            Value classRef;
            classRef.type = ValueType::CLASS;
            classRef.string = name;
            return classRef;
        }

        throw std::runtime_error("Undefined variable: " + name);
    }
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

Value StructLiteralExpr::evaluate() {
    auto obj = std::make_shared<ArrayObject>();
    for (auto& field : fields) {
        obj->named[field.first] = field.second->evaluate();
    }

    Value result(obj);
    result.customType = "STRUCT";
    return result;
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

    std::string className;
    if (!l.customType.empty() && DefinedClasses.find(l.customType) != DefinedClasses.end()) {
        className = l.customType;
    }
    if (!r.customType.empty() && DefinedClasses.find(r.customType) != DefinedClasses.end()) {
        if (!className.empty() && className != r.customType) {
            throw std::runtime_error("Cannot use operator " + op + " between class instances of different types: " + className + " and " + r.customType);
        }
        className = r.customType;
    }

    if (!className.empty() && isOperatorOverloadable(op)) {
        const std::string overloadFunction = getOperatorOverloadFunctionName(op);
        const std::string overloadOwner = resolveClassFunctionNamespace(className, overloadFunction);
        if (!overloadOwner.empty()) {
            auto leftArg = std::make_unique<LiteralExpr>();
            leftArg->value = l;

            auto rightArg = std::make_unique<LiteralExpr>();
            rightArg->value = r;

            std::vector<std::unique_ptr<Expr>> overloadArgs;
            overloadArgs.push_back(std::move(leftArg));
            overloadArgs.push_back(std::move(rightArg));
            return invokeNamespaceFunction(className, overloadFunction, overloadArgs);
        }
    }

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
            case ValueType::CLASS: return a.string == b.string;
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

    return invokeNamespaceFunction(namespaceName, function, args);
}
