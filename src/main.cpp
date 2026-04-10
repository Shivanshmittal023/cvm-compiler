#include <iostream>
#include <memory>
#include "Lexer.h"
#include "AST.h"

int main() {
    std::cout << "Testing MASSIVE Production AST Framework...\n\n";
    
    // We are manually building the exact AST for this highly complex C++ snippet:
    // int main() {
    //    return 2 + 3 * 4;
    // }
    
    auto num3 = std::make_unique<NumberExpr>(3.0);
    auto num4 = std::make_unique<NumberExpr>(4.0);
    auto mulNode = std::make_unique<BinaryExpr>("*", std::move(num3), std::move(num4));
    
    auto num2 = std::make_unique<NumberExpr>(2.0);
    auto addNode = std::make_unique<BinaryExpr>("+", std::move(num2), std::move(mulNode));
    
    auto returnNode = std::make_unique<ReturnStmt>(std::move(addNode));
    
    std::vector<std::unique_ptr<Stmt>> blockStmts;
    blockStmts.push_back(std::move(returnNode));
    auto body = std::make_unique<BlockStmt>(std::move(blockStmts));
    
    std::vector<std::pair<std::string, std::string>> params; // Empty parameters
    auto mainFunc = std::make_unique<FunctionDecl>("int", "main", std::move(params), std::move(body));
    
    std::vector<std::unique_ptr<Stmt>> fileStatements;
    fileStatements.push_back(std::move(mainFunc));
    
    auto programNode = std::make_unique<Program>(std::move(fileStatements));
    
    std::cout << programNode->dump() << std::endl;

    return 0;
}
