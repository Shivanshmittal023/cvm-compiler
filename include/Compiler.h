#pragma once
#include "AST.h"
#include "Chunk.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// ────────────────────────────────────────────────────────────────
// A compiled function: name, parameter count, and its bytecode.
// ────────────────────────────────────────────────────────────────
struct FunctionObject {
    std::string name;
    int arity = 0;
    Chunk chunk;
};

// ────────────────────────────────────────────────────────────────
// Tracks a local variable during compilation.
// ────────────────────────────────────────────────────────────────
struct Local {
    std::string name;
    int depth;       // scope depth where this local was declared
};

// ────────────────────────────────────────────────────────────────
// COMPILER — walks the AST and emits bytecode into Chunks.
//
// Each C++ function becomes a FunctionObject with its own Chunk.
// The VM will look up functions by name and execute their chunks.
// ────────────────────────────────────────────────────────────────
class Compiler {
public:
    Compiler();

    /// Compile an entire program (all top-level declarations).
    bool compile(Program& program);

    /// Access a compiled function by name. Returns nullptr if not found.
    FunctionObject* getFunction(const std::string& name);

    /// Get all compiled functions (for debug/listing).
    const std::unordered_map<std::string, std::unique_ptr<FunctionObject>>&
    getAllFunctions() const;

    bool hadError() const { return hadError_; }

private:
    std::unordered_map<std::string, std::unique_ptr<FunctionObject>> functions_;
    FunctionObject* current_ = nullptr;  // function currently being compiled

    // Local variable tracking (per-function)
    std::vector<Local> locals_;
    int scopeDepth_ = 0;

    bool hadError_ = false;

    // ── Chunk helpers ───────────────────────────────────────────
    Chunk& currentChunk();
    void emitByte(uint8_t byte, int line);
    void emitOp(OpCode op, int line);
    void emitConstant(Value val, int line);
    size_t emitJump(OpCode op, int line);
    void patchJump(size_t offset);
    void emitLoop(size_t loopStart, int line);

    // ── Scope management ────────────────────────────────────────
    void beginScope();
    void endScope(int line);
    int  resolveLocal(const std::string& name);
    void addLocal(const std::string& name);

    // ── Declaration / function compilation ──────────────────────
    void compileFunction(FunctionDecl* func);

    // ── Statement compilation ───────────────────────────────────
    void compileStmt(Stmt* stmt);
    void compileBlockStmt(BlockStmt* stmt);
    void compileVarDeclStmt(VarDeclStmt* stmt);
    void compileIfStmt(IfStmt* stmt);
    void compileWhileStmt(WhileStmt* stmt);
    void compileForStmt(ForStmt* stmt);
    void compileReturnStmt(ReturnStmt* stmt);
    void compileExprStmt(ExprStmt* stmt);

    // ── Expression compilation ──────────────────────────────────
    void compileExpr(Expr* expr);
    void compileNumberExpr(NumberExpr* expr);
    void compileBoolLitExpr(BoolLitExpr* expr);
    void compileVariableExpr(VariableExpr* expr);
    void compileBinaryExpr(BinaryExpr* expr);
    void compileUnaryExpr(UnaryExpr* expr);
    void compileAssignExpr(AssignExpr* expr);
    void compileCallExpr(CallExpr* expr);

    // ── Error reporting ─────────────────────────────────────────
    void error(const std::string& message, int line);
};
