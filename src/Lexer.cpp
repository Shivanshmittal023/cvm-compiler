#include "Lexer.h"
#include <cctype>

Lexer::Lexer(std::string source) : source(std::move(source)) {}

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
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        int startColumn = column;
        int startLine = line;
        char c = peek();

        if (std::isalpha(c) || c == '_') {
            std::string lexeme;
            while (std::isalnum(peek()) || peek() == '_') {
                lexeme += advance();
            }
            TokenType type = TokenType::IDENTIFIER;

            if (lexeme == "int") type = TokenType::INT;
            else if (lexeme == "float") type = TokenType::FLOAT;
            else if (lexeme == "double") type = TokenType::DOUBLE;
            else if (lexeme == "char") type = TokenType::CHAR;
            else if (lexeme == "void") type = TokenType::VOID;
            else if (lexeme == "bool") type = TokenType::BOOL;
            else if (lexeme == "return") type = TokenType::RETURN;
            else if (lexeme == "if") type = TokenType::IF;
            else if (lexeme == "else") type = TokenType::ELSE;
            else if (lexeme == "while") type = TokenType::WHILE;
            else if (lexeme == "for") type = TokenType::FOR;
            else if (lexeme == "do") type = TokenType::DO;
            else if (lexeme == "class") type = TokenType::CLASS;
            else if (lexeme == "public") type = TokenType::PUBLIC;
            else if (lexeme == "private") type = TokenType::PRIVATE;
            else if (lexeme == "protected") type = TokenType::PROTECTED;
            else if (lexeme == "true") type = TokenType::TRUE_LITERAL;
            else if (lexeme == "false") type = TokenType::FALSE_LITERAL;

            tokens.push_back({type, lexeme, startLine, startColumn});
        }
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
            tokens.push_back({isFloat ? TokenType::NUMBER_FLOAT : TokenType::NUMBER_INT, lexeme, startLine, startColumn});
        }
        else if (c == '"') {
            std::string lexeme;
            advance(); // consume leading quote
            while (!isAtEnd() && peek() != '"') {
                if (peek() == '\\') {
                    lexeme += advance();
                }
                if (!isAtEnd()) lexeme += advance();
            }
            if (!isAtEnd()) advance(); // consume trailing quote
            tokens.push_back({TokenType::STRING_LITERAL, lexeme, startLine, startColumn});
        }
        else if (c == '\'') {
            std::string lexeme;
            advance(); // consume leading quote
            while (!isAtEnd() && peek() != '\'') {
                if (peek() == '\\') {
                    lexeme += advance();
                }
                if (!isAtEnd()) lexeme += advance();
            }
            if (!isAtEnd()) advance(); // consume trailing quote
            tokens.push_back({TokenType::CHAR_LITERAL, lexeme, startLine, startColumn});
        }
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
                    break;
                case '|':
                    if (match('|')) { type = TokenType::PIPE_PIPE; lexeme = "||"; }
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
            }
            tokens.push_back({type, lexeme, startLine, startColumn});
        }
    }
    
    tokens.push_back({TokenType::EOF_TOK, "EOF", line, column});
    return tokens;
}
