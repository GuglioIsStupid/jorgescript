#include "IOFunctions.hpp"
#include "Runtime.hpp"
#include "ValueUtils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

Value evaluateArgument(const std::vector<std::unique_ptr<Expr>>& args, size_t index, const std::string& functionName) {
    if (index >= args.size()) {
        throw std::runtime_error("Missing argument for IO::" + functionName);
    }
    return args[index]->evaluate();
}

std::string requireString(const std::vector<std::unique_ptr<Expr>>& args, size_t index, const std::string& functionName) {
    const Value value = evaluateArgument(args, index, functionName);
    if (value.type != ValueType::STRING) {
        throw std::runtime_error("Expected string argument for IO::" + functionName);
    }
    return value.string;
}

std::filesystem::path getScriptBaseDirectory() {
    const std::string currentScript = getCurrentScriptPath();
    if (currentScript.empty()) {
        return std::filesystem::current_path();
    }

    std::error_code pathError;
    std::filesystem::path absolutePath = std::filesystem::absolute(std::filesystem::path(currentScript), pathError);
    if (pathError) {
        return std::filesystem::current_path();
    }

    if (absolutePath.has_parent_path()) {
        return absolutePath.parent_path();
    }

    return std::filesystem::current_path();
}

std::filesystem::path resolvePathFromScript(const std::string& inputPath) {
    const std::filesystem::path asPath(inputPath);
    if (asPath.is_absolute()) {
        return asPath;
    }
    return getScriptBaseDirectory() / asPath;
}

Value makeBoolean(bool value) {
    return Value(value);
}

Value makeString(std::string value) {
    return Value(std::move(value));
}

Value makeNumber(double value) {
    return Value(value);
}
}

bool tryInvokeIoNamespaceFunction(
    const std::string& function,
    const std::vector<std::unique_ptr<Expr>>& args,
    Value& result) {
    const std::string normalizedFunction = toUpperAscii(function);

    if (normalizedFunction == "GET_CURRENT_PATH") {
        if (!args.empty()) {
            throw std::runtime_error("IO::GET_CURRENT_PATH expects 0 arguments");
        }

        std::error_code pathError;
        const std::filesystem::path currentPath = std::filesystem::absolute(std::filesystem::path(getCurrentScriptPath()), pathError);
        if (pathError) {
            result = makeString(getCurrentScriptPath());
        } else {
            result = makeString(currentPath.string());
        }
        return true;
    }

    if (normalizedFunction == "INPUT" || normalizedFunction == "PROMPT" || normalizedFunction == "READ_LINE") {
        if (args.size() > 1) {
            throw std::runtime_error("IO::INPUT expects 0 or 1 argument");
        }

        if (args.size() == 1) {
            std::cout << requireString(args, 0, normalizedFunction);
            std::cout.flush();
        }

        std::string input;
        std::getline(std::cin, input);
        result = makeString(input);
        return true;
    }

    if (normalizedFunction == "READ_FILE") {
        if (args.size() != 1) {
            throw std::runtime_error("IO::READ_FILE expects 1 argument");
        }

        const std::filesystem::path path = resolvePathFromScript(requireString(args, 0, normalizedFunction));
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to read file: " + path.string());
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        result = makeString(contents.str());
        return true;
    }

    if (normalizedFunction == "WRITE_FILE") {
        if (args.size() != 2) {
            throw std::runtime_error("IO::WRITE_FILE expects 2 arguments: path and content");
        }

        const std::filesystem::path path = resolvePathFromScript(requireString(args, 0, normalizedFunction));
        const std::string content = requireString(args, 1, normalizedFunction);

        std::error_code createDirError;
        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, createDirError);
            if (createDirError) {
                throw std::runtime_error("Failed to create directories for file: " + path.string());
            }
        }

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) {
            throw std::runtime_error("Failed to open file for writing: " + path.string());
        }

        file << content;
        if (!file.good()) {
            throw std::runtime_error("Failed while writing file: " + path.string());
        }

        result = makeBoolean(true);
        return true;
    }

    if (normalizedFunction == "APPEND_FILE") {
        if (args.size() != 2) {
            throw std::runtime_error("IO::APPEND_FILE expects 2 arguments: path and content");
        }

        const std::filesystem::path path = resolvePathFromScript(requireString(args, 0, normalizedFunction));
        const std::string content = requireString(args, 1, normalizedFunction);

        std::error_code createDirError;
        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, createDirError);
            if (createDirError) {
                throw std::runtime_error("Failed to create directories for file: " + path.string());
            }
        }

        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file) {
            throw std::runtime_error("Failed to open file for append: " + path.string());
        }

        file << content;
        if (!file.good()) {
            throw std::runtime_error("Failed while appending file: " + path.string());
        }

        result = makeBoolean(true);
        return true;
    }

    if (normalizedFunction == "EXISTS") {
        if (args.size() != 1) {
            throw std::runtime_error("IO::EXISTS expects 1 argument");
        }

        const std::filesystem::path path = resolvePathFromScript(requireString(args, 0, normalizedFunction));
        std::error_code existsError;
        const bool exists = std::filesystem::exists(path, existsError);
        if (existsError) {
            throw std::runtime_error("Failed to check path existence: " + path.string());
        }

        result = makeBoolean(exists);
        return true;
    }

    if (normalizedFunction == "GET_CWD") {
        if (!args.empty()) {
            throw std::runtime_error("IO::GET_CWD expects 0 arguments");
        }

        std::error_code cwdError;
        const std::filesystem::path cwd = std::filesystem::current_path(cwdError);
        if (cwdError) {
            throw std::runtime_error("Failed to get current working directory");
        }

        result = makeString(cwd.string());
        return true;
    }

    if (normalizedFunction == "FILE_SIZE") {
        if (args.size() != 1) {
            throw std::runtime_error("IO::FILE_SIZE expects 1 argument");
        }

        const std::filesystem::path path = resolvePathFromScript(requireString(args, 0, normalizedFunction));
        std::error_code sizeError;
        const auto size = std::filesystem::file_size(path, sizeError);
        if (sizeError) {
            throw std::runtime_error("Failed to get file size: " + path.string());
        }

        result = makeNumber(static_cast<double>(size));
        return true;
    }

    return false;
}
