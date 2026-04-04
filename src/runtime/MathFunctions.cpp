#include "MathFunctions.hpp"
#include <stdexcept>
#include <string>

namespace {
std::string toUpperAscii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return value;
}

bool valueEquals(const Value& a, const Value& b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case ValueType::NOTHING:
            return true;
        case ValueType::NUMBER:
            return a.number == b.number;
        case ValueType::STRING:
            return a.string == b.string;
        case ValueType::BOOLEAN:
            return a.boolean == b.boolean;
        case ValueType::POINTER:
            return a.pointer == b.pointer;
        case ValueType::ARRAY:
            return a.array == b.array;
        case ValueType::POTENTIAL_BOOLEAN:
            return true;
        default:
            return false;
    }
}
}

Value invokeTestFunction(const std::string& function, const std::vector<std::unique_ptr<Expr>>& args) {
    const std::string normalizedFunction = toUpperAscii(function);

    if (normalizedFunction == "ASSERT_EQ" || normalizedFunction == "ASSERT_EQUAL") {
        if (args.size() != 2) {
            throw std::runtime_error("TESTS::ASSERT_EQ expects 2 arguments: value and expected");
        }

        const Value value = args[0]->evaluate();
        const Value expected = args[1]->evaluate();
        return Value(valueEquals(value, expected));
    }

    throw std::runtime_error("Unknown TESTS native function: " + function);
}
