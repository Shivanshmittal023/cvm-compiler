#pragma once
#include "Chunk.h"
#include "Compiler.h"
#include <string>
#include <unordered_map>
#include <vector>

// ────────────────────────────────────────────────────────────────
// CALL FRAME — one per active function invocation.
//
// Tracks the instruction pointer (ip), the function being
// executed, and the base of this frame on the value stack.
// ────────────────────────────────────────────────────────────────
struct CallFrame {
    FunctionObject* function = nullptr;
    size_t ip = 0;          // index into function->chunk.code
    size_t stackBase = 0;   // first stack slot owned by this frame
};

// ────────────────────────────────────────────────────────────────
// INTERPRET RESULT — outcome of running the VM.
// ────────────────────────────────────────────────────────────────
enum class InterpretResult {
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR,
};

// ────────────────────────────────────────────────────────────────
// VM — Stack-based virtual machine that executes compiled bytecode.
//
// Usage:
//   VM vm;
//   vm.loadFunctions(compiler);
//   InterpretResult result = vm.run("main");
// ────────────────────────────────────────────────────────────────
class VM {
public:
    VM();

    /// Load all compiled functions from a Compiler.
    void loadFunctions(const Compiler& compiler);

    /// Execute a function by name (typically "main").
    InterpretResult run(const std::string& entryPoint = "main");

    /// Get the final return value after run() completes.
    const Value& getReturnValue() const { return returnValue_; }

    /// Enable/disable debug tracing (prints each instruction as it executes).
    void setDebugTrace(bool enabled) { debugTrace_ = enabled; }

private:
    // The value stack
    std::vector<Value> stack_;

    // The call stack
    std::vector<CallFrame> frames_;

    // All available functions (non-owning pointers)
    std::unordered_map<std::string, FunctionObject*> functions_;

    // Return value from the last run
    Value returnValue_ = int64_t{0};

    // Debug tracing
    bool debugTrace_ = false;

    // ── Stack operations ────────────────────────────────────────
    void    push(Value val);
    Value   pop();
    Value&  peek(int distance = 0);

    // ── Bytecode reading ────────────────────────────────────────
    CallFrame& currentFrame();
    Chunk&     currentChunk();
    uint8_t    readByte();
    uint16_t   readShort();
    Value      readConstant();

    // ── The core dispatch loop ──────────────────────────────────
    InterpretResult execute();

    // ── Runtime helpers ─────────────────────────────────────────
    int64_t asInt(const Value& val);
    double  asDouble(const Value& val);
    bool    asBool(const Value& val);
    bool    isTruthy(const Value& val);
    bool    isNumber(const Value& val);

    void runtimeError(const std::string& msg);
};
