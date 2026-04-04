#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "AST.hpp"
#include "Runtime.hpp"

ClibHandle loadClibHandle(const std::string& name);
Value invokeClibFunction(const std::string& alias, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args);
