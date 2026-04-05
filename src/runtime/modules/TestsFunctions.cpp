#include "TestsFunctions.hpp"
#include "IOFunctions.hpp"
#include "StringFunctions.hpp"
#include <iostream>
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

bool tryInvokeNativeNamespaceFunction(
    const std::string& namespaceName,
    const std::string& function,
    const std::vector<std::unique_ptr<Expr>>& args,
    Value& result) {
    if (namespaceName == "STRING") {
        result = invokeStringFunction(function, args);
        return true;
    }

    if (namespaceName == "IO") {
        return tryInvokeIoNamespaceFunction(function, args, result);
    }

    if (namespaceName != "TESTS") {
        return false;
    }

    const std::string normalizedFunction = toUpperAscii(function);
    if (normalizedFunction == "ASSERT_EQ" || normalizedFunction == "ASSERT_EQUAL") {
        if (args.size() != 2 && args.size() != 3) {
            throw std::runtime_error("TESTS::ASSERT_EQ expects 2 or 3 arguments: value, expected, [name]");
        }

        const Value value = args[0]->evaluate();
        const Value expected = args[1]->evaluate();
        const bool passed = valueEquals(value, expected);

        if (args.size() == 3) {
            const Value testName = args[2]->evaluate();
            if (testName.type != ValueType::STRING) {
                throw std::runtime_error("TESTS::ASSERT_EQ optional name must be a string");
            }

            std::cout << testName.string << ": " << (passed ? "PASSED" : "FAILED") << "\n";
        }

        result = Value(passed);
        return true;
    }

    throw std::runtime_error("Unknown TESTS native function: " + function);
}
