#pragma once
#include "Token.h"
#include <string>
#include <vector>

class Lexer {
public:
    Lexer(std::string source, std::string filename = "<stdin>");
    std::vector<Token> tokenize();

private:
    std::string source;
    std::string filename;
    size_t position = 0;
    int line = 1;
    int column = 1;

    // Track whether we've encountered any lexer errors
    bool hadError = false;

    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;
    void skipWhitespaceAndComments();
    void reportError(int errLine, int errCol, const std::string& message);
};
