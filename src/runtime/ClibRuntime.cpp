#include "ClibRuntime.hpp"
#include "ValueUtils.hpp"
#include <stdexcept>
#include <cstdint>
#include <string>
#include <vector>

namespace {
#if defined(_WIN32)
std::wstring utf8ToUtf16(const std::string& s) {
    if (s.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &result[0], size);
    result.pop_back();
    return result;
}
#endif

void addClibCandidate(std::vector<std::string>& candidates, const std::string& candidate) {
    for (const auto& existing : candidates) {
        if (existing == candidate) {
            return;
        }
    }
    candidates.push_back(candidate);
}

std::vector<std::string> buildClibCandidates(const std::string& name) {
    std::vector<std::string> candidates;
    addClibCandidate(candidates, name);

#if defined(_WIN32)
    if (!endsWith(name, ".dll")) {
        addClibCandidate(candidates, name + ".dll");
    }
#else
    if (!endsWith(name, ".so")) {
        addClibCandidate(candidates, name + ".so");
    }

    if (!endsWith(name, ".dll")) {
        addClibCandidate(candidates, name + ".dll");
    }

    const std::string baseName = endsWith(name, ".dll") ? name.substr(0, name.size() - 4) : name;
    if (!baseName.empty()) {
        if (!endsWith(baseName, ".so")) {
            addClibCandidate(candidates, baseName + ".so");
        }
        if (!endsWith(baseName, ".so") && !endsWith(baseName, ".dll") && baseName.rfind("lib", 0) != 0) {
            addClibCandidate(candidates, "lib" + baseName + ".so");
        }
    }
#endif

    return candidates;
}

std::uintptr_t callRawProc(ClibProc proc, const std::vector<std::uintptr_t>& args) {
    switch (args.size()) {
        case 0: {
            using Fn = std::uintptr_t(*)();
            return reinterpret_cast<Fn>(proc)();
        }
        case 1: {
            using Fn = std::uintptr_t(*)(std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0]);
        }
        case 2: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1]);
        }
        case 3: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2]);
        }
        case 4: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3]);
        }
        case 5: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4]);
        }
        case 6: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4], args[5]);
        }
        case 7: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        }
        case 8: {
            using Fn = std::uintptr_t(*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
            return reinterpret_cast<Fn>(proc)(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        }
        default:
            throw std::runtime_error("Too many CLIB arguments (max 8)");
    }
}
}

ClibHandle loadClibHandle(const std::string& name) {
    std::string lastError;
    for (const auto& candidate : buildClibCandidates(name)) {
        ClibHandle handle = openClib(candidate);
        if (handle) {
            return handle;
        }

        const std::string errorMessage = getClibErrorMessage();
        if (!errorMessage.empty()) {
            lastError = errorMessage;
        }
    }

    if (!lastError.empty()) {
        throw std::runtime_error("Failed to load CLIB: " + name + " (" + lastError + ")");
    }

    throw std::runtime_error("Failed to load CLIB: " + name);
}

Value invokeClibFunction(const std::string& alias, const std::string& function, const std::vector<std::unique_ptr<Expr>>& args) {
    auto it = LoadedCLIBs.find(alias);
    if (it == LoadedCLIBs.end()) {
        throw std::runtime_error("CLIB not loaded: " + alias);
    }

    ClibProc proc = getClibSymbol(it->second, function);
    if (!proc) {
        throw std::runtime_error("Function not found: " + function);
    }

    std::vector<std::string> utf8Strings;
    std::vector<std::wstring> utf16Strings;
    std::vector<std::uintptr_t> rawArgs;
    rawArgs.reserve(args.size());

    const bool wantsWideStrings = !function.empty() && function.back() == 'W';

    for (const auto& expr : args) {
        Value v = expr->evaluate();
        if (v.type == ValueType::NUMBER) {
            rawArgs.push_back(static_cast<std::uintptr_t>(static_cast<std::intptr_t>(v.number)));
        } else if (v.type == ValueType::BOOLEAN) {
            rawArgs.push_back(v.boolean ? 1u : 0u);
        } else if (v.type == ValueType::STRING) {
#if defined(_WIN32)
            if (wantsWideStrings) {
                utf16Strings.push_back(utf8ToUtf16(v.string));
                rawArgs.push_back(reinterpret_cast<std::uintptr_t>(utf16Strings.back().c_str()));
            } else {
                utf8Strings.push_back(v.string);
                rawArgs.push_back(reinterpret_cast<std::uintptr_t>(utf8Strings.back().c_str()));
            }
#else
            (void)wantsWideStrings;
            utf8Strings.push_back(v.string);
            rawArgs.push_back(reinterpret_cast<std::uintptr_t>(utf8Strings.back().c_str()));
#endif
        } else if (v.type == ValueType::POINTER) {
            rawArgs.push_back(v.pointer);
        } else if (v.type == ValueType::NOTHING) {
            rawArgs.push_back(0);
        } else {
            throw std::runtime_error("Unsupported CLIB argument type");
        }
    }

    const std::string key = getClibSymbolKey(alias, function);
    auto retIt = ClibReturnTypes.find(key);
    const std::string returnType = (retIt != ClibReturnTypes.end()) ? retIt->second : "POINTER";
    const std::string normalizedType = normalizeTypeName(returnType);

    std::uintptr_t result = callRawProc(proc, rawArgs);

    if (normalizedType == "BOOLEAN" || normalizedType == "BOOL") {
        return Value(result != 0);
    }

    if (normalizedType == "NUMBER" || normalizedType == "INT" || normalizedType == "INT32" || normalizedType == "UINT32") {
        return Value(static_cast<double>(static_cast<std::int64_t>(result)));
    }

    if (result == 0) {
        return Value();
    }

    return Value(result, returnType);
}
