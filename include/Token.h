#pragma once
#include <string>

enum class TokenType {
    // Keywords
    INT, FLOAT, DOUBLE, CHAR, VOID, BOOL,
    RETURN, IF, ELSE, WHILE, FOR, DO,
    CLASS, PUBLIC, PRIVATE, PROTECTED,
    TRUE_LITERAL, FALSE_LITERAL,

    // Identifiers and Literals
    IDENTIFIER,
    NUMBER_INT,
    NUMBER_FLOAT,
    STRING_LITERAL,
    CHAR_LITERAL,

    // Operators
    EQUALS,         // =
    EQUALS_EQUALS,  // ==
    BANG,           // !
    BANG_EQUALS,    // !=
    LESS,           // <
    LESS_EQUALS,    // <=
    LESS_LESS,      // <<
    GREATER,        // >
    GREATER_EQUALS, // >=
    GREATER_GREATER, // >>

    PLUS,           // +
    PLUS_PLUS,      // ++
    PLUS_EQUALS,    // +=
    MINUS,          // -
    MINUS_MINUS,    // --
    MINUS_EQUALS,   // -=
    STAR,           // *
    STAR_EQUALS,    // *=
    SLASH,          // /
    SLASH_EQUALS,   // /=
    PERCENT,        // %

    AMPERSAND,           // &   (address-of / bitwise AND)
    AMPERSAND_AMPERSAND, // &&
    PIPE,                // |   (bitwise OR)
    PIPE_PIPE,           // ||
    
    // Symbols
    SEMICOLON, // ;
    COLON,     // :
    COMMA,     // ,
    DOT,       // .
    LPAREN,    // (
    RPAREN,    // )
    LBRACE,    // {
    RBRACE,    // }
    LBRACKET,  // [
    RBRACKET,  // ]

    // End of file / Error
    EOF_TOK,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::string filename;  // source file this token came from
    int line;
    int column;
};

std::string tokenTypeToString(TokenType type);
