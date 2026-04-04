#pragma once
#include <unordered_map>
#include <string>
#include <vector>
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

inline std::string getClibSymbolKey(const std::string& alias, const std::string& function) {
    return alias + "::" + function;
}

inline std::string getQualifiedFunctionName(const std::string& namespaceName, const std::string& functionName) {
    if (namespaceName.empty()) {
        return functionName;
    }
    return namespaceName + "::" + functionName;
}

Value invokeNamespaceFunction(const std::string& namespaceName, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args);

inline void pushScope() { ScopeStack.emplace_back(); }
inline void popScope() { ScopeStack.pop_back(); }

inline Variable* findVariable(const std::string& name) {
    for(auto it = ScopeStack.rbegin(); it != ScopeStack.rend(); ++it){
        auto v = it->find(name);
        if(v != it->end()) return &v->second;
    }
    return nullptr;
}
