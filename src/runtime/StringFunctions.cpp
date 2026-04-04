#include "StringFunctions.hpp"
#include "ValueUtils.hpp"
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace {
std::string toUpperAscii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string toLowerAscii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string trimAscii(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

Value requireValue(const std::vector<std::unique_ptr<Expr>>& args, size_t index, const std::string& functionName) {
    if (index >= args.size()) {
        throw std::runtime_error("Missing argument for STRING::" + functionName);
    }
    return args[index]->evaluate();
}

std::string requireString(const std::vector<std::unique_ptr<Expr>>& args, size_t index, const std::string& functionName) {
    const Value value = requireValue(args, index, functionName);
    if (value.type != ValueType::STRING) {
        throw std::runtime_error("Expected string argument for STRING::" + functionName);
    }
    return value.string;
}

double requireNumber(const std::vector<std::unique_ptr<Expr>>& args, size_t index, const std::string& functionName) {
    const Value value = requireValue(args, index, functionName);
    if (value.type != ValueType::NUMBER) {
        throw std::runtime_error("Expected number argument for STRING::" + functionName);
    }
    return value.number;
}

std::shared_ptr<ArrayObject> requireArray(const std::vector<std::unique_ptr<Expr>>& args, size_t index, const std::string& functionName) {
    const Value value = requireValue(args, index, functionName);
    if (value.type != ValueType::ARRAY || !value.array) {
        throw std::runtime_error("Expected array argument for STRING::" + functionName);
    }
    return value.array;
}

bool startsWith(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool endsWithString(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Value makeStringValue(std::string value) {
    Value result;
    result.type = ValueType::STRING;
    result.string = std::move(value);
    return result;
}

Value makeNumberValue(double value) {
    return Value(value);
}

Value makeBooleanValueResult(bool value) {
    return Value(value);
}

Value makeArrayValue(std::shared_ptr<ArrayObject> array) {
    return Value(std::move(array));
}
}

Value invokeStringFunction(const std::string& function, const std::vector<std::unique_ptr<Expr>>& args) {
    const std::string normalizedFunction = toUpperAscii(function);

    if (normalizedFunction == "LEN" || normalizedFunction == "LENGTH") {
        if (args.size() != 1) {
            throw std::runtime_error("STRING::LEN expects 1 argument");
        }
        return makeNumberValue(static_cast<double>(requireString(args, 0, normalizedFunction).size()));
    }

    if (normalizedFunction == "SUBSTR" || normalizedFunction == "SUBSTRING") {
        if (args.size() != 3) {
            throw std::runtime_error("STRING::SUBSTR expects 3 arguments");
        }

        const std::string source = requireString(args, 0, normalizedFunction);
        const double startValue = requireNumber(args, 1, normalizedFunction);
        const double lengthValue = requireNumber(args, 2, normalizedFunction);

        if (std::trunc(startValue) != startValue || std::trunc(lengthValue) != lengthValue) {
            throw std::runtime_error("STRING::SUBSTR expects integer start and length");
        }
        if (startValue < 0 || lengthValue < 0) {
            throw std::runtime_error("STRING::SUBSTR expects non-negative start and length");
        }

        const size_t start = static_cast<size_t>(startValue);
        const size_t length = static_cast<size_t>(lengthValue);
        if (start >= source.size()) {
            return makeStringValue("");
        }
        return makeStringValue(source.substr(start, length));
    }

    if (normalizedFunction == "FIND" || normalizedFunction == "INDEXOF") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::FIND expects 2 arguments");
        }
        const std::string source = requireString(args, 0, normalizedFunction);
        const std::string needle = requireString(args, 1, normalizedFunction);
        const std::size_t position = source.find(needle);
        return makeNumberValue(position == std::string::npos ? -1.0 : static_cast<double>(position));
    }

    if (normalizedFunction == "REPLACE") {
        if (args.size() != 3) {
            throw std::runtime_error("STRING::REPLACE expects 3 arguments");
        }

        std::string source = requireString(args, 0, normalizedFunction);
        const std::string from = requireString(args, 1, normalizedFunction);
        const std::string to = requireString(args, 2, normalizedFunction);

        if (from.empty()) {
            return makeStringValue(source);
        }

        std::string result;
        result.reserve(source.size());
        std::size_t searchPos = 0;
        while (true) {
            std::size_t foundPos = source.find(from, searchPos);
            if (foundPos == std::string::npos) {
                result.append(source, searchPos, std::string::npos);
                break;
            }
            result.append(source, searchPos, foundPos - searchPos);
            result.append(to);
            searchPos = foundPos + from.size();
        }
        return makeStringValue(std::move(result));
    }

    if (normalizedFunction == "TRIM") {
        if (args.size() != 1) {
            throw std::runtime_error("STRING::TRIM expects 1 argument");
        }
        return makeStringValue(trimAscii(requireString(args, 0, normalizedFunction)));
    }

    if (normalizedFunction == "UPPER") {
        if (args.size() != 1) {
            throw std::runtime_error("STRING::UPPER expects 1 argument");
        }
        return makeStringValue(toUpperAscii(requireString(args, 0, normalizedFunction)));
    }

    if (normalizedFunction == "LOWER") {
        if (args.size() != 1) {
            throw std::runtime_error("STRING::LOWER expects 1 argument");
        }
        return makeStringValue(toLowerAscii(requireString(args, 0, normalizedFunction)));
    }

    if (normalizedFunction == "STARTSWITH") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::STARTSWITH expects 2 arguments");
        }
        return makeBooleanValueResult(startsWith(requireString(args, 0, normalizedFunction), requireString(args, 1, normalizedFunction)));
    }

    if (normalizedFunction == "ENDSWITH") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::ENDSWITH expects 2 arguments");
        }
        return makeBooleanValueResult(endsWithString(requireString(args, 0, normalizedFunction), requireString(args, 1, normalizedFunction)));
    }

    if (normalizedFunction == "CONTAINS") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::CONTAINS expects 2 arguments");
        }
        const std::string source = requireString(args, 0, normalizedFunction);
        const std::string needle = requireString(args, 1, normalizedFunction);
        return makeBooleanValueResult(source.find(needle) != std::string::npos);
    }

    if (normalizedFunction == "REPEAT") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::REPEAT expects 2 arguments");
        }

        const std::string source = requireString(args, 0, normalizedFunction);
        const double countValue = requireNumber(args, 1, normalizedFunction);
        if (std::trunc(countValue) != countValue || countValue < 0) {
            throw std::runtime_error("STRING::REPEAT expects a non-negative integer count");
        }

        std::string result;
        result.reserve(source.size() * static_cast<size_t>(countValue));
        for (size_t i = 0; i < static_cast<size_t>(countValue); ++i) {
            result += source;
        }
        return makeStringValue(std::move(result));
    }

    if (normalizedFunction == "JOIN") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::JOIN expects 2 arguments");
        }

        auto array = requireArray(args, 0, normalizedFunction);
        const std::string delimiter = requireString(args, 1, normalizedFunction);
        std::string result;
        for (size_t i = 0; i < array->indexed.size(); ++i) {
            if (i > 0) {
                result += delimiter;
            }
            result += valueToString(array->indexed[i]);
        }
        return makeStringValue(std::move(result));
    }

    if (normalizedFunction == "SPLIT") {
        if (args.size() != 2) {
            throw std::runtime_error("STRING::SPLIT expects 2 arguments");
        }

        const std::string source = requireString(args, 0, normalizedFunction);
        const std::string delimiter = requireString(args, 1, normalizedFunction);
        auto array = std::make_shared<ArrayObject>();

        if (delimiter.empty()) {
            for (char ch : source) {
                array->indexed.emplace_back(std::string(1, ch));
            }
            return makeArrayValue(array);
        }

        size_t start = 0;
        while (true) {
            size_t found = source.find(delimiter, start);
            if (found == std::string::npos) {
                array->indexed.emplace_back(source.substr(start));
                break;
            }
            array->indexed.emplace_back(source.substr(start, found - start));
            start = found + delimiter.size();
        }
        return makeArrayValue(array);
    }

    throw std::runtime_error("Unknown STRING function: " + function);
}
