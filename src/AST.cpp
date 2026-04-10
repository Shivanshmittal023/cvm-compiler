#include "AST.h"
#include <sstream>

static std::string getIndent(int indent) {
  return std::string(indent * 2, ' ');
}

// ────────────────────────────────────────────────────────────────
// Expressions
// ────────────────────────────────────────────────────────────────
std::string NumberExpr::dump(int indent) const {
  std::ostringstream oss;
  oss << getIndent(indent) << "NumberExpr(";
  if (std::holds_alternative<int64_t>(value)) {
    oss << std::get<int64_t>(value);
  } else {
    oss << std::get<double>(value);
  }
  oss << ")\n";
  return oss.str();
}

std::string BoolLitExpr::dump(int indent) const {
  return getIndent(indent) + "BoolLitExpr(" +
         (value ? "true" : "false") + ")\n";
}

std::string VariableExpr::dump(int indent) const {
  return getIndent(indent) + "VariableExpr('" + name + "')\n";
}

std::string StringExpr::dump(int indent) const {
  return getIndent(indent) + "StringExpr(\"" + value + "\")\n";
}

std::string UnaryExpr::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "UnaryExpr(operator: '" << op << "')\n";
  ss << operand->dump(indent + 1);
  return ss.str();
}

std::string BinaryExpr::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "BinaryExpr(operator: '" << op << "')\n";
  ss << left->dump(indent + 1);
  ss << right->dump(indent + 1);
  return ss.str();
}

std::string CallExpr::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "CallExpr(callee: '" << calleeName << "')\n";
  if (!arguments.empty()) {
    ss << getIndent(indent + 1) << "[Arguments]:\n";
    for (const auto &arg : arguments)
      ss << arg->dump(indent + 2);
  }
  return ss.str();
}

std::string AssignExpr::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "AssignExpr(variable: '" << name << "')\n";
  ss << value->dump(indent + 1);
  return ss.str();
}

// ────────────────────────────────────────────────────────────────
// Statements
// ────────────────────────────────────────────────────────────────
std::string ExprStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "ExprStmt\n";
  ss << expression->dump(indent + 1);
  return ss.str();
}

std::string BlockStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "BlockStmt[\n";
  for (const auto &stmt : statements)
    ss << stmt->dump(indent + 1);
  ss << getIndent(indent) << "]\n";
  return ss.str();
}

std::string ReturnStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "ReturnStmt\n";
  if (returnValue)
    ss << returnValue->dump(indent + 1);
  return ss.str();
}

std::string IfStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "IfStmt\n";
  ss << getIndent(indent + 1) << "[Condition]:\n"
     << condition->dump(indent + 2);
  ss << getIndent(indent + 1) << "[Then Branch]:\n"
     << thenBranch->dump(indent + 2);
  if (elseBranch) {
    ss << getIndent(indent + 1) << "[Else Branch]:\n"
       << elseBranch->dump(indent + 2);
  }
  return ss.str();
}

std::string WhileStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "WhileStmt\n";
  ss << getIndent(indent + 1) << "[Condition]:\n"
     << condition->dump(indent + 2);
  ss << getIndent(indent + 1) << "[Body]:\n" << body->dump(indent + 2);
  return ss.str();
}

std::string ForStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "ForStmt\n";
  if (initializer) {
    ss << getIndent(indent + 1) << "[Initializer]:\n"
       << initializer->dump(indent + 2);
  }
  if (condition) {
    ss << getIndent(indent + 1) << "[Condition]:\n"
       << condition->dump(indent + 2);
  }
  if (increment) {
    ss << getIndent(indent + 1) << "[Increment]:\n"
       << increment->dump(indent + 2);
  }
  ss << getIndent(indent + 1) << "[Body]:\n" << body->dump(indent + 2);
  return ss.str();
}

// ────────────────────────────────────────────────────────────────
// Declarations
// ────────────────────────────────────────────────────────────────
std::string VarDeclStmt::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "VarDeclStmt(type: '" << varType << "', name: '"
     << name << "')\n";
  if (initializer) {
    ss << getIndent(indent + 1) << "[Initialization]:\n"
       << initializer->dump(indent + 2);
  }
  return ss.str();
}

std::string FunctionDecl::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "FunctionDecl(returnType: '" << returnType
     << "', name: '" << name << "')\n";
  if (!parameters.empty()) {
    ss << getIndent(indent + 1) << "[Parameters]:\n";
    for (const auto &p : parameters) {
      ss << getIndent(indent + 2) << "Type: " << p.first
         << ", Name: " << p.second << "\n";
    }
  }
  ss << getIndent(indent + 1) << "[Body]:\n" << body->dump(indent + 2);
  return ss.str();
}

std::string Program::dump(int indent) const {
  std::stringstream ss;
  ss << getIndent(indent) << "--- COMPLETE PROGRAM AST ---\n";
  for (const auto &decl : declarations)
    ss << decl->dump(indent + 1);
  return ss.str();
}
