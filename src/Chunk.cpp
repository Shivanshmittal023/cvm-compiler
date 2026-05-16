#include "Chunk.h"
#include <iomanip>
#include <iostream>

// ────────────────────────────────────────────────────────────────
// Value helpers
// ────────────────────────────────────────────────────────────────

std::string valueToString(const Value& val) {
    if (std::holds_alternative<int64_t>(val))
        return std::to_string(std::get<int64_t>(val));
    if (std::holds_alternative<double>(val))
        return std::to_string(std::get<double>(val));
    if (std::holds_alternative<bool>(val))
        return std::get<bool>(val) ? "true" : "false";
    if (std::holds_alternative<std::string>(val))
        return std::get<std::string>(val);
    return "<unknown>";
}

// ────────────────────────────────────────────────────────────────
// Writing bytecode
// ────────────────────────────────────────────────────────────────

void Chunk::write(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::write(OpCode op, int line) {
    write(static_cast<uint8_t>(op), line);
}

uint8_t Chunk::addConstant(Value value) {
    constants.push_back(std::move(value));
    return static_cast<uint8_t>(constants.size() - 1);
}

// ────────────────────────────────────────────────────────────────
// Disassembly — human-readable dump of the bytecode
// ────────────────────────────────────────────────────────────────

void Chunk::disassemble() const {
    std::cout << "=== " << name << " ===\n";
    for (size_t offset = 0; offset < code.size();) {
        offset = disassembleInstruction(offset);
    }
    std::cout << "\n";
}

size_t Chunk::disassembleInstruction(size_t offset) const {
    // Print offset (address)
    printf("%04zu  ", offset);

    // Print source line (or '|' if same as previous)
    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", lines[offset]);
    }

    auto op = static_cast<OpCode>(code[offset]);

    switch (op) {
        // ── Simple instructions (no operands) ───────────────────
        case OpCode::OP_ADD:
        case OpCode::OP_SUB:
        case OpCode::OP_MUL:
        case OpCode::OP_DIV:
        case OpCode::OP_MOD:
        case OpCode::OP_NEGATE:
        case OpCode::OP_EQUAL:
        case OpCode::OP_NOT_EQUAL:
        case OpCode::OP_LESS:
        case OpCode::OP_LESS_EQUAL:
        case OpCode::OP_GREATER:
        case OpCode::OP_GREATER_EQUAL:
        case OpCode::OP_NOT:
        case OpCode::OP_TRUE:
        case OpCode::OP_FALSE:
        case OpCode::OP_RETURN:
        case OpCode::OP_PRINT:
        case OpCode::OP_POP: {
            std::string n = opcodeName(op);
            printf("%s\n", n.c_str());
            return offset + 1;
        }

        // ── 1-byte operand (constant index or slot) ─────────────
        case OpCode::OP_CONSTANT: {
            uint8_t index = code[offset + 1];
            std::string n = opcodeName(op);
            printf("%-20s %d  '%s'\n", n.c_str(), (int)index,
                   valueToString(constants[index]).c_str());
            return offset + 2;
        }
        case OpCode::OP_GET_LOCAL:
        case OpCode::OP_SET_LOCAL: {
            uint8_t slot = code[offset + 1];
            std::string n = opcodeName(op);
            printf("%-20s slot=%d\n", n.c_str(), (int)slot);
            return offset + 2;
        }
        case OpCode::OP_GET_GLOBAL:
        case OpCode::OP_SET_GLOBAL: {
            uint8_t index = code[offset + 1];
            std::string n = opcodeName(op);
            printf("%-20s %d  '%s'\n", n.c_str(), (int)index,
                   valueToString(constants[index]).c_str());
            return offset + 2;
        }
        case OpCode::OP_CALL: {
            uint8_t argCount = code[offset + 1];
            uint8_t nameIdx  = code[offset + 2];
            std::string n = opcodeName(op);
            printf("%-20s args=%d  fn='%s'\n", n.c_str(), (int)argCount,
                   valueToString(constants[nameIdx]).c_str());
            return offset + 3;
        }

        // ── 2-byte operand (jump offset) ────────────────────────
        case OpCode::OP_JUMP:
        case OpCode::OP_JUMP_IF_FALSE: {
            uint16_t jump = (uint16_t)(code[offset + 1] << 8) | code[offset + 2];
            std::string n = opcodeName(op);
            printf("%-20s %zu -> %zu\n", n.c_str(), offset, offset + 3 + jump);
            return offset + 3;
        }
        case OpCode::OP_LOOP: {
            uint16_t jump = (uint16_t)(code[offset + 1] << 8) | code[offset + 2];
            std::string n = opcodeName(op);
            printf("%-20s %zu -> %zu\n", n.c_str(), offset, offset + 3 - jump);
            return offset + 3;
        }
    }

    printf("UNKNOWN opcode %d\n", (int)code[offset]);
    return offset + 1;
}

