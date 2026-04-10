#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// ────────────────────────────────────────────────────────────────
// SOURCE LOCATION — every AST node tracks where it came from
// ────────────────────────────────────────────────────────────────
struct SourceLocation {
  std::string filename;
  int line = 0;
  int column = 0;
};

// ────────────────────────────────────────────────────────────────
// INTERFACES
// ────────────────────────────────────────────────────────────────
class ASTNode {
public:
  SourceLocation loc; // every node knows its origin in source

  virtual ~ASTNode() = default;
  virtual std::string dump(int indent = 0) const = 0;
};

class Expr : public ASTNode {
public:
  virtual ~Expr() = default;
};

class Stmt : public ASTNode {
public:
  virtual ~Stmt() = default;
};

// ────────────────────────────────────────────────────────────────
// EXPRESSIONS (Things that output a runtime value)
// ────────────────────────────────────────────────────────────────

// Stores integer or floating-point literals with full precision.
// Uses std::variant so LLVM IR generation can emit i32 vs f64 correctly.
class NumberExpr : public Expr {
public:
  std::variant<int64_t, double> value;

  explicit NumberExpr(int64_t val) : value(val) {}
  explicit NumberExpr(double val) : value(val) {}
  std::string dump(int indent = 0) const override;
};

class BoolLitExpr : public Expr {
public:
  bool value;
  explicit BoolLitExpr(bool val) : value(val) {}
  std::string dump(int indent = 0) const override;
};

class VariableExpr : public Expr {
public:
  std::string name;
  explicit VariableExpr(std::string name) : name(std::move(name)) {}
  std::string dump(int indent = 0) const override;
};

class StringExpr : public Expr {
public:
  std::string value;
  explicit StringExpr(std::string val) : value(std::move(val)) {}
  std::string dump(int indent = 0) const override;
};

class UnaryExpr : public Expr {
public:
  std::string op; // "!", "-", "++", "--"
  std::unique_ptr<Expr> operand;
  UnaryExpr(std::string op, std::unique_ptr<Expr> operand)
      : op(std::move(op)), operand(std::move(operand)) {}
  std::string dump(int indent = 0) const override;
};

class BinaryExpr : public Expr {
public:
  std::string op;
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;
  BinaryExpr(std::string op, std::unique_ptr<Expr> left,
             std::unique_ptr<Expr> right)
      : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}
  std::string dump(int indent = 0) const override;
};

class CallExpr : public Expr {
public:
  std::string calleeName;
  std::vector<std::unique_ptr<Expr>> arguments;
  CallExpr(std::string calleeName, std::vector<std::unique_ptr<Expr>> arguments)
      : calleeName(std::move(calleeName)), arguments(std::move(arguments)) {}
  std::string dump(int indent = 0) const override;
};

class AssignExpr : public Expr {
public:
  std::string name;
  std::unique_ptr<Expr> value;
  AssignExpr(std::string name, std::unique_ptr<Expr> val)
      : name(std::move(name)), value(std::move(val)) {}
  std::string dump(int indent = 0) const override;
};

// ────────────────────────────────────────────────────────────────
// STATEMENTS (Things that execute control flow logic)
// ────────────────────────────────────────────────────────────────
class ExprStmt : public Stmt {
public:
  std::unique_ptr<Expr> expression;
  explicit ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}
  std::string dump(int indent = 0) const override;
};

class BlockStmt : public Stmt {
public:
  std::vector<std::unique_ptr<Stmt>> statements;
  explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts)
      : statements(std::move(stmts)) {}
  std::string dump(int indent = 0) const override;
};

class ReturnStmt : public Stmt {
public:
  std::unique_ptr<Expr> returnValue;
  explicit ReturnStmt(std::unique_ptr<Expr> returnValue)
      : returnValue(std::move(returnValue)) {}
  std::string dump(int indent = 0) const override;
};

class IfStmt : public Stmt {
public:
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> thenBranch;
  std::unique_ptr<Stmt> elseBranch;
  IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch,
         std::unique_ptr<Stmt> elseBranch)
      : condition(std::move(condition)), thenBranch(std::move(thenBranch)),
        elseBranch(std::move(elseBranch)) {}
  std::string dump(int indent = 0) const override;
};

class WhileStmt : public Stmt {
public:
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> body;
  WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
      : condition(std::move(condition)), body(std::move(body)) {}
  std::string dump(int indent = 0) const override;
};

class ForStmt : public Stmt {
public:
  std::unique_ptr<Stmt> initializer; // int i = 0; or i = 0;
  std::unique_ptr<Expr> condition;   // i < 10
  std::unique_ptr<Expr> increment;   // i++
  std::unique_ptr<Stmt> body;
  ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond,
          std::unique_ptr<Expr> inc, std::unique_ptr<Stmt> body)
      : initializer(std::move(init)), condition(std::move(cond)),
        increment(std::move(inc)), body(std::move(body)) {}
  std::string dump(int indent = 0) const override;
};

// ────────────────────────────────────────────────────────────────
// DECLARATIONS (Making variables and functions)
// ────────────────────────────────────────────────────────────────
class VarDeclStmt : public Stmt {
public:
  std::string varType;
  std::string name;
  std::unique_ptr<Expr> initializer;
  VarDeclStmt(std::string varType, std::string name, std::unique_ptr<Expr> init)
      : varType(std::move(varType)), name(std::move(name)),
        initializer(std::move(init)) {}
  std::string dump(int indent = 0) const override;
};

class FunctionDecl : public Stmt {
public:
  std::string returnType;
  std::string name;
  std::vector<std::pair<std::string, std::string>> parameters; // <Type, Name>
  std::unique_ptr<BlockStmt> body;
  FunctionDecl(std::string returnType, std::string name,
               std::vector<std::pair<std::string, std::string>> parameters,
               std::unique_ptr<BlockStmt> body)
      : returnType(std::move(returnType)), name(std::move(name)),
        parameters(std::move(parameters)), body(std::move(body)) {}
  std::string dump(int indent = 0) const override;
};

// ────────────────────────────────────────────────────────────────
// THE TOP LEVEL PROGRAM
// ────────────────────────────────────────────────────────────────
class Program : public ASTNode {
public:
  std::vector<std::unique_ptr<Stmt>> declarations;
  explicit Program(std::vector<std::unique_ptr<Stmt>> decls)
      : declarations(std::move(decls)) {}
  std::string dump(int indent = 0) const override;
};
