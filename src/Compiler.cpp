#include "Compiler.h"
#include <iostream>

// ════════════════════════════════════════════════════════════════
// Construction & public API
// ════════════════════════════════════════════════════════════════

Compiler::Compiler() {}

bool Compiler::compile(Program& program) {
    for (auto& decl : program.declarations) {
        if (auto* func = dynamic_cast<FunctionDecl*>(decl.get())) {
            compileFunction(func);
        }
        // Top-level VarDecl not supported (all vars must be inside functions)
    }
    return !hadError_;
}

FunctionObject* Compiler::getFunction(const std::string& name) {
    auto it = functions_.find(name);
    return (it != functions_.end()) ? it->second.get() : nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<FunctionObject>>&
Compiler::getAllFunctions() const {
    return functions_;
}

// ════════════════════════════════════════════════════════════════
// Chunk helpers
// ════════════════════════════════════════════════════════════════

Chunk& Compiler::currentChunk() { return current_->chunk; }

void Compiler::emitByte(uint8_t byte, int line) {
    currentChunk().write(byte, line);
}

void Compiler::emitOp(OpCode op, int line) {
    currentChunk().write(op, line);
}

void Compiler::emitConstant(Value val, int line) {
    uint8_t index = currentChunk().addConstant(std::move(val));
    emitOp(OpCode::OP_CONSTANT, line);
    emitByte(index, line);
}

/// Emit a jump instruction with a 2-byte placeholder.
/// Returns the offset of the placeholder bytes (for patching later).
size_t Compiler::emitJump(OpCode op, int line) {
    emitOp(op, line);
    size_t offset = currentChunk().code.size();
    emitByte(0xFF, line);  // placeholder high byte
    emitByte(0xFF, line);  // placeholder low byte
    return offset;
}

/// Patch a previously emitted jump to land at the current position.
void Compiler::patchJump(size_t offset) {
    auto& code = currentChunk().code;
    size_t jump = code.size() - offset - 2;

    if (jump > 0xFFFF) {
        error("jump too large", currentChunk().lines[offset]);
        return;
    }
    code[offset]     = (jump >> 8) & 0xFF;
    code[offset + 1] = jump & 0xFF;
}

/// Emit a backwards jump (for loops).
void Compiler::emitLoop(size_t loopStart, int line) {
    emitOp(OpCode::OP_LOOP, line);
    size_t offset = currentChunk().code.size() - loopStart + 2;

    if (offset > 0xFFFF) {
        error("loop body too large", line);
        return;
    }
    emitByte((offset >> 8) & 0xFF, line);
    emitByte(offset & 0xFF, line);
}

// ════════════════════════════════════════════════════════════════
// Scope management
// ════════════════════════════════════════════════════════════════

void Compiler::beginScope() { scopeDepth_++; }

void Compiler::endScope(int line) {
    scopeDepth_--;
    // Pop all locals that belong to the scope we just left.
    while (!locals_.empty() && locals_.back().depth > scopeDepth_) {
        emitOp(OpCode::OP_POP, line);
        locals_.pop_back();
    }
}

int Compiler::resolveLocal(const std::string& name) {
    // Walk backwards so inner scopes shadow outer ones.
    for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; i--) {
        if (locals_[i].name == name) return i;
    }
    return -1;  // not found
}

void Compiler::addLocal(const std::string& name) {
    locals_.push_back({name, scopeDepth_});
}

// ════════════════════════════════════════════════════════════════
// Function compilation
// ════════════════════════════════════════════════════════════════

void Compiler::compileFunction(FunctionDecl* func) {
    // Create the function object
    auto fnObj = std::make_unique<FunctionObject>();
    fnObj->name = func->name;
    fnObj->arity = static_cast<int>(func->parameters.size());
    fnObj->chunk.name = func->name;

    // Save compiler state
    auto savedLocals = std::move(locals_);
    int  savedDepth  = scopeDepth_;
    auto savedCurrent = current_;

    // Set up fresh state for this function
    locals_.clear();
    scopeDepth_ = 0;
    current_ = fnObj.get();

    // Parameters become locals at depth 0 (they are already on the
    // stack when the VM enters the function via OP_CALL).
    for (auto& [type, pname] : func->parameters) {
        addLocal(pname);
    }

    // Compile the body — BlockStmt handles begin/endScope internally.
    compileBlockStmt(func->body.get());

    // Emit a safety-net return in case the function falls through
    // without an explicit 'return' statement.
    int lastLine = func->loc.line;
    emitConstant(int64_t{0}, lastLine);
    emitOp(OpCode::OP_RETURN, lastLine);

    // Restore state
    locals_  = std::move(savedLocals);
    scopeDepth_ = savedDepth;
    current_ = savedCurrent;

    // Store the compiled function
    functions_[func->name] = std::move(fnObj);
}

// ════════════════════════════════════════════════════════════════
// Statement compilation — dispatch by AST node type
// ════════════════════════════════════════════════════════════════

void Compiler::compileStmt(Stmt* stmt) {
    if (auto* s = dynamic_cast<BlockStmt*>(stmt))     { compileBlockStmt(s);   return; }
    if (auto* s = dynamic_cast<VarDeclStmt*>(stmt))   { compileVarDeclStmt(s); return; }
    if (auto* s = dynamic_cast<IfStmt*>(stmt))        { compileIfStmt(s);      return; }
    if (auto* s = dynamic_cast<WhileStmt*>(stmt))     { compileWhileStmt(s);   return; }
    if (auto* s = dynamic_cast<ForStmt*>(stmt))       { compileForStmt(s);     return; }
    if (auto* s = dynamic_cast<ReturnStmt*>(stmt))    { compileReturnStmt(s);  return; }
    if (auto* s = dynamic_cast<ExprStmt*>(stmt))      { compileExprStmt(s);    return; }

    error("unknown statement type", stmt->loc.line);
}

void Compiler::compileBlockStmt(BlockStmt* stmt) {
    beginScope();
    for (auto& s : stmt->statements) {
        compileStmt(s.get());
    }
    endScope(stmt->loc.line);
}

void Compiler::compileVarDeclStmt(VarDeclStmt* stmt) {
    // Compile the initializer (pushes value onto the stack).
    // The value IS the local variable — its stack position is the slot.
    if (stmt->initializer) {
        compileExpr(stmt->initializer.get());
    } else {
        // Default value: false for bool, 0 for everything else.
        if (stmt->varType == "bool") {
            emitOp(OpCode::OP_FALSE, stmt->loc.line);
        } else {
            emitConstant(int64_t{0}, stmt->loc.line);
        }
    }
    addLocal(stmt->name);
}

void Compiler::compileIfStmt(IfStmt* stmt) {
    int line = stmt->loc.line;

    // Compile condition → pushes bool
    compileExpr(stmt->condition.get());

    // Jump over the "then" branch if condition is false
    size_t thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE, line);
    emitOp(OpCode::OP_POP, line);  // pop the condition (true path)

    // Compile "then" branch
    compileStmt(stmt->thenBranch.get());

    // Jump over the false-path POP (and else branch if present)
    size_t elseJump = emitJump(OpCode::OP_JUMP, line);

    // False path lands here
    patchJump(thenJump);
    emitOp(OpCode::OP_POP, line);  // pop the condition (false path)

    if (stmt->elseBranch) {
        compileStmt(stmt->elseBranch.get());
    }

    patchJump(elseJump);
}

void Compiler::compileWhileStmt(WhileStmt* stmt) {
    int line = stmt->loc.line;

    size_t loopStart = currentChunk().code.size();

    // Compile condition
    compileExpr(stmt->condition.get());

    // Exit loop if false
    size_t exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, line);
    emitOp(OpCode::OP_POP, line);  // pop condition (true path)

    // Compile body
    compileStmt(stmt->body.get());

    // Loop back to condition
    emitLoop(loopStart, line);

    // Patch exit
    patchJump(exitJump);
    emitOp(OpCode::OP_POP, line);  // pop condition (false path)
}

void Compiler::compileForStmt(ForStmt* stmt) {
    int line = stmt->loc.line;

    // The for loop gets its own scope (for the loop variable).
    beginScope();

    // 1. Initializer
    if (stmt->initializer) {
        compileStmt(stmt->initializer.get());
    }

    // 2. Condition
    size_t loopStart = currentChunk().code.size();
    size_t exitJump = 0;
    bool hasCondition = (stmt->condition != nullptr);

    if (hasCondition) {
        compileExpr(stmt->condition.get());
        exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, line);
        emitOp(OpCode::OP_POP, line);  // pop condition (true path)
    }

    // 3. Body
    compileStmt(stmt->body.get());

    // 4. Increment
    if (stmt->increment) {
        compileExpr(stmt->increment.get());
        emitOp(OpCode::OP_POP, line);  // discard increment result
    }

    // 5. Loop back
    emitLoop(loopStart, line);

    // 6. Patch exit
    if (hasCondition) {
        patchJump(exitJump);
        emitOp(OpCode::OP_POP, line);  // pop condition (false path)
    }

    endScope(line);
}

void Compiler::compileReturnStmt(ReturnStmt* stmt) {
    int line = stmt->loc.line;
    if (stmt->returnValue) {
        compileExpr(stmt->returnValue.get());
    } else {
        emitConstant(int64_t{0}, line);  // default return 0
    }
    emitOp(OpCode::OP_RETURN, line);
}

void Compiler::compileExprStmt(ExprStmt* stmt) {
    compileExpr(stmt->expression.get());
    emitOp(OpCode::OP_POP, stmt->loc.line);  // discard expression result
}

// ════════════════════════════════════════════════════════════════
// Expression compilation — dispatch by AST node type
// ════════════════════════════════════════════════════════════════

void Compiler::compileExpr(Expr* expr) {
    if (auto* e = dynamic_cast<NumberExpr*>(expr))    { compileNumberExpr(e);   return; }
    if (auto* e = dynamic_cast<BoolLitExpr*>(expr))   { compileBoolLitExpr(e);  return; }
    if (auto* e = dynamic_cast<VariableExpr*>(expr))  { compileVariableExpr(e); return; }
    if (auto* e = dynamic_cast<BinaryExpr*>(expr))    { compileBinaryExpr(e);   return; }
    if (auto* e = dynamic_cast<UnaryExpr*>(expr))     { compileUnaryExpr(e);    return; }
    if (auto* e = dynamic_cast<AssignExpr*>(expr))    { compileAssignExpr(e);   return; }
    if (auto* e = dynamic_cast<CallExpr*>(expr))      { compileCallExpr(e);     return; }

    error("unknown expression type", expr->loc.line);
}

void Compiler::compileNumberExpr(NumberExpr* expr) {
    int line = expr->loc.line;
    if (std::holds_alternative<int64_t>(expr->value)) {
        emitConstant(std::get<int64_t>(expr->value), line);
    } else {
        emitConstant(std::get<double>(expr->value), line);
    }
}

void Compiler::compileBoolLitExpr(BoolLitExpr* expr) {
    emitOp(expr->value ? OpCode::OP_TRUE : OpCode::OP_FALSE, expr->loc.line);
}

void Compiler::compileVariableExpr(VariableExpr* expr) {
    int slot = resolveLocal(expr->name);
    if (slot == -1) {
        error("undefined variable '" + expr->name + "'", expr->loc.line);
        return;
    }
    emitOp(OpCode::OP_GET_LOCAL, expr->loc.line);
    emitByte(static_cast<uint8_t>(slot), expr->loc.line);
}

void Compiler::compileBinaryExpr(BinaryExpr* expr) {
    int line = expr->loc.line;
    const std::string& op = expr->op;

    // Short-circuit: &&
    if (op == "&&") {
        compileExpr(expr->left.get());
        size_t endJump = emitJump(OpCode::OP_JUMP_IF_FALSE, line);
        emitOp(OpCode::OP_POP, line);
        compileExpr(expr->right.get());
        patchJump(endJump);
        return;
    }

    // Short-circuit: ||
    if (op == "||") {
        compileExpr(expr->left.get());
        size_t elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE, line);
        size_t endJump = emitJump(OpCode::OP_JUMP, line);
        patchJump(elseJump);
        emitOp(OpCode::OP_POP, line);
        compileExpr(expr->right.get());
        patchJump(endJump);
        return;
    }

    // Normal binary: compile both sides then emit the operator
    compileExpr(expr->left.get());
    compileExpr(expr->right.get());

    if      (op == "+")  emitOp(OpCode::OP_ADD, line);
    else if (op == "-")  emitOp(OpCode::OP_SUB, line);
    else if (op == "*")  emitOp(OpCode::OP_MUL, line);
    else if (op == "/")  emitOp(OpCode::OP_DIV, line);
    else if (op == "%")  emitOp(OpCode::OP_MOD, line);
    else if (op == "==") emitOp(OpCode::OP_EQUAL, line);
    else if (op == "!=") emitOp(OpCode::OP_NOT_EQUAL, line);
    else if (op == "<")  emitOp(OpCode::OP_LESS, line);
    else if (op == "<=") emitOp(OpCode::OP_LESS_EQUAL, line);
    else if (op == ">")  emitOp(OpCode::OP_GREATER, line);
    else if (op == ">=") emitOp(OpCode::OP_GREATER_EQUAL, line);
    else error("unsupported binary operator '" + op + "'", line);
}

void Compiler::compileUnaryExpr(UnaryExpr* expr) {
    int line = expr->loc.line;
    const std::string& op = expr->op;

    // ── Postfix ++/-- on a variable ─────────────────────────────
    // e.g. i++ → get i, add 1, set i (leaves new value on stack)
    if (op == "post++" || op == "post--") {
        auto* var = dynamic_cast<VariableExpr*>(expr->operand.get());
        if (!var) { error("postfix ++/-- requires a variable", line); return; }

        int slot = resolveLocal(var->name);
        if (slot == -1) { error("undefined variable '" + var->name + "'", line); return; }

        emitOp(OpCode::OP_GET_LOCAL, line);
        emitByte(static_cast<uint8_t>(slot), line);
        emitConstant(int64_t{1}, line);
        emitOp(op == "post++" ? OpCode::OP_ADD : OpCode::OP_SUB, line);
        emitOp(OpCode::OP_SET_LOCAL, line);
        emitByte(static_cast<uint8_t>(slot), line);
        return;
    }

    // ── Prefix ++/-- ────────────────────────────────────────────
    if (op == "++" || op == "--") {
        auto* var = dynamic_cast<VariableExpr*>(expr->operand.get());
        if (!var) { error("prefix ++/-- requires a variable", line); return; }

        int slot = resolveLocal(var->name);
        if (slot == -1) { error("undefined variable '" + var->name + "'", line); return; }

        emitOp(OpCode::OP_GET_LOCAL, line);
        emitByte(static_cast<uint8_t>(slot), line);
        emitConstant(int64_t{1}, line);
        emitOp(op == "++" ? OpCode::OP_ADD : OpCode::OP_SUB, line);
        emitOp(OpCode::OP_SET_LOCAL, line);
        emitByte(static_cast<uint8_t>(slot), line);
        return;
    }

    // ── Prefix - (negate) and ! (not) ───────────────────────────
    compileExpr(expr->operand.get());
    if      (op == "-") emitOp(OpCode::OP_NEGATE, line);
    else if (op == "!") emitOp(OpCode::OP_NOT, line);
    else error("unsupported unary operator '" + op + "'", line);
}

void Compiler::compileAssignExpr(AssignExpr* expr) {
    int line = expr->loc.line;

    // Compile the value (pushes it onto the stack)
    compileExpr(expr->value.get());

    int slot = resolveLocal(expr->name);
    if (slot == -1) {
        error("undefined variable '" + expr->name + "'", line);
        return;
    }

    // SET_LOCAL copies TOS into the slot but does NOT pop.
    // (Assignment is an expression that produces the assigned value.)
    emitOp(OpCode::OP_SET_LOCAL, line);
    emitByte(static_cast<uint8_t>(slot), line);
}

void Compiler::compileCallExpr(CallExpr* expr) {
    int line = expr->loc.line;

    // Built-in: print(expr) → OP_PRINT
    if (expr->calleeName == "print") {
        if (expr->arguments.size() != 1) {
            error("print() takes exactly 1 argument", line);
            return;
        }
        compileExpr(expr->arguments[0].get());
        emitOp(OpCode::OP_PRINT, line);
        // OP_PRINT pops the value; push a dummy 0 so ExprStmt's
        // OP_POP has something to discard.
        emitConstant(int64_t{0}, line);
        return;
    }

    // Normal function call:
    //   1. Push each argument onto the stack.
    //   2. Emit OP_CALL [argCount] [nameConstIndex]
    for (auto& arg : expr->arguments) {
        compileExpr(arg.get());
    }

    // Store function name as a string constant
    uint8_t nameIdx = currentChunk().addConstant(expr->calleeName);

    emitOp(OpCode::OP_CALL, line);
    emitByte(static_cast<uint8_t>(expr->arguments.size()), line);
    emitByte(nameIdx, line);
}

// ════════════════════════════════════════════════════════════════
// Error reporting
// ════════════════════════════════════════════════════════════════

void Compiler::error(const std::string& message, int line) {
    hadError_ = true;
    std::cerr << "compile error (line " << line << "): " << message << "\n";
}
