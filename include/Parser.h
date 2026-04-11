#pragma once
#include "AST.h"
#include "Token.h"
#include <memory>
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    /// Parse the entire token stream into a Program AST.
    std::unique_ptr<Program> parse();
    bool hadError() const { return hadError_; }

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;
    bool hadError_ = false;

    // ── Lightweight sentinel thrown to unwind on syntax errors ───
    struct ParseError {};

    // ── Token stream navigation ─────────────────────────────────
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const std::string& message);

    // ── Error reporting & recovery ──────────────────────────────
    ParseError errorAt(const Token& token, const std::string& message);
    void synchronize();

    // ── Helpers ─────────────────────────────────────────────────
    bool isTypeKeyword(TokenType type) const;
    SourceLocation makeLoc(const Token& tok) const;

    // ── Top-level declarations ──────────────────────────────────
    std::unique_ptr<Stmt> parseDeclaration();
    std::unique_ptr<FunctionDecl> parseFunctionDecl(
        const std::string& retType, const std::string& name, SourceLocation loc);
    std::unique_ptr<VarDeclStmt> parseVarDecl(
        const std::string& varType, const std::string& name, SourceLocation loc);

    // ── Statements ──────────────────────────────────────────────
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<ForStmt> parseForStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<ExprStmt> parseExprStmt();

    // ── Expressions (Pratt-style precedence climbing) ───────────
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseBinary(int minPrecedence);
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parseCall();
    std::unique_ptr<Expr> parsePrimary();

    int getOperatorPrecedence(TokenType type) const;
};
