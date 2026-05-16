// ────────────────────────────────────────────────────────────────
// test_compiler.cpp — Tests for the Bytecode Compiler
// ────────────────────────────────────────────────────────────────

#include "Compiler.h"
#include "Lexer.h"
#include "Parser.h"
#include <cassert>
#include <iostream>
#include <string>

static int testsPassed = 0;
static int testsFailed = 0;

static void pass(const char* name) {
    std::cout << "  ✅ " << name << "\n";
    testsPassed++;
}
static void fail(const char* name, const std::string& msg) {
    std::cout << "  ❌ " << name << ": " << msg << "\n";
    testsFailed++;
}

// ── Helper: compile source code string, return success ──────────
static bool compileSource(const std::string& src, Compiler& compiler) {
    Lexer lexer(src, "<test>");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parse();
    if (parser.hadError()) return false;
    return compiler.compile(*program);
}

// Helper: get a specific opcode from a chunk at a given instruction index
// (skipping operand bytes). Returns the OpCode at the nth instruction.
static OpCode getOp(const Chunk& chunk, size_t byteOffset) {
    return static_cast<OpCode>(chunk.code[byteOffset]);
}

// ════════════════════════════════════════════════════════════════
// Test 1: Simple function compiles
// ════════════════════════════════════════════════════════════════
void testSimpleFunction() {
    const char* name = "Simple function compiles";
    Compiler c;
    bool ok = compileSource("int main() { return 0; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");
    if (!fn) { fail(name, "main not found"); return; }
    if (fn->arity != 0) { fail(name, "arity != 0"); return; }
    if (fn->chunk.code.empty()) { fail(name, "empty bytecode"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 2: Function with parameters
// ════════════════════════════════════════════════════════════════
void testFunctionParams() {
    const char* name = "Function with parameters";
    Compiler c;
    bool ok = compileSource("int add(int a, int b) { return a + b; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("add");
    if (!fn) { fail(name, "add not found"); return; }
    if (fn->arity != 2) { fail(name, "arity != 2"); return; }

    // Expected bytecode: GET_LOCAL(0), GET_LOCAL(1), ADD, RETURN, ...
    auto& code = fn->chunk.code;
    if (getOp(fn->chunk, 0) != OpCode::OP_GET_LOCAL) { fail(name, "expected GET_LOCAL(a)"); return; }
    if (code[1] != 0) { fail(name, "param a should be slot 0"); return; }
    if (getOp(fn->chunk, 2) != OpCode::OP_GET_LOCAL) { fail(name, "expected GET_LOCAL(b)"); return; }
    if (code[3] != 1) { fail(name, "param b should be slot 1"); return; }
    if (getOp(fn->chunk, 4) != OpCode::OP_ADD) { fail(name, "expected ADD"); return; }
    if (getOp(fn->chunk, 5) != OpCode::OP_RETURN) { fail(name, "expected RETURN"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 3: Variable declaration with initializer
// ════════════════════════════════════════════════════════════════
void testVarDecl() {
    const char* name = "Variable declaration with init";
    Compiler c;
    bool ok = compileSource("int main() { int x = 42; return x; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");
    if (!fn) { fail(name, "main not found"); return; }

    // Expected: OP_CONSTANT(42), then later GET_LOCAL(0), RETURN
    if (getOp(fn->chunk, 0) != OpCode::OP_CONSTANT) { fail(name, "expected CONSTANT"); return; }
    // The constant value should be 42
    uint8_t constIdx = fn->chunk.code[1];
    auto& val = fn->chunk.constants[constIdx];
    if (!std::holds_alternative<int64_t>(val) || std::get<int64_t>(val) != 42) {
        fail(name, "constant != 42"); return;
    }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 4: Variable without initializer gets default value
// ════════════════════════════════════════════════════════════════
void testVarDeclDefault() {
    const char* name = "Variable without init gets default 0";
    Compiler c;
    bool ok = compileSource("int main() { int x; return x; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");
    if (!fn) { fail(name, "main not found"); return; }

    // First instruction should push default 0
    if (getOp(fn->chunk, 0) != OpCode::OP_CONSTANT) { fail(name, "expected CONSTANT"); return; }
    uint8_t idx = fn->chunk.code[1];
    if (!std::holds_alternative<int64_t>(fn->chunk.constants[idx]) ||
        std::get<int64_t>(fn->chunk.constants[idx]) != 0) {
        fail(name, "default != 0"); return;
    }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 5: Bool default
// ════════════════════════════════════════════════════════════════
void testBoolDefault() {
    const char* name = "Bool variable defaults to false";
    Compiler c;
    bool ok = compileSource("int main() { bool f; return 0; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");
    if (!fn) { fail(name, "main not found"); return; }
    // First instruction should be OP_FALSE
    if (getOp(fn->chunk, 0) != OpCode::OP_FALSE) { fail(name, "expected OP_FALSE"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 6: Arithmetic operators
// ════════════════════════════════════════════════════════════════
void testArithmetic() {
    const char* name = "Arithmetic operators emit correct opcodes";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int a = 10;
            int b = 3;
            int c = a + b;
            int d = a - b;
            int e = a * b;
            int f = a / b;
            int g = a % b;
            return 0;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }

    auto* fn = c.getFunction("main");
    if (!fn) { fail(name, "main not found"); return; }

    // Check that ADD, SUB, MUL, DIV, MOD all appear in the bytecode
    bool hasAdd = false, hasSub = false, hasMul = false, hasDiv = false, hasMod = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_ADD) hasAdd = true;
        if (op == OpCode::OP_SUB) hasSub = true;
        if (op == OpCode::OP_MUL) hasMul = true;
        if (op == OpCode::OP_DIV) hasDiv = true;
        if (op == OpCode::OP_MOD) hasMod = true;
    }
    if (!hasAdd) { fail(name, "missing OP_ADD"); return; }
    if (!hasSub) { fail(name, "missing OP_SUB"); return; }
    if (!hasMul) { fail(name, "missing OP_MUL"); return; }
    if (!hasDiv) { fail(name, "missing OP_DIV"); return; }
    if (!hasMod) { fail(name, "missing OP_MOD"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 7: Comparison operators
// ════════════════════════════════════════════════════════════════
void testComparisons() {
    const char* name = "Comparison operators emit correct opcodes";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int a = 1; int b = 2;
            bool c1 = a == b;
            bool c2 = a != b;
            bool c3 = a < b;
            bool c4 = a <= b;
            bool c5 = a > b;
            bool c6 = a >= b;
            return 0;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasEq = false, hasNeq = false, hasLt = false, hasLe = false, hasGt = false, hasGe = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_EQUAL) hasEq = true;
        if (op == OpCode::OP_NOT_EQUAL) hasNeq = true;
        if (op == OpCode::OP_LESS) hasLt = true;
        if (op == OpCode::OP_LESS_EQUAL) hasLe = true;
        if (op == OpCode::OP_GREATER) hasGt = true;
        if (op == OpCode::OP_GREATER_EQUAL) hasGe = true;
    }
    if (!hasEq)  { fail(name, "missing OP_EQUAL"); return; }
    if (!hasNeq) { fail(name, "missing OP_NOT_EQUAL"); return; }
    if (!hasLt)  { fail(name, "missing OP_LESS"); return; }
    if (!hasLe)  { fail(name, "missing OP_LESS_EQUAL"); return; }
    if (!hasGt)  { fail(name, "missing OP_GREATER"); return; }
    if (!hasGe)  { fail(name, "missing OP_GREATER_EQUAL"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 8: If/else emits correct jump structure
// ════════════════════════════════════════════════════════════════
void testIfElse() {
    const char* name = "If/else emits JUMP_IF_FALSE + JUMP";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int x = 5;
            if (x < 10) {
                x = 1;
            } else {
                x = 2;
            }
            return x;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasJIF = false, hasJump = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_JUMP_IF_FALSE) hasJIF = true;
        if (op == OpCode::OP_JUMP) hasJump = true;
    }
    if (!hasJIF)  { fail(name, "missing JUMP_IF_FALSE"); return; }
    if (!hasJump) { fail(name, "missing JUMP (skip else)"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 9: While loop emits LOOP opcode
// ════════════════════════════════════════════════════════════════
void testWhileLoop() {
    const char* name = "While loop emits JUMP_IF_FALSE + LOOP";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int i = 0;
            while (i < 5) {
                i = i + 1;
            }
            return i;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasJIF = false, hasLoop = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_JUMP_IF_FALSE) hasJIF = true;
        if (op == OpCode::OP_LOOP) hasLoop = true;
    }
    if (!hasJIF)  { fail(name, "missing JUMP_IF_FALSE"); return; }
    if (!hasLoop) { fail(name, "missing OP_LOOP"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 10: For loop emits LOOP opcode
// ════════════════════════════════════════════════════════════════
void testForLoop() {
    const char* name = "For loop emits LOOP + increment";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int sum = 0;
            for (int i = 0; i < 10; i++) {
                sum = sum + i;
            }
            return sum;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasLoop = false, hasAdd = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_LOOP) hasLoop = true;
        if (op == OpCode::OP_ADD) hasAdd = true;
    }
    if (!hasLoop) { fail(name, "missing OP_LOOP"); return; }
    if (!hasAdd)  { fail(name, "missing OP_ADD"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 11: Function call emits OP_CALL with correct args
// ════════════════════════════════════════════════════════════════
void testFunctionCall() {
    const char* name = "Function call emits OP_CALL";
    Compiler c;
    bool ok = compileSource(R"(
        int add(int a, int b) { return a + b; }
        int main() { int r = add(3, 7); return r; }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }

    auto* mainFn = c.getFunction("main");
    if (!mainFn) { fail(name, "main not found"); return; }

    // Find OP_CALL and check its operands
    bool foundCall = false;
    for (size_t i = 0; i < mainFn->chunk.code.size(); i++) {
        if (static_cast<OpCode>(mainFn->chunk.code[i]) == OpCode::OP_CALL) {
            foundCall = true;
            uint8_t argCount = mainFn->chunk.code[i + 1];
            uint8_t nameIdx = mainFn->chunk.code[i + 2];
            if (argCount != 2) { fail(name, "argCount != 2"); return; }
            auto& nameVal = mainFn->chunk.constants[nameIdx];
            if (!std::holds_alternative<std::string>(nameVal) ||
                std::get<std::string>(nameVal) != "add") {
                fail(name, "call target != 'add'"); return;
            }
            break;
        }
    }
    if (!foundCall) { fail(name, "OP_CALL not found"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 12: Recursive function call
// ════════════════════════════════════════════════════════════════
void testRecursion() {
    const char* name = "Recursive function compiles correctly";
    Compiler c;
    bool ok = compileSource(R"(
        int fact(int n) {
            if (n <= 1) { return 1; }
            return n * fact(n - 1);
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("fact");
    if (!fn) { fail(name, "fact not found"); return; }
    if (fn->arity != 1) { fail(name, "arity != 1"); return; }

    // Should contain OP_CALL (recursive) and OP_MUL
    bool hasCall = false, hasMul = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_CALL) hasCall = true;
        if (op == OpCode::OP_MUL) hasMul = true;
    }
    if (!hasCall) { fail(name, "missing recursive OP_CALL"); return; }
    if (!hasMul)  { fail(name, "missing OP_MUL"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 13: Assignment expression
// ════════════════════════════════════════════════════════════════
void testAssignment() {
    const char* name = "Assignment emits SET_LOCAL";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int x = 0;
            x = 42;
            return x;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasSet = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        if (static_cast<OpCode>(fn->chunk.code[i]) == OpCode::OP_SET_LOCAL) {
            hasSet = true;
            break;
        }
    }
    if (!hasSet) { fail(name, "missing SET_LOCAL"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 14: Boolean literals
// ════════════════════════════════════════════════════════════════
void testBoolLiterals() {
    const char* name = "Bool literals emit OP_TRUE / OP_FALSE";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            bool a = true;
            bool b = false;
            return 0;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasTrue = false, hasFalse = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        auto op = static_cast<OpCode>(fn->chunk.code[i]);
        if (op == OpCode::OP_TRUE) hasTrue = true;
        if (op == OpCode::OP_FALSE) hasFalse = true;
    }
    if (!hasTrue)  { fail(name, "missing OP_TRUE"); return; }
    if (!hasFalse) { fail(name, "missing OP_FALSE"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 15: Unary negate
// ════════════════════════════════════════════════════════════════
void testUnaryNegate() {
    const char* name = "Unary negate emits OP_NEGATE";
    Compiler c;
    bool ok = compileSource("int main() { int x = -5; return x; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasNeg = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        if (static_cast<OpCode>(fn->chunk.code[i]) == OpCode::OP_NEGATE) {
            hasNeg = true; break;
        }
    }
    if (!hasNeg) { fail(name, "missing OP_NEGATE"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 16: Unary not
// ════════════════════════════════════════════════════════════════
void testUnaryNot() {
    const char* name = "Unary ! emits OP_NOT";
    Compiler c;
    bool ok = compileSource("int main() { bool x = !true; return 0; }", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    bool hasNot = false;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        if (static_cast<OpCode>(fn->chunk.code[i]) == OpCode::OP_NOT) {
            hasNot = true; break;
        }
    }
    if (!hasNot) { fail(name, "missing OP_NOT"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 17: Undefined variable reports error
// ════════════════════════════════════════════════════════════════
void testUndefinedVar() {
    const char* name = "Undefined variable reports compile error";
    Compiler c;
    bool ok = compileSource("int main() { return y; }", c);
    if (ok) { fail(name, "should have failed but succeeded"); return; }
    if (!c.hadError()) { fail(name, "hadError should be true"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 18: Multiple functions compile
// ════════════════════════════════════════════════════════════════
void testMultipleFunctions() {
    const char* name = "Multiple functions all compile";
    Compiler c;
    bool ok = compileSource(R"(
        int foo() { return 1; }
        int bar(int x) { return x + 1; }
        int main() { return foo(); }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    if (!c.getFunction("foo"))  { fail(name, "foo not found"); return; }
    if (!c.getFunction("bar"))  { fail(name, "bar not found"); return; }
    if (!c.getFunction("main")) { fail(name, "main not found"); return; }
    if (c.getFunction("foo")->arity != 0) { fail(name, "foo arity"); return; }
    if (c.getFunction("bar")->arity != 1) { fail(name, "bar arity"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 19: Nested scopes — inner vars cleaned up
// ════════════════════════════════════════════════════════════════
void testNestedScopes() {
    const char* name = "Nested scopes emit OP_POP for cleanup";
    Compiler c;
    bool ok = compileSource(R"(
        int main() {
            int x = 1;
            {
                int y = 2;
            }
            return x;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }
    auto* fn = c.getFunction("main");

    // The inner block should push y then POP it at end of scope
    int popCount = 0;
    for (size_t i = 0; i < fn->chunk.code.size(); i++) {
        if (static_cast<OpCode>(fn->chunk.code[i]) == OpCode::OP_POP)
            popCount++;
    }
    if (popCount < 1) { fail(name, "expected at least 1 OP_POP for scope cleanup"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 20: Disassembly runs without crash on compiled output
// ════════════════════════════════════════════════════════════════
void testDisassemblyIntegration() {
    const char* name = "Disassembly of compiled bytecode doesn't crash";
    Compiler c;
    bool ok = compileSource(R"(
        int fact(int n) {
            if (n <= 1) { return 1; }
            return n * fact(n - 1);
        }
        int main() {
            int r = fact(5);
            int s = 0;
            for (int i = 0; i < 10; i++) { s = s + i; }
            return s;
        }
    )", c);
    if (!ok) { fail(name, "compilation failed"); return; }

    std::cout << "\n";
    for (auto& [fname, fn] : c.getAllFunctions()) {
        fn->chunk.disassemble();
    }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// MAIN
// ════════════════════════════════════════════════════════════════
int main() {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout <<   "║       CVM++ Compiler Tests               ║\n";
    std::cout <<   "╚══════════════════════════════════════════╝\n\n";

    testSimpleFunction();
    testFunctionParams();
    testVarDecl();
    testVarDeclDefault();
    testBoolDefault();
    testArithmetic();
    testComparisons();
    testIfElse();
    testWhileLoop();
    testForLoop();
    testFunctionCall();
    testRecursion();
    testAssignment();
    testBoolLiterals();
    testUnaryNegate();
    testUnaryNot();
    testUndefinedVar();
    testMultipleFunctions();
    testNestedScopes();
    testDisassemblyIntegration();

    std::cout << "\n══════════════════════════════════════════\n";
    std::cout << "  Results: " << testsPassed << " passed, "
              << testsFailed << " failed\n";
    std::cout << "══════════════════════════════════════════\n\n";

    return testsFailed > 0 ? 1 : 0;
}
