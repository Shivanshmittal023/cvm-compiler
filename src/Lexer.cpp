#include "Lexer.h"
#include <cctype>
#include <iostream>
#include <unordered_map>

// ────────────────────────────────────────────────────────────────
// O(1) keyword lookup instead of the if/else chain
// ────────────────────────────────────────────────────────────────
static const std::unordered_map<std::string, TokenType> keywords = {
    {"int",       TokenType::INT},
    {"float",     TokenType::FLOAT},
    {"double",    TokenType::DOUBLE},
    {"char",      TokenType::CHAR},
    {"void",      TokenType::VOID},
    {"bool",      TokenType::BOOL},
    {"return",    TokenType::RETURN},
    {"if",        TokenType::IF},
    {"else",      TokenType::ELSE},
    {"while",     TokenType::WHILE},
    {"for",       TokenType::FOR},
    {"do",        TokenType::DO},
    {"class",     TokenType::CLASS},
    {"public",    TokenType::PUBLIC},
    {"private",   TokenType::PRIVATE},
    {"protected", TokenType::PROTECTED},
    {"true",      TokenType::TRUE_LITERAL},
    {"false",     TokenType::FALSE_LITERAL},
};

// ────────────────────────────────────────────────────────────────
// Constructor & helpers
// ────────────────────────────────────────────────────────────────
Lexer::Lexer(std::string source, std::string filename)
    : source(std::move(source)), filename(std::move(filename)) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[position];
}

char Lexer::peekNext() const {
    if (position + 1 >= source.length()) return '\0';
    return source[position + 1];
}

char Lexer::advance() {
    char c = source[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[position] != expected) return false;
    position++;
    column++;
    return true;
}

bool Lexer::isAtEnd() const {
    return position >= source.length();
}

void Lexer::reportError(int errLine, int errCol, const std::string& message) {
    hadError = true;
    std::cerr << filename << ":" << errLine << ":" << errCol
              << ": error: " << message << "\n";
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
        } else if (c == '/') {
            if (peekNext() == '/') {
                // A single-line comment goes until the end of the line.
                while (!isAtEnd() && peek() != '\n') advance();
            } else if (peekNext() == '*') {
                int commentLine = line;
                int commentCol = column;
                // Multi-line comment
                advance(); // consume '/'
                advance(); // consume '*'
                while (!isAtEnd()) {
                    if (peek() == '*' && peekNext() == '/') {
                        advance(); // consume '*'
                        advance(); // consume '/'
                        break;
                    }
                    advance();
                }
                // Check for unterminated block comment
                if (isAtEnd() && !(source.length() >= 2 &&
                    source[source.length() - 2] == '*' &&
                    source[source.length() - 1] == '/')) {
                    reportError(commentLine, commentCol,
                                "unterminated block comment");
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

// ────────────────────────────────────────────────────────────────
// Main tokenize loop
// ────────────────────────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        int startColumn = column;
        int startLine = line;
        char c = peek();

        // ── Identifiers & Keywords ──────────────────────────────
        if (std::isalpha(c) || c == '_') {
            std::string lexeme;
            while (std::isalnum(peek()) || peek() == '_') {
                lexeme += advance();
            }

            // O(1) keyword lookup
            auto it = keywords.find(lexeme);
            TokenType type = (it != keywords.end()) ? it->second
                                                    : TokenType::IDENTIFIER;

            tokens.push_back({type, lexeme, filename, startLine, startColumn});
        }
        // ── Number literals ─────────────────────────────────────
        else if (std::isdigit(c)) {
            std::string lexeme;
            bool isFloat = false;
            while (std::isdigit(peek())) {
                lexeme += advance();
            }
            if (peek() == '.' && std::isdigit(peekNext())) {
                isFloat = true;
                lexeme += advance(); // Consume '.'
                while (std::isdigit(peek())) {
                    lexeme += advance();
                }
            }
            tokens.push_back({isFloat ? TokenType::NUMBER_FLOAT
                                      : TokenType::NUMBER_INT,
                              lexeme, filename, startLine, startColumn});
        }
        // ── String literals (with proper escape decoding) ───────
        else if (c == '"') {
            std::string lexeme;
            advance(); // consume leading quote
            while (!isAtEnd() && peek() != '"') {
                if (peek() == '\\') {
                    advance(); // consume '\'
                    if (isAtEnd()) break;
                    char escaped = advance();
                    switch (escaped) {
                        case 'n':  lexeme += '\n'; break;
                        case 't':  lexeme += '\t'; break;
                        case 'r':  lexeme += '\r'; break;
                        case '\\': lexeme += '\\'; break;
                        case '"':  lexeme += '"';  break;
                        case '0':  lexeme += '\0'; break;
                        default:   lexeme += escaped; break;
                    }
                } else {
                    lexeme += advance();
                }
            }
            if (isAtEnd()) {
                reportError(startLine, startColumn,
                            "unterminated string literal");
                tokens.push_back({TokenType::UNKNOWN, lexeme, filename,
                                  startLine, startColumn});
            } else {
                advance(); // consume trailing quote
                tokens.push_back({TokenType::STRING_LITERAL, lexeme, filename,
                                  startLine, startColumn});
            }
        }
        // ── Char literals (with proper escape decoding) ─────────
        else if (c == '\'') {
            std::string lexeme;
            advance(); // consume leading quote
            while (!isAtEnd() && peek() != '\'') {
                if (peek() == '\\') {
                    advance(); // consume '\'
                    if (isAtEnd()) break;
                    char escaped = advance();
                    switch (escaped) {
                        case 'n':  lexeme += '\n'; break;
                        case 't':  lexeme += '\t'; break;
                        case 'r':  lexeme += '\r'; break;
                        case '\\': lexeme += '\\'; break;
                        case '\'': lexeme += '\''; break;
                        case '0':  lexeme += '\0'; break;
                        default:   lexeme += escaped; break;
                    }
                } else {
                    lexeme += advance();
                }
            }
            if (isAtEnd()) {
                reportError(startLine, startColumn,
                            "unterminated character literal");
                tokens.push_back({TokenType::UNKNOWN, lexeme, filename,
                                  startLine, startColumn});
            } else {
                advance(); // consume trailing quote
                tokens.push_back({TokenType::CHAR_LITERAL, lexeme, filename,
                                  startLine, startColumn});
            }
        }
        // ── Operators & symbols ─────────────────────────────────
        else {
            advance(); // consume the symbol
            TokenType type = TokenType::UNKNOWN;
            std::string lexeme(1, c);

            switch (c) {
                case '=':
                    if (match('=')) { type = TokenType::EQUALS_EQUALS; lexeme = "=="; }
                    else { type = TokenType::EQUALS; }
                    break;
                case '!':
                    if (match('=')) { type = TokenType::BANG_EQUALS; lexeme = "!="; }
                    else { type = TokenType::BANG; }
                    break;
                case '<':
                    if (match('=')) { type = TokenType::LESS_EQUALS; lexeme = "<="; }
                    else if (match('<')) { type = TokenType::LESS_LESS; lexeme = "<<"; }
                    else { type = TokenType::LESS; }
                    break;
                case '>':
                    if (match('=')) { type = TokenType::GREATER_EQUALS; lexeme = ">="; }
                    else if (match('>')) { type = TokenType::GREATER_GREATER; lexeme = ">>"; }
                    else { type = TokenType::GREATER; }
                    break;
                case '+':
                    if (match('+')) { type = TokenType::PLUS_PLUS; lexeme = "++"; }
                    else if (match('=')) { type = TokenType::PLUS_EQUALS; lexeme = "+="; }
                    else { type = TokenType::PLUS; }
                    break;
                case '-':
                    if (match('-')) { type = TokenType::MINUS_MINUS; lexeme = "--"; }
                    else if (match('=')) { type = TokenType::MINUS_EQUALS; lexeme = "-="; }
                    else { type = TokenType::MINUS; }
                    break;
                case '*':
                    if (match('=')) { type = TokenType::STAR_EQUALS; lexeme = "*="; }
                    else { type = TokenType::STAR; }
                    break;
                case '/':
                    if (match('=')) { type = TokenType::SLASH_EQUALS; lexeme = "/="; }
                    else { type = TokenType::SLASH; }
                    break;
                case '&':
                    if (match('&')) { type = TokenType::AMPERSAND_AMPERSAND; lexeme = "&&"; }
                    else { type = TokenType::AMPERSAND; }
                    break;
                case '|':
                    if (match('|')) { type = TokenType::PIPE_PIPE; lexeme = "||"; }
                    else { type = TokenType::PIPE; }
                    break;
                case '%': type = TokenType::PERCENT; break;
                case ';': type = TokenType::SEMICOLON; break;
                case ':': type = TokenType::COLON; break;
                case ',': type = TokenType::COMMA; break;
                case '.': type = TokenType::DOT; break;
                case '(': type = TokenType::LPAREN; break;
                case ')': type = TokenType::RPAREN; break;
                case '{': type = TokenType::LBRACE; break;
                case '}': type = TokenType::RBRACE; break;
                case '[': type = TokenType::LBRACKET; break;
                case ']': type = TokenType::RBRACKET; break;
                default:
                    reportError(startLine, startColumn,
                                "unexpected character '" + std::string(1, c) + "'");
                    break;
            }
            tokens.push_back({type, lexeme, filename, startLine, startColumn});
        }
    }
    
    tokens.push_back({TokenType::EOF_TOK, "EOF", filename, line, column});
    return tokens;
}
