#include "ValueUtils.hpp"
#include <cctype>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

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

std::string normalizeTypeName(std::string typeName) {
    for (char& ch : typeName) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return typeName;
}

bool endsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}
