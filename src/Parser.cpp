#include "Parser.h"
#include <iostream>

// ════════════════════════════════════════════════════════════════════
// Construction
// ════════════════════════════════════════════════════════════════════

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

// ════════════════════════════════════════════════════════════════════
// Token stream navigation
// ════════════════════════════════════════════════════════════════════

const Token& Parser::peek() const { return tokens_[current_]; }

const Token& Parser::previous() const { return tokens_[current_ - 1]; }

const Token& Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOK;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw errorAt(peek(), message);
}

// ════════════════════════════════════════════════════════════════════
// Error reporting & panic-mode recovery
// ════════════════════════════════════════════════════════════════════

Parser::ParseError Parser::errorAt(const Token& token, const std::string& message) {
    hadError_ = true;
    std::cerr << token.filename << ":" << token.line << ":" << token.column
              << ": error: " << message;
    if (token.type == TokenType::EOF_TOK)
        std::cerr << " at end of file";
    else
        std::cerr << " near '" << token.lexeme << "'";
    std::cerr << "\n";
    return ParseError{};
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
            case TokenType::INT:    case TokenType::FLOAT:
            case TokenType::DOUBLE: case TokenType::CHAR:
            case TokenType::VOID:   case TokenType::BOOL:
            case TokenType::IF:     case TokenType::WHILE:
            case TokenType::FOR:    case TokenType::RETURN:
                return;
            default: break;
        }
        advance();
    }
}

// ════════════════════════════════════════════════════════════════════
// Helpers
// ════════════════════════════════════════════════════════════════════

bool Parser::isTypeKeyword(TokenType type) const {
    switch (type) {
        case TokenType::INT:    case TokenType::FLOAT:
        case TokenType::DOUBLE: case TokenType::CHAR:
        case TokenType::VOID:   case TokenType::BOOL:
            return true;
        default: return false;
    }
}

SourceLocation Parser::makeLoc(const Token& tok) const {
    return {tok.filename, tok.line, tok.column};
}

// ════════════════════════════════════════════════════════════════════
// Top-level parsing
// ════════════════════════════════════════════════════════════════════

std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> declarations;
    while (!isAtEnd()) {
        auto decl = parseDeclaration();
        if (decl) declarations.push_back(std::move(decl));
    }
    auto prog = std::make_unique<Program>(std::move(declarations));
    if (!tokens_.empty()) prog->loc = makeLoc(tokens_[0]);
    return prog;
}

std::unique_ptr<Stmt> Parser::parseDeclaration() {
    try {
        if (isTypeKeyword(peek().type)) {
            Token typeTok = advance();
            Token nameTok = consume(TokenType::IDENTIFIER,
                "expected identifier after type '" + typeTok.lexeme + "'");
            if (check(TokenType::LPAREN))
                return parseFunctionDecl(typeTok.lexeme, nameTok.lexeme, makeLoc(typeTok));
            return parseVarDecl(typeTok.lexeme, nameTok.lexeme, makeLoc(typeTok));
        }
        return parseStatement();
    } catch (const ParseError&) {
        synchronize();
        return nullptr;
    }
}

// ════════════════════════════════════════════════════════════════════
// Declarations
// ════════════════════════════════════════════════════════════════════

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl(
    const std::string& retType, const std::string& name, SourceLocation loc) {

    consume(TokenType::LPAREN, "expected '(' after function name");

    std::vector<std::pair<std::string, std::string>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            if (!isTypeKeyword(peek().type))
                throw errorAt(peek(), "expected parameter type");
            std::string pType = advance().lexeme;
            std::string pName = consume(TokenType::IDENTIFIER,
                "expected parameter name").lexeme;
            params.emplace_back(pType, pName);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "expected ')' after parameters");

    auto body = parseBlock();

    auto func = std::make_unique<FunctionDecl>(
        retType, name, std::move(params), std::move(body));
    func->loc = loc;
    return func;
}

std::unique_ptr<VarDeclStmt> Parser::parseVarDecl(
    const std::string& varType, const std::string& name, SourceLocation loc) {

    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenType::EQUALS))
        init = parseExpression();

    consume(TokenType::SEMICOLON, "expected ';' after variable declaration");

    auto decl = std::make_unique<VarDeclStmt>(varType, name, std::move(init));
    decl->loc = loc;
    return decl;
}

// ════════════════════════════════════════════════════════════════════
// Statements
// ════════════════════════════════════════════════════════════════════

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenType::LBRACE))  return parseBlock();
    if (match(TokenType::IF))      return parseIfStmt();
    if (match(TokenType::WHILE))   return parseWhileStmt();
    if (match(TokenType::FOR))     return parseForStmt();
    if (match(TokenType::RETURN))  return parseReturnStmt();
    return parseExprStmt();
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    SourceLocation loc = makeLoc(peek());
    consume(TokenType::LBRACE, "expected '{'");

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto decl = parseDeclaration();
        if (decl) stmts.push_back(std::move(decl));
    }
    consume(TokenType::RBRACE, "expected '}'");

    auto block = std::make_unique<BlockStmt>(std::move(stmts));
    block->loc = loc;
    return block;
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    SourceLocation loc = makeLoc(previous());
    consume(TokenType::LPAREN, "expected '(' after 'if'");
    auto cond = parseExpression();
    consume(TokenType::RPAREN, "expected ')' after if condition");

    auto thenBranch = parseStatement();
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::ELSE))
        elseBranch = parseStatement();

    auto node = std::make_unique<IfStmt>(
        std::move(cond), std::move(thenBranch), std::move(elseBranch));
    node->loc = loc;
    return node;
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    SourceLocation loc = makeLoc(previous());
    consume(TokenType::LPAREN, "expected '(' after 'while'");
    auto cond = parseExpression();
    consume(TokenType::RPAREN, "expected ')' after while condition");

    auto body = parseStatement();

    auto node = std::make_unique<WhileStmt>(std::move(cond), std::move(body));
    node->loc = loc;
    return node;
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    SourceLocation loc = makeLoc(previous());
    consume(TokenType::LPAREN, "expected '(' after 'for'");

    // ── initializer ─────────────────────────────────────────────
    std::unique_ptr<Stmt> init;
    if (match(TokenType::SEMICOLON)) {
        init = nullptr;
    } else if (isTypeKeyword(peek().type)) {
        Token typeTok = advance();
        Token nameTok = consume(TokenType::IDENTIFIER,
            "expected variable name in for initializer");
        init = parseVarDecl(typeTok.lexeme, nameTok.lexeme, makeLoc(typeTok));
    } else {
        auto expr = parseExpression();
        consume(TokenType::SEMICOLON, "expected ';' after for initializer");
        auto s = std::make_unique<ExprStmt>(std::move(expr));
        s->loc = loc;
        init = std::move(s);
    }

    // ── condition ───────────────────────────────────────────────
    std::unique_ptr<Expr> cond;
    if (!check(TokenType::SEMICOLON))
        cond = parseExpression();
    consume(TokenType::SEMICOLON, "expected ';' after for condition");

    // ── increment ───────────────────────────────────────────────
    std::unique_ptr<Expr> inc;
    if (!check(TokenType::RPAREN))
        inc = parseExpression();
    consume(TokenType::RPAREN, "expected ')' after for clauses");

    auto body = parseStatement();

    auto node = std::make_unique<ForStmt>(
        std::move(init), std::move(cond), std::move(inc), std::move(body));
    node->loc = loc;
    return node;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    SourceLocation loc = makeLoc(previous());
    std::unique_ptr<Expr> value;
    if (!check(TokenType::SEMICOLON))
        value = parseExpression();
    consume(TokenType::SEMICOLON, "expected ';' after return value");

    auto node = std::make_unique<ReturnStmt>(std::move(value));
    node->loc = loc;
    return node;
}

std::unique_ptr<ExprStmt> Parser::parseExprStmt() {
    SourceLocation loc = makeLoc(peek());
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "expected ';' after expression");

    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->loc = loc;
    return node;
}

// ════════════════════════════════════════════════════════════════════
// Expressions — Pratt-style precedence climbing
//
//   Precedence table (low → high):
//     1  ||
//     2  &&
//     3  ==  !=
//     4  <  <=  >  >=
//     5  <<  >>
//     6  +  -
//     7  *  /  %
// ════════════════════════════════════════════════════════════════════

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseBinary(1);

    if (match(TokenType::EQUALS)) {
        SourceLocation loc = makeLoc(previous());
        auto value = parseAssignment(); // right-associative

        if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
            auto node = std::make_unique<AssignExpr>(var->name, std::move(value));
            node->loc = loc;
            return node;
        }
        throw errorAt(previous(), "invalid assignment target");
    }
    return expr;
}

int Parser::getOperatorPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::PIPE_PIPE:           return 1;
        case TokenType::AMPERSAND_AMPERSAND: return 2;
        case TokenType::EQUALS_EQUALS:
        case TokenType::BANG_EQUALS:         return 3;
        case TokenType::LESS:
        case TokenType::LESS_EQUALS:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUALS:      return 4;
        case TokenType::LESS_LESS:
        case TokenType::GREATER_GREATER:     return 5;
        case TokenType::PLUS:
        case TokenType::MINUS:               return 6;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:             return 7;
        default:                             return 0;
    }
}

std::unique_ptr<Expr> Parser::parseBinary(int minPrecedence) {
    auto left = parseUnary();

    while (true) {
        int prec = getOperatorPrecedence(peek().type);
        if (prec < minPrecedence) break;

        Token op = advance();
        auto right = parseBinary(prec + 1); // left-associative

        auto node = std::make_unique<BinaryExpr>(
            op.lexeme, std::move(left), std::move(right));
        node->loc = makeLoc(op);
        left = std::move(node);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::BANG) || match(TokenType::MINUS) ||
        match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
        Token op = previous();
        auto operand = parseUnary();
        auto node = std::make_unique<UnaryExpr>(op.lexeme, std::move(operand));
        node->loc = makeLoc(op);
        return node;
    }
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parseCall();

    if (match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
        Token op = previous();
        auto node = std::make_unique<UnaryExpr>(
            "post" + op.lexeme, std::move(expr));
        node->loc = makeLoc(op);
        return node;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseCall() {
    auto expr = parsePrimary();

    while (check(TokenType::LPAREN)) {
        auto* var = dynamic_cast<VariableExpr*>(expr.get());
        if (!var) throw errorAt(peek(), "expression is not callable");

        std::string name = var->name;
        SourceLocation loc = expr->loc;
        advance(); // consume '('

        std::vector<std::unique_ptr<Expr>> args;
        if (!check(TokenType::RPAREN)) {
            args.push_back(parseExpression());
            while (match(TokenType::COMMA))
                args.push_back(parseExpression());
        }
        consume(TokenType::RPAREN, "expected ')' after arguments");

        auto node = std::make_unique<CallExpr>(name, std::move(args));
        node->loc = loc;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    // Integer literal
    if (match(TokenType::NUMBER_INT)) {
        Token tok = previous();
        auto node = std::make_unique<NumberExpr>(std::stoll(tok.lexeme));
        node->loc = makeLoc(tok);
        return node;
    }
    // Float literal
    if (match(TokenType::NUMBER_FLOAT)) {
        Token tok = previous();
        auto node = std::make_unique<NumberExpr>(std::stod(tok.lexeme));
        node->loc = makeLoc(tok);
        return node;
    }
    // String literal
    if (match(TokenType::STRING_LITERAL)) {
        Token tok = previous();
        auto node = std::make_unique<StringExpr>(tok.lexeme);
        node->loc = makeLoc(tok);
        return node;
    }
    // Char literal
    if (match(TokenType::CHAR_LITERAL)) {
        Token tok = previous();
        auto node = std::make_unique<StringExpr>(tok.lexeme);
        node->loc = makeLoc(tok);
        return node;
    }
    // Boolean literals
    if (match(TokenType::TRUE_LITERAL)) {
        Token tok = previous();
        auto node = std::make_unique<BoolLitExpr>(true);
        node->loc = makeLoc(tok);
        return node;
    }
    if (match(TokenType::FALSE_LITERAL)) {
        Token tok = previous();
        auto node = std::make_unique<BoolLitExpr>(false);
        node->loc = makeLoc(tok);
        return node;
    }
    // Identifier
    if (match(TokenType::IDENTIFIER)) {
        Token tok = previous();
        auto node = std::make_unique<VariableExpr>(tok.lexeme);
        node->loc = makeLoc(tok);
        return node;
    }
    // Grouped expression
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "expected ')' after expression");
        return expr;
    }

    throw errorAt(peek(), "expected expression");
}
