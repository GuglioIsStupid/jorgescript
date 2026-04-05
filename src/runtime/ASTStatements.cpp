#include "AST.hpp"
#include "ClibRuntime.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Resources.hpp"
#include "Runtime.hpp"
#include "modules/TestsFunctions.hpp"
#include "ValueUtils.hpp"
#include <cmath>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {
struct FunctionReturnSignal {
    Value value;
};

Value invokeUserFunction(const std::string& qualifiedName, const std::vector<std::unique_ptr<Expr>>& args) {
    auto fnIt = UserFunctions.find(qualifiedName);
    if (fnIt == UserFunctions.end()) {
        throw std::runtime_error("Undefined function: " + qualifiedName);
    }

    const auto& fn = fnIt->second;
    if (args.size() != fn->params.size()) {
        throw std::runtime_error("Function argument mismatch for " + qualifiedName);
    }

    std::vector<Value> evaluatedArgs;
    evaluatedArgs.reserve(args.size());
    for (const auto& arg : args) {
        evaluatedArgs.push_back(arg->evaluate());
    }

    pushScope();
    for (size_t i = 0; i < fn->params.size(); ++i) {
        ScopeStack.back()[fn->params[i]] = {evaluatedArgs[i], false};
    }

    try {
        for (const auto& stmt : fn->body) {
            stmt->execute();
        }
    } catch (const FunctionReturnSignal& signal) {
        popScope();
        return signal.value;
    }

    popScope();
    return Value();
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + filename);

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return content;
}

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

std::string parseNamespaceDirective(std::string& src) {
    std::istringstream in(src);
    std::ostringstream out;
    std::string line;
    std::string namespaceName;
    bool consumedDirective = false;

    while (std::getline(in, line)) {
        const std::string stripped = trim(line);
        if (!consumedDirective && stripped.rfind("NAMESPACE ", 0) == 0) {
            namespaceName = trim(stripped.substr(10));
            if (!namespaceName.empty() && namespaceName.back() == ';') {
                namespaceName.pop_back();
                namespaceName = trim(namespaceName);
            }
            consumedDirective = true;
            continue;
        }
        out << line << '\n';
    }

    src = out.str();
    return namespaceName;
}
}

Value invokeNamespaceFunction(const std::string& namespaceName, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args) {
    Value nativeResult;
    if (tryInvokeNativeNamespaceFunction(namespaceName, function, args, nativeResult)) {
        return nativeResult;
    }

    const std::string qualifiedName = getQualifiedFunctionName(namespaceName, function);
    if (UserFunctions.find(qualifiedName) != UserFunctions.end()) {
        return invokeUserFunction(qualifiedName, args);
    }

    throw std::runtime_error("Undefined namespaced function: " + qualifiedName);
}

void SetStatement::execute() {
    Value val = expr->evaluate();

    if (indexExpr) {
        Variable* indexedVar = findVariable(name);
        if (!indexedVar) {
            ScopeStack.front()[name] = {Value(std::make_shared<ArrayObject>()), false};
            indexedVar = findVariable(name);
        }

        if (indexedVar->isconstant)
            throw std::runtime_error("Cannot modify constant: " + name);

        if (indexedVar->value.type != ValueType::ARRAY || !indexedVar->value.array) {
            throw std::runtime_error("Indexed assignment requires array variable: " + name);
        }

        Value idx = indexExpr->evaluate();
        if (idx.type == ValueType::STRING) {
            indexedVar->value.array->named[idx.string] = val;
            return;
        }

        if (idx.type != ValueType::NUMBER)
            throw std::runtime_error("Array index must be number or string");

        if (std::trunc(idx.number) != idx.number)
            throw std::runtime_error("Array numeric index must be an integer");

        const long long logicalIndex = static_cast<long long>(idx.number);
        const long long storageIndex = logicalIndex + 1;
        if (storageIndex < 0)
            throw std::runtime_error("Array index out of range (minimum is -1)");

        if (static_cast<size_t>(storageIndex) >= indexedVar->value.array->indexed.size()) {
            indexedVar->value.array->indexed.resize(static_cast<size_t>(storageIndex) + 1);
        }

        indexedVar->value.array->indexed[static_cast<size_t>(storageIndex)] = val;
        return;
    }

    if (isLocal) {
        auto& localScope = ScopeStack.back();
        localScope[name] = {val, isconstant};
        return;
    }

    for (auto it = ScopeStack.rbegin(); it != ScopeStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            if (found->second.isconstant)
                throw std::runtime_error("Cannot modify constant: " + name);
            found->second.value = val;
            return;
        }
    }

    ScopeStack.front()[name] = {val, isconstant};
}

void PrintStatement::execute() {
    Value val = resolvePotentialBoolean(expr->evaluate());
    switch (val.type) {
        case ValueType::STRING:  std::cout << val.string; break;
        case ValueType::NUMBER:  std::cout << val.number; break;
        case ValueType::BOOLEAN: std::cout << (val.boolean ? "TRUE!" : "Untrue..."); break;
        case ValueType::POTENTIAL_BOOLEAN: std::cout << (randomBoolean() ? "TRUE!" : "Untrue..."); break;
        case ValueType::NOTHING: std::cout << "NOTHING"; break;
        case ValueType::POINTER: std::cout << valueToString(val); break;
        default:                 std::cout << "IDK"; break;
    }
    std::cout << "\n";
}

void ExprStatement::execute() {
    (void)expr->evaluate();
}

void IfStatement::execute() {
    Value cond = resolvePotentialBoolean(condition->evaluate());
    if (cond.type != ValueType::BOOLEAN)
        throw std::runtime_error("IF condition must be boolean");
    if (cond.boolean) {
        for (auto& stmt : body)
            stmt->execute();
    }
}

void LoadClibStatement::execute() {
    ClibHandle mod = loadClibHandle(clibName);

    auto existing = LoadedCLIBs.find(alias);
    if (existing != LoadedCLIBs.end()) {
        closeClib(existing->second);
    }

    LoadedCLIBs[alias] = mod;
}

void CallClibStatement::execute() {
    (void)invokeClibFunction(alias, function, args);
}

void SetClibReturnTypeStatement::execute() {
    ClibReturnTypes[getClibSymbolKey(alias, function)] = returnType;
}

void SummonStatement::execute() {
    std::string src;

    if (filename.substr(0, 4) == "STD." || filename.substr(0, 4) == "std.") {
        std::string resourceName = filename + ".jorge";
        std::string_view resourceContent = Resources::getResource(resourceName);
        if (resourceContent.empty()) {
            throw std::runtime_error("Standard library not found: " + filename);
        }

        src = std::string(resourceContent);
    } else {
        src = readFile(filename);
    }

    const std::string declaredNamespace = parseNamespaceDirective(src);

    Lexer lexer(src);
    Parser parser(lexer);
    auto program = parser.parseProgram();

    ScopeStack.push_back({});

    bool namespacePushed = false;
    if (!declaredNamespace.empty()) {
        NamespaceStack.push_back(declaredNamespace);
        namespacePushed = true;
    }

    try {
        for (auto& stmt : program)
            stmt->execute();

        if (!declaredNamespace.empty()) {
            FileScopes[declaredNamespace] = ScopeStack.back();
            NamespaceStack.pop_back();
            namespacePushed = false;
        }

        if (!alias.empty()) {
            FileScopes[alias] = ScopeStack.back();
        }
    } catch (...) {
        if (namespacePushed) {
            NamespaceStack.pop_back();
        }
        ScopeStack.pop_back();
        throw;
    }

    ScopeStack.pop_back();
}

void WhileStatement::execute() {
    while (condition->evaluate().boolean) {
        for (auto& stmt : body)
            stmt->execute();
    }
}

void ForStatement::execute() {
    Value startVal = startExpr->evaluate();
    Value endVal = endExpr->evaluate();
    Value stepVal = stepExpr ? stepExpr->evaluate() : Value(1.0);

    if (startVal.type != ValueType::NUMBER || endVal.type != ValueType::NUMBER || stepVal.type != ValueType::NUMBER)
        throw std::runtime_error("FOR loop bounds must be numbers");

    double i = startVal.number;
    double end = endVal.number;
    double step = stepVal.number;

    while ((step > 0 && i <= end) || (step < 0 && i >= end)) {
        ScopeStack.back()[varName] = {Value(i), false};

        for (auto& stmt : body)
            stmt->execute();

        i += step;
    }

    ScopeStack.back().erase(varName);
}

void FunctionDeclStatement::execute() {
    std::string activeNamespace;
    if (!NamespaceStack.empty()) {
        activeNamespace = NamespaceStack.back();
    }

    auto def = std::make_shared<FunctionDefinition>();
    def->params = std::move(params);
    def->body = std::move(body);

    UserFunctions[getQualifiedFunctionName(activeNamespace, name)] = std::move(def);
}

void ReturnStatement::execute() {
    throw FunctionReturnSignal{expr->evaluate()};
}
