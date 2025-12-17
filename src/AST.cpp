#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "Runtime.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>

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
            std::string ls = (l.type==ValueType::STRING) ? l.string :
                             (l.type==ValueType::NUMBER) ? std::to_string(l.number) :
                             (l.boolean ? "TRUE!" : "FALSE!");
            std::string rs = (r.type==ValueType::STRING) ? r.string :
                             (r.type==ValueType::NUMBER) ? std::to_string(r.number) :
                             (r.boolean ? "TRUE!" : "FALSE!");
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
    auto it = LoadedDLLs.find(alias);
    if (it == LoadedDLLs.end())
        throw std::runtime_error("DLL not loaded: " + alias);

    FARPROC proc = GetProcAddress(it->second, function.c_str());
    if (!proc)
        throw std::runtime_error("Function not found: " + function);

    using StubFn = int(__stdcall*)(int, void**);
    StubFn fn = reinterpret_cast<StubFn>(proc);

    std::vector<void*> argsPtrs;
    std::vector<std::wstring> wstrings;
    std::vector<intptr_t> ints;

    for (auto& expr : args) {
        Value v = expr->evaluate();
        if (v.type == ValueType::NUMBER) {
            ints.push_back(static_cast<intptr_t>(v.number));
            argsPtrs.push_back(reinterpret_cast<void*>(ints.back()));
        } else if (v.type == ValueType::BOOLEAN) {
            ints.push_back(v.boolean ? 1 : 0);
            argsPtrs.push_back(reinterpret_cast<void*>(ints.back()));
        } else if (v.type == ValueType::STRING) {
            wstrings.push_back(utf8ToUtf16(v.string));
            argsPtrs.push_back((void*)wstrings.back().c_str());
        } else {
            throw std::runtime_error("Unsupported argument type");
        }
    }

    fn(static_cast<int>(argsPtrs.size()), argsPtrs.data());
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
