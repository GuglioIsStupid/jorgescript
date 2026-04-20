#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <stdexcept>
#include "AST.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using ClibHandle = HMODULE;
using ClibProc = FARPROC;

inline ClibHandle openClib(const std::string& path) {
    return LoadLibraryA(path.c_str());
}

inline ClibProc getClibSymbol(ClibHandle handle, const std::string& symbol) {
    return GetProcAddress(handle, symbol.c_str());
}

inline void closeClib(ClibHandle handle) {
    if (handle) {
        FreeLibrary(handle);
    }
}

inline std::string getClibErrorMessage() {
    return {};
}
#else
#include <dlfcn.h>
using ClibHandle = void*;
using ClibProc = void*;

inline ClibHandle openClib(const std::string& path) {
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

inline ClibProc getClibSymbol(ClibHandle handle, const std::string& symbol) {
    dlerror();
    return dlsym(handle, symbol.c_str());
}

inline void closeClib(ClibHandle handle) {
    if (handle) {
        dlclose(handle);
    }
}

inline std::string getClibErrorMessage() {
    const char* error = dlerror();
    return error ? error : "";
}
#endif

using Scope = std::unordered_map<std::string, Variable>;

inline std::vector<Scope> ScopeStack;
inline std::unordered_map<std::string, Scope> FileScopes;
inline std::unordered_map<std::string, ClibHandle> LoadedCLIBs;
inline std::unordered_map<std::string, std::string> ClibReturnTypes;
inline std::unordered_map<std::string, std::shared_ptr<FunctionDefinition>> UserFunctions;
inline std::vector<std::string> NamespaceStack;
inline std::unordered_set<std::string> DefinedClasses;
inline std::unordered_map<std::string, std::string> ClassParents;
inline std::unordered_map<std::string, std::vector<std::string>> ClassImplements;
inline std::string MainScriptPath;

inline std::string getClibSymbolKey(const std::string& alias, const std::string& function) {
    return alias + "::" + function;
}

inline std::string getQualifiedFunctionName(const std::string& namespaceName, const std::string& functionName) {
    if (namespaceName.empty()) {
        return functionName;
    }
    return namespaceName + "::" + functionName;
}

inline std::string getOperatorOverloadFunctionName(const std::string& op) {
    if (op == "+") return "OPERATOR_PLUS";
    if (op == "-") return "OPERATOR_MINUS";
    if (op == "*") return "OPERATOR_MUL";
    if (op == "/") return "OPERATOR_DIV";
    if (op == "//") return "OPERATOR_FLOORDIV";
    if (op == "%") return "OPERATOR_MOD";
    if (op == "==") return "OPERATOR_EQ";
    if (op == "!=") return "OPERATOR_NEQ";
    if (op == ">") return "OPERATOR_GT";
    if (op == "<") return "OPERATOR_LT";
    if (op == ">=") return "OPERATOR_GTE";
    if (op == "<=") return "OPERATOR_LTE";
    throw std::runtime_error("Unsupported operator overload symbol: " + op);
}

inline bool isOperatorOverloadable(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/" || op == "//" || op == "%" ||
           op == "==" || op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=";
}

inline std::string resolveClassFunctionNamespace(const std::string& className, const std::string& functionName) {
    if (className.empty()) {
        return "";
    }

    std::string current = className;
    std::unordered_set<std::string> visited;
    while (!current.empty() && visited.insert(current).second) {
        const std::string qualified = getQualifiedFunctionName(current, functionName);
        if (UserFunctions.find(qualified) != UserFunctions.end()) {
            return current;
        }

        auto parentIt = ClassParents.find(current);
        if (parentIt == ClassParents.end()) {
            break;
        }
        current = parentIt->second;
    }

    return "";
}

inline bool isClassOrDerivedFrom(const std::string& className, const std::string& targetClass) {
    if (className.empty() || targetClass.empty()) {
        return false;
    }

    std::string current = className;
    std::unordered_set<std::string> visited;
    while (!current.empty() && visited.insert(current).second) {
        if (current == targetClass) {
            return true;
        }

        auto parentIt = ClassParents.find(current);
        if (parentIt == ClassParents.end()) {
            break;
        }
        current = parentIt->second;
    }

    return false;
}

Value invokeNamespaceFunction(const std::string& namespaceName, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args);

inline std::string getCurrentScriptPath() {
    return MainScriptPath;
}

inline void pushCurrentScriptPath(std::string path) {
    MainScriptPath = std::move(path);
}

inline void popCurrentScriptPath() {
    MainScriptPath.clear();
}

inline void pushScope() { ScopeStack.emplace_back(); }
inline void popScope() { ScopeStack.pop_back(); }

inline Variable* findVariable(const std::string& name) {
    for(auto it = ScopeStack.rbegin(); it != ScopeStack.rend(); ++it){
        auto v = it->find(name);
        if(v != it->end()) return &v->second;
    }
    return nullptr;
}
