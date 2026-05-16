#include "VM.h"
#include <iostream>

// ════════════════════════════════════════════════════════════════
// Construction & setup
// ════════════════════════════════════════════════════════════════

VM::VM() {
    stack_.reserve(256);
}

void VM::loadFunctions(const Compiler& compiler) {
    for (auto& [name, fn] : compiler.getAllFunctions()) {
        functions_[name] = fn.get();
    }
}

InterpretResult VM::run(const std::string& entryPoint) {
    auto it = functions_.find(entryPoint);
    if (it == functions_.end()) {
        std::cerr << "runtime error: function '" << entryPoint << "' not found\n";
        return InterpretResult::RUNTIME_ERROR;
    }

    // Set up the initial call frame
    stack_.clear();
    frames_.clear();

    CallFrame frame;
    frame.function = it->second;
    frame.ip = 0;
    frame.stackBase = 0;
    frames_.push_back(frame);

    return execute();
}

// ════════════════════════════════════════════════════════════════
// Stack operations
// ════════════════════════════════════════════════════════════════

void VM::push(Value val) { stack_.push_back(std::move(val)); }

Value VM::pop() {
    Value val = std::move(stack_.back());
    stack_.pop_back();
    return val;
}

Value& VM::peek(int distance) {
    return stack_[stack_.size() - 1 - distance];
}

// ════════════════════════════════════════════════════════════════
// Bytecode reading
// ════════════════════════════════════════════════════════════════

CallFrame& VM::currentFrame() { return frames_.back(); }
Chunk& VM::currentChunk() { return currentFrame().function->chunk; }

uint8_t VM::readByte() {
    return currentChunk().code[currentFrame().ip++];
}

uint16_t VM::readShort() {
    uint8_t hi = readByte();
    uint8_t lo = readByte();
    return (uint16_t)(hi << 8) | lo;
}

Value VM::readConstant() {
    uint8_t index = readByte();
    return currentChunk().constants[index];
}

// ════════════════════════════════════════════════════════════════
// Runtime helpers — type coercion & truthiness
// ════════════════════════════════════════════════════════════════

int64_t VM::asInt(const Value& val) {
    if (std::holds_alternative<int64_t>(val)) return std::get<int64_t>(val);
    if (std::holds_alternative<double>(val))  return static_cast<int64_t>(std::get<double>(val));
    if (std::holds_alternative<bool>(val))    return std::get<bool>(val) ? 1 : 0;
    return 0;
}

double VM::asDouble(const Value& val) {
    if (std::holds_alternative<double>(val))  return std::get<double>(val);
    if (std::holds_alternative<int64_t>(val)) return static_cast<double>(std::get<int64_t>(val));
    if (std::holds_alternative<bool>(val))    return std::get<bool>(val) ? 1.0 : 0.0;
    return 0.0;
}

bool VM::asBool(const Value& val) {
    return isTruthy(val);
}

bool VM::isTruthy(const Value& val) {
    if (std::holds_alternative<bool>(val))    return std::get<bool>(val);
    if (std::holds_alternative<int64_t>(val)) return std::get<int64_t>(val) != 0;
    if (std::holds_alternative<double>(val))  return std::get<double>(val) != 0.0;
    return false;
}

bool VM::isNumber(const Value& val) {
    return std::holds_alternative<int64_t>(val) || std::holds_alternative<double>(val);
}

void VM::runtimeError(const std::string& msg) {
    auto& frame = currentFrame();
    int line = currentChunk().lines[frame.ip > 0 ? frame.ip - 1 : 0];
    std::cerr << "runtime error (line " << line << "): " << msg << "\n";

    // Print call stack
    for (int i = static_cast<int>(frames_.size()) - 1; i >= 0; i--) {
        auto& f = frames_[i];
        int ln = f.function->chunk.lines[f.ip > 0 ? f.ip - 1 : 0];
        std::cerr << "  in " << f.function->name << "() [line " << ln << "]\n";
    }
}

// ════════════════════════════════════════════════════════════════
// THE CORE DISPATCH LOOP
// ════════════════════════════════════════════════════════════════

InterpretResult VM::execute() {
    for (;;) {
        // ── Debug tracing ───────────────────────────────────────
        if (debugTrace_) {
            // Print stack state
            printf("          ");
            for (size_t i = 0; i < stack_.size(); i++) {
                printf("[ %s ]", valueToString(stack_[i]).c_str());
            }
            printf("\n");
            currentChunk().disassembleInstruction(currentFrame().ip);
        }

        uint8_t instruction = readByte();
        auto op = static_cast<OpCode>(instruction);

        switch (op) {

        // ── Constants & Literals ────────────────────────────────
        case OpCode::OP_CONSTANT: {
            push(readConstant());
            break;
        }
        case OpCode::OP_TRUE:  push(true);  break;
        case OpCode::OP_FALSE: push(false); break;

        // ── Arithmetic ─────────────────────────────────────────
        case OpCode::OP_ADD: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) + asDouble(b));
            else
                push(asInt(a) + asInt(b));
            break;
        }
        case OpCode::OP_SUB: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) - asDouble(b));
            else
                push(asInt(a) - asInt(b));
            break;
        }
        case OpCode::OP_MUL: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) * asDouble(b));
            else
                push(asInt(a) * asInt(b));
            break;
        }
        case OpCode::OP_DIV: {
            Value b = pop(), a = pop();
            if (asInt(b) == 0 && !std::holds_alternative<double>(b)) {
                runtimeError("division by zero");
                return InterpretResult::RUNTIME_ERROR;
            }
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) / asDouble(b));
            else
                push(asInt(a) / asInt(b));
            break;
        }
        case OpCode::OP_MOD: {
            Value b = pop(), a = pop();
            int64_t bv = asInt(b);
            if (bv == 0) {
                runtimeError("modulo by zero");
                return InterpretResult::RUNTIME_ERROR;
            }
            push(asInt(a) % bv);
            break;
        }
        case OpCode::OP_NEGATE: {
            Value a = pop();
            if (std::holds_alternative<double>(a))
                push(-std::get<double>(a));
            else
                push(-asInt(a));
            break;
        }

        // ── Comparison ─────────────────────────────────────────
        case OpCode::OP_EQUAL: {
            Value b = pop(), a = pop();
            if (isNumber(a) && isNumber(b)) {
                if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                    push(asDouble(a) == asDouble(b));
                else
                    push(asInt(a) == asInt(b));
            } else {
                push(asBool(a) == asBool(b));
            }
            break;
        }
        case OpCode::OP_NOT_EQUAL: {
            Value b = pop(), a = pop();
            if (isNumber(a) && isNumber(b))
                push(asInt(a) != asInt(b));
            else
                push(asBool(a) != asBool(b));
            break;
        }
        case OpCode::OP_LESS: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) < asDouble(b));
            else
                push(asInt(a) < asInt(b));
            break;
        }
        case OpCode::OP_LESS_EQUAL: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) <= asDouble(b));
            else
                push(asInt(a) <= asInt(b));
            break;
        }
        case OpCode::OP_GREATER: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) > asDouble(b));
            else
                push(asInt(a) > asInt(b));
            break;
        }
        case OpCode::OP_GREATER_EQUAL: {
            Value b = pop(), a = pop();
            if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b))
                push(asDouble(a) >= asDouble(b));
            else
                push(asInt(a) >= asInt(b));
            break;
        }

        // ── Logical ────────────────────────────────────────────
        case OpCode::OP_NOT: {
            push(!isTruthy(pop()));
            break;
        }

        // ── Variables ──────────────────────────────────────────
        case OpCode::OP_GET_LOCAL: {
            uint8_t slot = readByte();
            push(stack_[currentFrame().stackBase + slot]);
            break;
        }
        case OpCode::OP_SET_LOCAL: {
            uint8_t slot = readByte();
            stack_[currentFrame().stackBase + slot] = peek(0);
            break;
        }
        case OpCode::OP_GET_GLOBAL:
        case OpCode::OP_SET_GLOBAL: {
            // Not used in current implementation
            readByte();
            break;
        }

        // ── Control Flow ───────────────────────────────────────
        case OpCode::OP_JUMP: {
            uint16_t offset = readShort();
            currentFrame().ip += offset;
            break;
        }
        case OpCode::OP_JUMP_IF_FALSE: {
            uint16_t offset = readShort();
            if (!isTruthy(peek(0))) {
                currentFrame().ip += offset;
            }
            break;
        }
        case OpCode::OP_LOOP: {
            uint16_t offset = readShort();
            currentFrame().ip -= offset;
            break;
        }

        // ── Functions ──────────────────────────────────────────
        case OpCode::OP_CALL: {
            uint8_t argCount = readByte();
            uint8_t nameIdx  = readByte();
            std::string fnName = std::get<std::string>(
                currentChunk().constants[nameIdx]);

            auto it = functions_.find(fnName);
            if (it == functions_.end()) {
                runtimeError("undefined function '" + fnName + "'");
                return InterpretResult::RUNTIME_ERROR;
            }

            FunctionObject* fn = it->second;
            if (fn->arity != argCount) {
                runtimeError("function '" + fnName + "' expects " +
                    std::to_string(fn->arity) + " args, got " +
                    std::to_string(argCount));
                return InterpretResult::RUNTIME_ERROR;
            }

            // The arguments are already on the stack.
            // stackBase points to where the first argument is.
            CallFrame newFrame;
            newFrame.function = fn;
            newFrame.ip = 0;
            newFrame.stackBase = stack_.size() - argCount;
            frames_.push_back(newFrame);
            break;
        }

        case OpCode::OP_RETURN: {
            Value result = pop();

            // Clean up: discard everything this frame put on the stack
            size_t base = currentFrame().stackBase;
            frames_.pop_back();

            if (frames_.empty()) {
                // We've returned from the top-level function — done!
                returnValue_ = result;
                return InterpretResult::OK;
            }

            // Discard the callee's locals + arguments
            stack_.resize(base);
            // Push the return value for the caller
            push(result);
            break;
        }

        // ── I/O ────────────────────────────────────────────────
        case OpCode::OP_PRINT: {
            Value val = pop();
            std::cout << valueToString(val) << "\n";
            break;
        }

        // ── Stack management ───────────────────────────────────
        case OpCode::OP_POP: {
            pop();
            break;
        }

        } // switch
    } // for
}
