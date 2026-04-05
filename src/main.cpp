#include "Lexer.hpp"
#include "Parser.hpp"
#include "Runtime.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: jorgescript <file.jorge>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    try {
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parseProgram();

        std::cout << "Running JorgeScript\n";
        ScopeStack.clear();
        MainScriptPath.clear();
        pushScope();

        std::error_code absolutePathError;
        const std::filesystem::path mainScriptPath = std::filesystem::absolute(std::filesystem::path(argv[1]), absolutePathError);
        if (absolutePathError) {
            pushCurrentScriptPath(argv[1]);
        } else {
            pushCurrentScriptPath(mainScriptPath.string());
        }

        for (auto& stmt : program)
            stmt->execute();

        popCurrentScriptPath();

    } catch (const std::exception& e) {
        std::cerr << "JorgeScript Error: " << e.what() << '\n';
    }
}
