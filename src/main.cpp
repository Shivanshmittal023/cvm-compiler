#include "Lexer.h"
#include "Parser.h"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::string source;
    std::string filename;

    if (argc >= 2) {
        // ── Read from file ──────────────────────────────────────
        filename = argv[1];
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "error: could not open file '" << filename << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    } else {
        // ── Built-in demo ───────────────────────────────────────
        filename = "demo.cpp";
        source = R"(
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum = sum + i;
    }
    return 0;
}
)";
        std::cout << "── No input file provided, using built-in demo ──\n\n";
    }

    // ── Phase 1, Step 1: Lexing ─────────────────────────────────
    Lexer lexer(source, filename);
    auto tokens = lexer.tokenize();

    std::cout << "=== TOKENS ===\n";
    for (const auto& tok : tokens) {
        std::cout << "  " << tok.filename << ":" << tok.line << ":" << tok.column
                  << "  " << tokenTypeToString(tok.type)
                  << "  '" << tok.lexeme << "'\n";
    }
    std::cout << "\n";

    // ── Phase 1, Step 3: Parsing ────────────────────────────────
    Parser parser(tokens);
    auto program = parser.parse();

    if (parser.hadError()) {
        std::cerr << "\nParsing failed with errors.\n";
        return 1;
    }

    std::cout << "=== AST ===\n";
    std::cout << program->dump() << std::endl;

    return 0;
}
