#pragma once
#include <string>
#include "AST.hpp"

std::string valueToString(const Value& v);
bool randomBoolean();
Value resolvePotentialBoolean(const Value& v);
std::string normalizeTypeName(std::string typeName);
bool endsWith(const std::string& value, const std::string& suffix);
