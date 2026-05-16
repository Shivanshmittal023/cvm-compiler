#pragma once
#include <cstdint>
#include <string>

// ────────────────────────────────────────────────────────────────
// CVM++ Instruction Set Architecture (ISA)
//
// Each opcode is a single byte (uint8_t). Operands follow inline
// in the bytecode stream.  The VM is stack-based: most instructions
// pop their inputs from the stack and push their result back.
// ────────────────────────────────────────────────────────────────

enum class OpCode : uint8_t {
    // ── Constants & Literals ────────────────────────────────────
    OP_CONSTANT,      // [1-byte index] → push constants[index]
    OP_TRUE,          //                → push true
    OP_FALSE,         //                → push false

    // ── Arithmetic ─────────────────────────────────────────────
    OP_ADD,           // pop b, pop a   → push a + b
    OP_SUB,           // pop b, pop a   → push a - b
    OP_MUL,           // pop b, pop a   → push a * b
    OP_DIV,           // pop b, pop a   → push a / b
    OP_MOD,           // pop b, pop a   → push a % b
    OP_NEGATE,        // pop a          → push -a

    // ── Comparison ─────────────────────────────────────────────
    OP_EQUAL,         // pop b, pop a   → push a == b
    OP_NOT_EQUAL,     // pop b, pop a   → push a != b
    OP_LESS,          // pop b, pop a   → push a <  b
    OP_LESS_EQUAL,    // pop b, pop a   → push a <= b
    OP_GREATER,       // pop b, pop a   → push a >  b
    OP_GREATER_EQUAL, // pop b, pop a   → push a >= b

    // ── Logical ────────────────────────────────────────────────
    OP_NOT,           // pop a          → push !a

    // ── Variables ──────────────────────────────────────────────
    OP_GET_LOCAL,     // [1-byte slot]  → push stack[slot]
    OP_SET_LOCAL,     // [1-byte slot]  → stack[slot] = peek(); (no pop)

    OP_GET_GLOBAL,    // [1-byte index] → push globals[constants[index]]
    OP_SET_GLOBAL,    // [1-byte index] → globals[constants[index]] = peek()

    // ── Control Flow ───────────────────────────────────────────
    OP_JUMP,          // [2-byte offset] → ip += offset
    OP_JUMP_IF_FALSE, // [2-byte offset] → if falsy: ip += offset
    OP_LOOP,          // [2-byte offset] → ip -= offset  (jump back)

    // ── Functions ──────────────────────────────────────────────
    OP_CALL,          // [1-byte argCount] → call function
    OP_RETURN,        //                   → return from function

    // ── I/O ────────────────────────────────────────────────────
    OP_PRINT,         // pop a → print a to stdout

    // ── Stack management ───────────────────────────────────────
    OP_POP,           // pop and discard top of stack
};

// Human-readable name for debug/disassembly output
std::string opcodeName(OpCode op);
