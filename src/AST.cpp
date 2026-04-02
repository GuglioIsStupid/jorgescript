#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "Runtime.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>

namespace {
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

std::string valueToString(const Value& v) {
    switch (v.type) {
        case ValueType::STRING:
            return v.string;
        case ValueType::NUMBER:
            return std::to_string(v.number);
        case ValueType::BOOLEAN:
            return v.boolean ? "TRUE!" : "FALSE!";
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
    return v->value;
}

Value BinaryExpr::evaluate() {
    Value l = left->evaluate();
    Value r = right->evaluate();

    if(op == '+') {
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
    else if(op == '-') {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number - r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for -");
    }
    else if(op == '*') {
        if(l.type==ValueType::NUMBER && r.type==ValueType::NUMBER) {
            Value val;
            val.type = ValueType::NUMBER;
            val.number = l.number * r.number;
            return val;
        }
        throw std::runtime_error("Invalid types for *");
    }
    else if(op == '/') {
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
    else if(op == '%') {
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
    else if(op == '=') {
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
    Value val = expr->evaluate();
    switch(val.type){
        case ValueType::STRING:  std::cout << val.string; break;
        case ValueType::NUMBER:  std::cout << val.number; break;
        case ValueType::BOOLEAN: std::cout << (val.boolean?"TRUE!":"Untrue..."); break;
        case ValueType::NOTHING: std::cout << "NOTHING"; break;
        case ValueType::POINTER: std::cout << valueToString(val); break;
        default:                 std::cout << "IDK"; break;
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
    std::string src = readFile(filename);

    Lexer lexer(src);
    Parser parser(lexer);
    auto program = parser.parseProgram();

    ScopeStack.push_back({});

    for (auto& stmt : program)
        stmt->execute();

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
    auto* varExpr = dynamic_cast<VariableExpr*>(object.get());
    if (!varExpr)
        throw std::runtime_error("DLL call object must be an identifier alias");

    return invokeDllFunction(varExpr->name, function, args);
}
