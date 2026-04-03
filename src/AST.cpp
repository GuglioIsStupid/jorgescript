#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "Runtime.hpp"
#include "Resources.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>
#include <random>
#include <cmath>

namespace {
struct FunctionReturnSignal {
    Value value;
};

std::wstring utf8ToUtf16(const std::string& s) {
    if (s.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &result[0], size);
    result.pop_back();
    return result;
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

std::string valueToString(const Value& v) {
    switch (v.type) {
        case ValueType::STRING:
            return v.string;
        case ValueType::NUMBER:
            return std::to_string(v.number);
        case ValueType::BOOLEAN:
            return v.boolean ? "TRUE!" : "FALSE!";
        case ValueType::POTENTIAL_BOOLEAN:
            return "POTENTIONABLY";
        case ValueType::ARRAY: {
            if (!v.array) {
                return "[]";
            }

            std::ostringstream oss;
            oss << "[";
            for (size_t i = 0; i < v.array->indexed.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << valueToString(v.array->indexed[i]);
            }
            if (!v.array->named.empty()) {
                if (!v.array->indexed.empty()) oss << ", ";
                bool first = true;
                for (const auto& [k, val] : v.array->named) {
                    if (!first) oss << ", ";
                    first = false;
                    oss << k << "=" << valueToString(val);
                }
            }
            oss << "]";
            return oss.str();
        }
        case ValueType::NOTHING:
            return "NOTHING";
        case ValueType::POINTER: {
            std::ostringstream oss;
            oss << (v.customType.empty() ? "POINTER" : v.customType)
                << "@0x" << std::hex << std::uppercase << v.pointer;
            return oss.str();
        }
        default:
            return "IDK";
    }
}

bool randomBoolean() {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_int_distribution<int> dist(0, 1);
    return dist(rng) == 1;
}

Value resolvePotentialBoolean(const Value& v) {
    if (v.type != ValueType::POTENTIAL_BOOLEAN) {
        return v;
    }
    return Value(randomBoolean());
}

std::uintptr_t callRawProc(FARPROC proc, const std::vector<std::uintptr_t>& args) {
    // todo: support more than 8 args ???
    switch (args.size()) {
        case 0: {
            using Fn = std::uintptr_t(*)();
            return reinterpret_cast<Fn>(proc)();
        }
        case 1: {
            using Fn = std::uintptr_t(*)(std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0]);
        }
        case 2: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1]);
        }
        case 3: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2]);
        }
        case 4: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3]);
        }
        case 5: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4]);
        }
        case 6: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4], args[5]);
        }
        case 7: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        }
        case 8: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        }
        default:
            throw std::runtime_error("Too many DLL arguments (max 8)");
    }
}

std::string normalizeTypeName(std::string typeName) {
    for (char& ch : typeName) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return typeName;
}

std::string getQualifiedFunctionName(const std::string& namespaceName, const std::string& functionName) {
    if (namespaceName.empty()) {
        return functionName;
    }
    return namespaceName + "::" + functionName;
}

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

Value invokeNamespaceFunction(const std::string& namespaceName, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args) {
    const std::string qualifiedName = getQualifiedFunctionName(namespaceName, function);
    if (UserFunctions.contains(qualifiedName)) {
        return invokeUserFunction(qualifiedName, args);
    }

    throw std::runtime_error("Undefined namespaced function: " + qualifiedName);
}

Value invokeDllFunction(const std::string& alias, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args) {
    auto it = LoadedDLLs.find(alias);
    if (it == LoadedDLLs.end())
        throw std::runtime_error("DLL not loaded: " + alias);

    FARPROC proc = GetProcAddress(it->second, function.c_str());
    if (!proc)
        throw std::runtime_error("Function not found: " + function);

    std::vector<std::string> utf8Strings;
    std::vector<std::wstring> utf16Strings;
    std::vector<std::uintptr_t> rawArgs;
    rawArgs.reserve(args.size());

    const bool wantsWideStrings = !function.empty() && function.back() == 'W';

    for (const auto& expr : args) {
        Value v = expr->evaluate();
        if (v.type == ValueType::NUMBER) {
            rawArgs.push_back(static_cast<std::uintptr_t>(static_cast<std::intptr_t>(v.number)));
        } else if (v.type == ValueType::BOOLEAN) {
            rawArgs.push_back(v.boolean ? 1u : 0u);
        } else if (v.type == ValueType::STRING) {
            if (wantsWideStrings) {
                utf16Strings.push_back(utf8ToUtf16(v.string));
                rawArgs.push_back(reinterpret_cast<std::uintptr_t>(utf16Strings.back().c_str()));
            } else {
                utf8Strings.push_back(v.string);
                rawArgs.push_back(reinterpret_cast<std::uintptr_t>(utf8Strings.back().c_str()));
            }
        } else if (v.type == ValueType::POINTER) {
            rawArgs.push_back(v.pointer);
        } else if (v.type == ValueType::NOTHING) {
            rawArgs.push_back(0);
        } else {
            throw std::runtime_error("Unsupported DLL argument type");
        }
    }

    const std::string key = getDllSymbolKey(alias, function);
    auto retIt = DllReturnTypes.find(key);
    const std::string returnType = (retIt != DllReturnTypes.end()) ? retIt->second : "POINTER";
    const std::string normalizedType = normalizeTypeName(returnType);

    std::uintptr_t result = callRawProc(proc, rawArgs);

    if (normalizedType == "BOOLEAN" || normalizedType == "BOOL") {
        return Value(result != 0);
    }

    if (normalizedType == "NUMBER" || normalizedType == "INT" || normalizedType == "INT32" || normalizedType == "UINT32") {
        return Value(static_cast<double>(static_cast<std::int64_t>(result)));
    }

    if (result == 0)
        return Value();

    return Value(result, returnType);
}

} // namespace

Value VariableExpr::evaluate() {
    Variable* v = findVariable(name);
    if(!v) throw std::runtime_error("Undefined variable: " + name);
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

    if(op == "+") {
        if(l.type == ValueType::STRING || r.type == ValueType::STRING) {
            std::string ls = valueToString(l);
            std::string rs = valueToString(r);
            Value val;
            val.type = ValueType::STRING;
            val.string = ls + rs;
            return val;
        }
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number + r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for +");
    }
    else if(op == "-") {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number - r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for -");
    }
    else if(op == "*") {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number * r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for *");
    }
    else if(op == "/") {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            if(r.number == 0)
                throw std::runtime_error("Division by zero");
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number / r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for /");
    }
    else if(op == "//") {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            if(r.number == 0)
                throw std::runtime_error("Division by zero");
            Value val;
            val.type = ValueType::NUMBER;
            val.number = std::floor(l.number / r.number);
            return val;
        }
        throw std::runtime_error("Invalid types for //");
    }
    else if(op == "%") {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            if(r.number == 0)
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

    if(op == "==") {
        return makeBoolean(valueEquals(l, r));
    }

    if(op == "!=") {
        return makeBoolean(!valueEquals(l, r));
    }

    if(op == ">" || op == "<" || op == ">=" || op == "<=") {
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

    else if(op == "=") {
        Value val;
        val.type = ValueType::BOOLEAN;
        if(l.type != r.type) {
            val.boolean = false;
        } else if(l.type==ValueType::NUMBER) {
            val.boolean = (l.number == r.number);
        } else if(l.type==ValueType::STRING) {
            val.boolean = (l.string == r.string);
        } else if(l.type==ValueType::BOOLEAN) {
            val.boolean = (l.boolean == r.boolean);
        } else if(l.type==ValueType::POINTER) {
            val.boolean = (l.pointer == r.pointer);
        } else {
            val.boolean = false;
        }
        return val;
    }

    throw std::runtime_error("Unsupported binary op");
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
    switch(val.type){
        case ValueType::STRING:  std::cout << val.string; break;
        case ValueType::NUMBER:  std::cout << val.number; break;
        case ValueType::BOOLEAN: std::cout << (val.boolean?"TRUE!":"Untrue..."); break;
        case ValueType::POTENTIAL_BOOLEAN: std::cout << (randomBoolean()?"TRUE!":"Untrue..."); break;
        case ValueType::NOTHING: std::cout << "NOTHING"; break;
        case ValueType::POINTER: std::cout << valueToString(val); break;
        default:                 std::cout << "IDK"; break;
    }
    std::cout << "\n";
}

void IfStatement::execute() {
    Value cond = resolvePotentialBoolean(condition->evaluate());
    if(cond.type != ValueType::BOOLEAN)
        throw std::runtime_error("IF condition must be boolean");
    if(cond.boolean) {
        for(auto& stmt : body)
            stmt->execute();
    }
}

void LoadDllStatement::execute() {
    HMODULE mod = LoadLibraryA(dllName.c_str());
    if (!mod)
        throw std::runtime_error("Failed to load DLL: " + dllName);

    LoadedDLLs[alias] = mod;
}

void CallDllStatement::execute() {
    (void)invokeDllFunction(alias, function, args);
}

void SetDllReturnTypeStatement::execute() {
    DllReturnTypes[getDllSymbolKey(alias, function)] = returnType;
}

void SummonStatement::execute() {
    std::string src;
    
    // Check if this is a standard library (STD.*)
    if (filename.substr(0, 4) == "STD." || filename.substr(0, 4) == "std.") {
        // Construct the resource name by appending .jorge
        std::string resourceName = filename + ".jorge";
        
        // Check if the resource exists
        std::string_view resourceContent = Resources::getResource(resourceName);
        if (resourceContent.empty()) {
            throw std::runtime_error("Standard library not found: " + filename);
        }
        
        src = std::string(resourceContent);
    } else {
        // Load from file system
        src = readFile(filename);
    }

    const std::string declaredNamespace = parseNamespaceDirective(src);

    Lexer lexer(src);
    Parser parser(lexer);
    auto program = parser.parseProgram();

    ScopeStack.push_back({});
    if (!declaredNamespace.empty()) {
        NamespaceStack.push_back(declaredNamespace);
    }

    for (auto& stmt : program)
        stmt->execute();

    if (!declaredNamespace.empty()) {
        FileScopes[declaredNamespace] = ScopeStack.back();
        NamespaceStack.pop_back();
    }

    if (!alias.empty()) {
        FileScopes[alias] = ScopeStack.back();
    }

    ScopeStack.pop_back();
}

void WhileStatement::execute() {
    while(condition->evaluate().boolean) {
        for(auto& stmt : body)
            stmt->execute();
    }
}

void ForStatement::execute() {
    Value startVal = startExpr->evaluate();
    Value endVal = endExpr->evaluate();
    Value stepVal = stepExpr ? stepExpr->evaluate() : Value(1.0);

    if(startVal.type != ValueType::NUMBER || endVal.type != ValueType::NUMBER || stepVal.type != ValueType::NUMBER)
        throw std::runtime_error("FOR loop bounds must be numbers");

    double i = startVal.number;
    double end = endVal.number;
    double step = stepVal.number;

    while ((step > 0 && i <= end) || (step < 0 && i >= end)) {
        ScopeStack.back()[varName] = {Value(i), false};

        for(auto& stmt : body)
            stmt->execute();

        i += step;
    }

    ScopeStack.back().erase(varName);
}

Value CallExpr::evaluate() {
    if (LoadedDLLs.contains(namespaceName)) {
        return invokeDllFunction(namespaceName, function, args);
    }

    if (FileScopes.contains(namespaceName)) {
        return invokeNamespaceFunction(namespaceName, function, args);
    }

    throw std::runtime_error("Unknown call namespace: " + namespaceName);
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
