#include <iostream>
#include <vector>
#include <string>
#include "Lexer.h"
#include "Token.h"

int main() {
    std::cout << "Testing Production Lexer Frontend...\n\n";
    
    // A complex C++ snippet containing math, logic, strings, and comments.
    std::string sourceCode = R"(
        // This is a comprehensive C++ snippet to stress test the lexer
        int main() {
            float pi = 3.14159;
            if (pi >= 3.0 && pi != 4.0) {
                std::cout << "Pi is roughly three!" << std::endl;
            }
            
            /* 
              Multi-line comment block
            */
            for(int i = 0; i < 10; i++) {
                pi += 1.0;
            }
            
            return 0;
        }
    )";
    
    std::cout << "Source Code:\n" << sourceCode << "\n\n";
    std::cout << "Generated Lexical Tokens:\n";
    
    // 1. Initialize the Lexer
    Lexer lexer(sourceCode);
    
    // 2. Run the Engine
    std::vector<Token> tokens = lexer.tokenize();
    
    // 3. Print out the results
    for (const auto& token : tokens) {
        std::cout << "Line " << token.line << ", Col " << token.column << " : ";
        std::cout << tokenTypeToString(token.type) << " ('" << token.lexeme << "')\n";
    }
    
    return 0;
}
