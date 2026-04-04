#pragma once
#include <memory>
#include <string>
#include <vector>
#include "AST.hpp"

bool tryInvokeNativeNamespaceFunction(
    const std::string& namespaceName,
    const std::string& function,
    const std::vector<std::unique_ptr<Expr>>& args,
    Value& result);
