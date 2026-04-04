#pragma once
#include <memory>
#include <string>
#include <vector>
#include "AST.hpp"

Value invokeStringFunction(const std::string& function, const std::vector<std::unique_ptr<Expr>>& args);
