#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "AST.hpp"

using Scope = std::unordered_map<std::string, Variable>;
inline std::vector<Scope> ScopeStack;

inline void pushScope() { ScopeStack.emplace_back(); }
inline void popScope() { ScopeStack.pop_back(); }

inline Variable* findVariable(const std::string& name) {
    for(auto it = ScopeStack.rbegin(); it != ScopeStack.rend(); ++it){
        auto v = it->find(name);
        if(v != it->end()) return &v->second;
    }
    return nullptr;
}
