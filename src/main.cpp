#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"
#include "VM.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ════════════════════════════════════════════════════════════════
// CLI flags
// ════════════════════════════════════════════════════════════════
struct Options {
    std::string inputFile;
    bool showTokens   = false;
    bool showAST      = false;
    bool showBytecode = false;
    bool traceExec    = false;
    bool replMode     = false;
    bool helpMode     = false;
};

static void printUsage() {
    std::cout <<
R"(
  ╔═══════════════════════════════════════════════════════════╗
  ║              CVM++ — Mini-C++ Virtual Machine            ║
  ╚═══════════════════════════════════════════════════════════╝

  USAGE:
    mycc [options] <source-file.cpp>
    mycc --repl

  OPTIONS:
    --help        Show this help message
    --tokens      Print the token stream
    --ast         Print the Abstract Syntax Tree
    --bytecode    Print the compiled bytecode for each function
    --trace       Trace VM execution (print stack + instructions)
    --repl        Start interactive REPL mode
    --all         Enable --tokens --ast --bytecode

  EXAMPLES:
    mycc program.cpp                  Compile & run
    mycc --bytecode program.cpp       Show bytecode then run
    mycc --all program.cpp            Show everything
    mycc --repl                       Interactive mode

)";
}

static Options parseArgs(int argc, char* argv[]) {
    Options opts;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")       opts.helpMode = true;
        else if (arg == "--tokens")               opts.showTokens = true;
        else if (arg == "--ast")                  opts.showAST = true;
        else if (arg == "--bytecode")             opts.showBytecode = true;
        else if (arg == "--trace")                opts.traceExec = true;
        else if (arg == "--repl")                 opts.replMode = true;
        else if (arg == "--all") {
            opts.showTokens = true;
            opts.showAST = true;
            opts.showBytecode = true;
        }
        else if (arg[0] == '-') {
            std::cerr << "unknown option: " << arg << "\n";
            opts.helpMode = true;
        }
        else {
            opts.inputFile = arg;
        }
    }
    return opts;
}

// ════════════════════════════════════════════════════════════════
// Pipeline: Source → Tokens → AST → Bytecode → VM
// ════════════════════════════════════════════════════════════════
static int runPipeline(const std::string& source,
                       const std::string& filename,
                       const Options& opts) {
    // ── Phase 1: Lexing ─────────────────────────────────────
    Lexer lexer(source, filename);
    auto tokens = lexer.tokenize();

    if (opts.showTokens) {
        std::cout << "=== TOKENS ===\n";
        for (const auto& tok : tokens) {
            std::cout << "  " << tok.filename << ":" << tok.line
                      << ":" << tok.column
                      << "  " << tokenTypeToString(tok.type)
                      << "  '" << tok.lexeme << "'\n";
        }
        std::cout << "\n";
    }

    // ── Phase 2: Parsing ────────────────────────────────────
    Parser parser(tokens);
    auto program = parser.parse();

    if (parser.hadError()) {
        std::cerr << "error: parsing failed.\n";
        return 1;
    }

    if (opts.showAST) {
        std::cout << "=== AST ===\n";
        std::cout << program->dump() << "\n\n";
    }

    // ── Phase 3: Compilation (AST → Bytecode) ──────────────
    Compiler compiler;
    if (!compiler.compile(*program)) {
        std::cerr << "error: compilation failed.\n";
        return 1;
    }

    if (opts.showBytecode) {
        std::cout << "=== COMPILED BYTECODE ===\n";
        for (auto& [name, fn] : compiler.getAllFunctions()) {
            std::cout << "[function '" << name
                      << "', arity=" << fn->arity << "]\n";
            fn->chunk.disassemble();
        }
    }

    // ── Phase 4: Execution (VM) ────────────────────────────
    if (!compiler.getFunction("main")) {
        std::cerr << "error: no 'main' function found.\n";
        return 1;
    }

    VM vm;
    vm.loadFunctions(compiler);
    vm.setDebugTrace(opts.traceExec);

    InterpretResult result = vm.run("main");

    if (result == InterpretResult::OK) {
        if (opts.showBytecode || opts.showAST || opts.showTokens) {
            std::cout << "--- exit: "
                      << valueToString(vm.getReturnValue()) << " ---\n";
        }
        return 0;
    } else {
        std::cerr << "error: runtime error.\n";
        return 1;
    }
}

// ════════════════════════════════════════════════════════════════
// REPL mode
// ════════════════════════════════════════════════════════════════
static void runREPL() {
    std::cout <<
R"(
  ╔═══════════════════════════════════════════════════════════╗
  ║            CVM++ Interactive REPL                        ║
  ║  Enter a complete Mini-C++ program, then press Enter     ║
  ║  on a blank line to compile & run.                       ║
  ║  Type 'quit' or 'exit' to leave.                         ║
  ╚═══════════════════════════════════════════════════════════╝

)";

    Options opts;
    opts.showBytecode = true;

    int session = 0;
    while (true) {
        session++;
        std::cout << "repl[" << session << "]> ";
        std::cout.flush();

        std::string source;
        std::string line;
        bool firstLine = true;

        while (std::getline(std::cin, line)) {
            if (firstLine && (line == "quit" || line == "exit")) {
                std::cout << "Bye!\n";
                return;
            }
            firstLine = false;

            if (line.empty()) break;  // blank line = submit

            source += line + "\n";
            std::cout << "     > ";
            std::cout.flush();
        }

        if (std::cin.eof()) {
            std::cout << "\nBye!\n";
            return;
        }

        if (source.empty()) continue;

        std::cout << "\n";
        runPipeline(source, "<repl>", opts);
        std::cout << "\n";
    }
}

// ════════════════════════════════════════════════════════════════
// MAIN
// ════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    Options opts = parseArgs(argc, argv);

    if (opts.helpMode) {
        printUsage();
        return 0;
    }

    if (opts.replMode) {
        runREPL();
        return 0;
    }

    std::string source;
    std::string filename;

    if (!opts.inputFile.empty()) {
        // Read from file
        filename = opts.inputFile;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "error: could not open file '"
                      << filename << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    } else {
        // Built-in demo
        filename = "demo.cpp";
        source = R"(
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    print(factorial(5));
    print(factorial(10));

    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum = sum + i;
    }
    print(sum);

    return 0;
}
)";
        std::cout << "── No input file. Running built-in demo ──\n";
        std::cout << "── Use 'mycc --help' for options ──\n\n";
        opts.showBytecode = true;
    }

    return runPipeline(source, filename, opts);
}
