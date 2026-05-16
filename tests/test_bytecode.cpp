// ────────────────────────────────────────────────────────────────
// test_bytecode.cpp — Exhaustive test of Opcode ISA + Chunk
// ────────────────────────────────────────────────────────────────

#include "Chunk.h"
#include "Opcode.h"
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

// ════════════════════════════════════════════════════════════════
// Test 1: Chunk::write() single byte
// ════════════════════════════════════════════════════════════════
void testChunkWrite() {
    const char* name = "Chunk write single byte";
    Chunk c;
    c.name = "test";
    c.write(OpCode::OP_RETURN, 42);

    if (c.code.size() != 1) { fail(name, "code size != 1"); return; }
    if (c.lines.size() != 1) { fail(name, "lines size != 1"); return; }
    if (c.code[0] != static_cast<uint8_t>(OpCode::OP_RETURN)) { fail(name, "wrong opcode"); return; }
    if (c.lines[0] != 42) { fail(name, "wrong line"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 2: Chunk::write() multiple bytes
// ════════════════════════════════════════════════════════════════
void testChunkWriteMultiple() {
    const char* name = "Chunk write multiple bytes";
    Chunk c;
    c.name = "test";
    c.write(OpCode::OP_ADD, 1);
    c.write(OpCode::OP_SUB, 2);
    c.write(OpCode::OP_MUL, 3);

    if (c.code.size() != 3) { fail(name, "code size != 3"); return; }
    if (c.code[0] != static_cast<uint8_t>(OpCode::OP_ADD)) { fail(name, "op[0] wrong"); return; }
    if (c.code[1] != static_cast<uint8_t>(OpCode::OP_SUB)) { fail(name, "op[1] wrong"); return; }
    if (c.code[2] != static_cast<uint8_t>(OpCode::OP_MUL)) { fail(name, "op[2] wrong"); return; }
    if (c.lines[0] != 1 || c.lines[1] != 2 || c.lines[2] != 3) { fail(name, "lines wrong"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 3: Constant pool — sequential indices
// ════════════════════════════════════════════════════════════════
void testConstantPool() {
    const char* name = "Constant pool returns sequential indices";
    Chunk c;
    c.name = "const_test";
    uint8_t i0 = c.addConstant(int64_t{42});
    uint8_t i1 = c.addConstant(int64_t{100});
    uint8_t i2 = c.addConstant(3.14);
    uint8_t i3 = c.addConstant(true);
    uint8_t i4 = c.addConstant(false);

    if (i0 != 0 || i1 != 1 || i2 != 2 || i3 != 3 || i4 != 4) { fail(name, "wrong indices"); return; }
    if (c.constants.size() != 5) { fail(name, "pool size != 5"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 4: Constants store correct values and types
// ════════════════════════════════════════════════════════════════
void testConstantValues() {
    const char* name = "Constants store correct values & types";
    Chunk c;
    c.addConstant(int64_t{-99});
    c.addConstant(2.718);
    c.addConstant(true);

    if (!std::holds_alternative<int64_t>(c.constants[0]) ||
        std::get<int64_t>(c.constants[0]) != -99) { fail(name, "int wrong"); return; }

    if (!std::holds_alternative<double>(c.constants[1]) ||
        std::get<double>(c.constants[1]) != 2.718) { fail(name, "double wrong"); return; }

    if (!std::holds_alternative<bool>(c.constants[2]) ||
        std::get<bool>(c.constants[2]) != true) { fail(name, "bool wrong"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 5: valueToString()
// ════════════════════════════════════════════════════════════════
void testValueToString() {
    const char* name = "valueToString for all types";
    Value v1 = int64_t{42};
    if (valueToString(v1) != "42") { fail(name, "int: got " + valueToString(v1)); return; }

    Value v2 = int64_t{-7};
    if (valueToString(v2) != "-7") { fail(name, "neg int: got " + valueToString(v2)); return; }

    Value v3 = true;
    if (valueToString(v3) != "true") { fail(name, "bool true: got " + valueToString(v3)); return; }

    Value v4 = false;
    if (valueToString(v4) != "false") { fail(name, "bool false: got " + valueToString(v4)); return; }

    Value v5 = 3.14;
    std::string s5 = valueToString(v5);
    if (s5.substr(0, 4) != "3.14") { fail(name, "double: got " + s5); return; }

    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 6: opcodeName() — every opcode has a name
// ════════════════════════════════════════════════════════════════
void testOpcodeNames() {
    const char* name = "All opcodes have correct names";
    struct { OpCode op; std::string expected; } cases[] = {
        {OpCode::OP_CONSTANT,      "OP_CONSTANT"},
        {OpCode::OP_TRUE,          "OP_TRUE"},
        {OpCode::OP_FALSE,         "OP_FALSE"},
        {OpCode::OP_ADD,           "OP_ADD"},
        {OpCode::OP_SUB,           "OP_SUB"},
        {OpCode::OP_MUL,           "OP_MUL"},
        {OpCode::OP_DIV,           "OP_DIV"},
        {OpCode::OP_MOD,           "OP_MOD"},
        {OpCode::OP_NEGATE,        "OP_NEGATE"},
        {OpCode::OP_EQUAL,         "OP_EQUAL"},
        {OpCode::OP_NOT_EQUAL,     "OP_NOT_EQUAL"},
        {OpCode::OP_LESS,          "OP_LESS"},
        {OpCode::OP_LESS_EQUAL,    "OP_LESS_EQUAL"},
        {OpCode::OP_GREATER,       "OP_GREATER"},
        {OpCode::OP_GREATER_EQUAL, "OP_GREATER_EQUAL"},
        {OpCode::OP_NOT,           "OP_NOT"},
        {OpCode::OP_GET_LOCAL,     "OP_GET_LOCAL"},
        {OpCode::OP_SET_LOCAL,     "OP_SET_LOCAL"},
        {OpCode::OP_GET_GLOBAL,    "OP_GET_GLOBAL"},
        {OpCode::OP_SET_GLOBAL,    "OP_SET_GLOBAL"},
        {OpCode::OP_JUMP,          "OP_JUMP"},
        {OpCode::OP_JUMP_IF_FALSE, "OP_JUMP_IF_FALSE"},
        {OpCode::OP_LOOP,          "OP_LOOP"},
        {OpCode::OP_CALL,          "OP_CALL"},
        {OpCode::OP_RETURN,        "OP_RETURN"},
        {OpCode::OP_PRINT,         "OP_PRINT"},
        {OpCode::OP_POP,           "OP_POP"},
    };

    for (const auto& tc : cases) {
        std::string got = opcodeName(tc.op);
        if (got != tc.expected) {
            fail(name, ("mismatch: expected " + tc.expected + ", got " + got).c_str());
            return;
        }
    }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 7: Full bytecode sequence — "10 + 20"
// ════════════════════════════════════════════════════════════════
void testFullSequence_Add() {
    const char* name = "Full bytecode: CONST(10) CONST(20) ADD RET";
    Chunk c;
    c.name = "add_test";
    uint8_t a = c.addConstant(int64_t{10});
    uint8_t b = c.addConstant(int64_t{20});

    c.write(OpCode::OP_CONSTANT, 1);  c.write(a, 1);
    c.write(OpCode::OP_CONSTANT, 1);  c.write(b, 1);
    c.write(OpCode::OP_ADD, 1);
    c.write(OpCode::OP_RETURN, 1);

    // Verify bytecode layout: 6 bytes total
    if (c.code.size() != 6) { fail(name, "size != 6"); return; }
    if (c.code[0] != static_cast<uint8_t>(OpCode::OP_CONSTANT)) { fail(name, "byte 0"); return; }
    if (c.code[1] != 0) { fail(name, "byte 1 should be const index 0"); return; }
    if (c.code[2] != static_cast<uint8_t>(OpCode::OP_CONSTANT)) { fail(name, "byte 2"); return; }
    if (c.code[3] != 1) { fail(name, "byte 3 should be const index 1"); return; }
    if (c.code[4] != static_cast<uint8_t>(OpCode::OP_ADD)) { fail(name, "byte 4"); return; }
    if (c.code[5] != static_cast<uint8_t>(OpCode::OP_RETURN)) { fail(name, "byte 5"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 8: Jump offset encoding (2-byte big-endian)
// ════════════════════════════════════════════════════════════════
void testJumpEncoding() {
    const char* name = "Jump offset 2-byte big-endian encoding";
    Chunk c;
    c.name = "jump_test";

    c.write(OpCode::OP_JUMP, 5);
    uint16_t offset = 258;
    c.write(static_cast<uint8_t>((offset >> 8) & 0xFF), 5);
    c.write(static_cast<uint8_t>(offset & 0xFF), 5);

    if (c.code.size() != 3) { fail(name, "size != 3"); return; }
    if (c.code[1] != 1) { fail(name, "high byte != 1"); return; }
    if (c.code[2] != 2) { fail(name, "low byte != 2"); return; }

    uint16_t decoded = (uint16_t)(c.code[1] << 8) | c.code[2];
    if (decoded != 258) { fail(name, "decoded != 258"); return; }
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 9: Disassembler — all opcodes without crash
// ════════════════════════════════════════════════════════════════
void testDisassemblerAllOpcodes() {
    const char* name = "Disassembler handles all opcodes without crash";
    Chunk c;
    c.name = "disasm_all";

    // Simple ops
    c.write(OpCode::OP_ADD, 1);
    c.write(OpCode::OP_SUB, 1);
    c.write(OpCode::OP_MUL, 1);
    c.write(OpCode::OP_DIV, 1);
    c.write(OpCode::OP_MOD, 1);
    c.write(OpCode::OP_NEGATE, 2);
    c.write(OpCode::OP_EQUAL, 2);
    c.write(OpCode::OP_NOT_EQUAL, 2);
    c.write(OpCode::OP_LESS, 2);
    c.write(OpCode::OP_LESS_EQUAL, 2);
    c.write(OpCode::OP_GREATER, 3);
    c.write(OpCode::OP_GREATER_EQUAL, 3);
    c.write(OpCode::OP_NOT, 3);
    c.write(OpCode::OP_TRUE, 3);
    c.write(OpCode::OP_FALSE, 3);
    c.write(OpCode::OP_PRINT, 4);
    c.write(OpCode::OP_POP, 4);

    // 1-byte operand ops
    uint8_t ci = c.addConstant(int64_t{999});
    c.write(OpCode::OP_CONSTANT, 5); c.write(ci, 5);
    c.write(OpCode::OP_GET_LOCAL, 6); c.write(uint8_t{3}, 6);
    c.write(OpCode::OP_SET_LOCAL, 6); c.write(uint8_t{3}, 6);

    uint8_t gi = c.addConstant(int64_t{0});
    c.write(OpCode::OP_GET_GLOBAL, 7); c.write(gi, 7);
    c.write(OpCode::OP_SET_GLOBAL, 7); c.write(gi, 7);
    c.write(OpCode::OP_CALL, 8); c.write(uint8_t{2}, 8);

    // 2-byte operand ops
    c.write(OpCode::OP_JUMP, 9); c.write(uint8_t{0}, 9); c.write(uint8_t{5}, 9);
    c.write(OpCode::OP_JUMP_IF_FALSE, 10); c.write(uint8_t{0}, 10); c.write(uint8_t{10}, 10);
    c.write(OpCode::OP_LOOP, 11); c.write(uint8_t{0}, 11); c.write(uint8_t{20}, 11);

    c.write(OpCode::OP_RETURN, 12);

    std::cout << "\n";
    c.disassemble();  // Should NOT crash
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 10: Simulated if/else bytecode pattern
// ════════════════════════════════════════════════════════════════
void testIfElsePattern() {
    const char* name = "If/else bytecode pattern (with jump patching)";
    Chunk c;
    c.name = "if_else";

    // Condition: GET_LOCAL(0) < 10
    c.write(OpCode::OP_GET_LOCAL, 1); c.write(uint8_t{0}, 1);
    uint8_t ten = c.addConstant(int64_t{10});
    c.write(OpCode::OP_CONSTANT, 1); c.write(ten, 1);
    c.write(OpCode::OP_LESS, 1);

    // JUMP_IF_FALSE to else
    c.write(OpCode::OP_JUMP_IF_FALSE, 1);
    size_t jumpToElse = c.code.size();
    c.write(uint8_t{0}, 1); c.write(uint8_t{0}, 1);  // placeholder

    c.write(OpCode::OP_POP, 1);  // pop condition

    // Then: a = 1
    uint8_t one = c.addConstant(int64_t{1});
    c.write(OpCode::OP_CONSTANT, 2); c.write(one, 2);
    c.write(OpCode::OP_SET_LOCAL, 2); c.write(uint8_t{1}, 2);
    c.write(OpCode::OP_POP, 2);

    // JUMP over else
    c.write(OpCode::OP_JUMP, 2);
    size_t jumpOverElse = c.code.size();
    c.write(uint8_t{0}, 2); c.write(uint8_t{0}, 2);

    // Patch jump-to-else
    size_t elseStart = c.code.size();
    uint16_t elseOff = static_cast<uint16_t>(elseStart - jumpToElse - 2);
    c.code[jumpToElse]     = (elseOff >> 8) & 0xFF;
    c.code[jumpToElse + 1] = elseOff & 0xFF;

    c.write(OpCode::OP_POP, 3);

    // Else: a = 2
    uint8_t two = c.addConstant(int64_t{2});
    c.write(OpCode::OP_CONSTANT, 3); c.write(two, 3);
    c.write(OpCode::OP_SET_LOCAL, 3); c.write(uint8_t{1}, 3);
    c.write(OpCode::OP_POP, 3);

    // Patch jump-over-else
    size_t afterElse = c.code.size();
    uint16_t overOff = static_cast<uint16_t>(afterElse - jumpOverElse - 2);
    c.code[jumpOverElse]     = (overOff >> 8) & 0xFF;
    c.code[jumpOverElse + 1] = overOff & 0xFF;

    c.write(OpCode::OP_RETURN, 4);

    if (c.constants.size() != 3) { fail(name, "expected 3 constants"); return; }
    if (c.code.empty()) { fail(name, "empty code"); return; }

    std::cout << "\n";
    c.disassemble();
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 11: While loop bytecode pattern
// ════════════════════════════════════════════════════════════════
void testWhileLoopPattern() {
    const char* name = "While loop bytecode pattern (with OP_LOOP)";
    Chunk c;
    c.name = "while_loop";

    // Init: i = 0  (slot 0)
    uint8_t zero = c.addConstant(int64_t{0});
    c.write(OpCode::OP_CONSTANT, 1); c.write(zero, 1);

    // Loop start
    size_t loopStart = c.code.size();

    // Condition: i < 5
    c.write(OpCode::OP_GET_LOCAL, 2); c.write(uint8_t{0}, 2);
    uint8_t five = c.addConstant(int64_t{5});
    c.write(OpCode::OP_CONSTANT, 2); c.write(five, 2);
    c.write(OpCode::OP_LESS, 2);

    // Exit if false
    c.write(OpCode::OP_JUMP_IF_FALSE, 2);
    size_t exitJump = c.code.size();
    c.write(uint8_t{0}, 2); c.write(uint8_t{0}, 2);

    c.write(OpCode::OP_POP, 2);

    // Body: i = i + 1
    c.write(OpCode::OP_GET_LOCAL, 3); c.write(uint8_t{0}, 3);
    uint8_t oneIdx = c.addConstant(int64_t{1});
    c.write(OpCode::OP_CONSTANT, 3); c.write(oneIdx, 3);
    c.write(OpCode::OP_ADD, 3);
    c.write(OpCode::OP_SET_LOCAL, 3); c.write(uint8_t{0}, 3);
    c.write(OpCode::OP_POP, 3);

    // Loop back
    c.write(OpCode::OP_LOOP, 3);
    size_t loopEnd = c.code.size();
    uint16_t loopOffset = static_cast<uint16_t>(loopEnd - loopStart + 2);
    c.write(static_cast<uint8_t>((loopOffset >> 8) & 0xFF), 3);
    c.write(static_cast<uint8_t>(loopOffset & 0xFF), 3);

    // Patch exit jump
    size_t afterLoop = c.code.size();
    uint16_t exitOff = static_cast<uint16_t>(afterLoop - exitJump - 2);
    c.code[exitJump]     = (exitOff >> 8) & 0xFF;
    c.code[exitJump + 1] = exitOff & 0xFF;

    c.write(OpCode::OP_POP, 4);
    c.write(OpCode::OP_RETURN, 4);

    if (c.code.empty()) { fail(name, "empty code"); return; }

    std::cout << "\n";
    c.disassemble();
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// Test 12: Edge case — empty chunk disassembly
// ════════════════════════════════════════════════════════════════
void testEmptyChunk() {
    const char* name = "Empty chunk disassembly doesn't crash";
    Chunk c;
    c.name = "<empty>";
    std::cout << "\n";
    c.disassemble();  // Should print header and nothing else
    pass(name);
}

// ════════════════════════════════════════════════════════════════
// MAIN
// ════════════════════════════════════════════════════════════════
int main() {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout <<   "║   CVM++ Bytecode Infrastructure Tests    ║\n";
    std::cout <<   "╚══════════════════════════════════════════╝\n\n";

    testChunkWrite();
    testChunkWriteMultiple();
    testConstantPool();
    testConstantValues();
    testValueToString();
    testOpcodeNames();
    testFullSequence_Add();
    testJumpEncoding();
    testDisassemblerAllOpcodes();
    testIfElsePattern();
    testWhileLoopPattern();
    testEmptyChunk();

    std::cout << "\n══════════════════════════════════════════\n";
    std::cout << "  Results: " << testsPassed << " passed, "
              << testsFailed << " failed\n";
    std::cout << "══════════════════════════════════════════\n\n";

    return testsFailed > 0 ? 1 : 0;
}
