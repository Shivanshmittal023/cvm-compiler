#include "Opcode.h"

std::string opcodeName(OpCode op) {
    switch (op) {
        case OpCode::OP_CONSTANT:      return "OP_CONSTANT";
        case OpCode::OP_TRUE:          return "OP_TRUE";
        case OpCode::OP_FALSE:         return "OP_FALSE";

        case OpCode::OP_ADD:           return "OP_ADD";
        case OpCode::OP_SUB:           return "OP_SUB";
        case OpCode::OP_MUL:          return "OP_MUL";
        case OpCode::OP_DIV:           return "OP_DIV";
        case OpCode::OP_MOD:           return "OP_MOD";
        case OpCode::OP_NEGATE:        return "OP_NEGATE";

        case OpCode::OP_EQUAL:         return "OP_EQUAL";
        case OpCode::OP_NOT_EQUAL:     return "OP_NOT_EQUAL";
        case OpCode::OP_LESS:          return "OP_LESS";
        case OpCode::OP_LESS_EQUAL:    return "OP_LESS_EQUAL";
        case OpCode::OP_GREATER:       return "OP_GREATER";
        case OpCode::OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";

        case OpCode::OP_NOT:           return "OP_NOT";

        case OpCode::OP_GET_LOCAL:     return "OP_GET_LOCAL";
        case OpCode::OP_SET_LOCAL:     return "OP_SET_LOCAL";
        case OpCode::OP_GET_GLOBAL:    return "OP_GET_GLOBAL";
        case OpCode::OP_SET_GLOBAL:    return "OP_SET_GLOBAL";

        case OpCode::OP_JUMP:          return "OP_JUMP";
        case OpCode::OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OpCode::OP_LOOP:          return "OP_LOOP";

        case OpCode::OP_CALL:          return "OP_CALL";
        case OpCode::OP_RETURN:        return "OP_RETURN";

        case OpCode::OP_PRINT:         return "OP_PRINT";
        case OpCode::OP_POP:           return "OP_POP";
    }
    return "UNKNOWN_OP";
}
