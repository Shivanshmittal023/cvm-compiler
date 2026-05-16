#pragma once
#include "Opcode.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// ────────────────────────────────────────────────────────────────
// VALUE — The runtime representation of data in the VM.
//
// Our Mini-C++ subset supports: integers, doubles, and booleans.
// We use std::variant so the VM can distinguish types at runtime.
// ────────────────────────────────────────────────────────────────

using Value = std::variant<int64_t, double, bool, std::string>;

/// Pretty-print a Value for debugging and OP_PRINT output.
std::string valueToString(const Value& val);

// ────────────────────────────────────────────────────────────────
// CHUNK — A compiled unit of bytecode.
//
// Holds:
//   1. code     – the raw bytecode bytes (opcodes + inline operands)
//   2. constants – the constant pool (literal values referenced
//                  by OP_CONSTANT via index)
//   3. lines    – source line number for each byte (for errors)
//   4. name     – human-readable label (e.g. function name)
// ────────────────────────────────────────────────────────────────

struct Chunk {
    std::string name;                 // e.g. "main", "factorial"
    std::vector<uint8_t> code;        // the bytecode stream
    std::vector<Value>   constants;   // constant pool
    std::vector<int>     lines;       // source line per byte

    // ── Writing bytecode ────────────────────────────────────────

    /// Append a single byte (opcode or operand) with its source line.
    void write(uint8_t byte, int line);

    /// Convenience: write an OpCode enum value.
    void write(OpCode op, int line);

    // ── Constants ───────────────────────────────────────────────

    /// Add a constant to the pool.  Returns its index (0–255).
    uint8_t addConstant(Value value);

    // ── Disassembly (debug) ─────────────────────────────────────

    /// Pretty-print the entire chunk for debugging.
    void disassemble() const;

    /// Disassemble a single instruction starting at `offset`.
    /// Returns the offset of the NEXT instruction.
    size_t disassembleInstruction(size_t offset) const;
};
