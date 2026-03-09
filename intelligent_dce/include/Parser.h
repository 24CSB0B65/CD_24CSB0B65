#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include "ASTNode.h"

ASTNode* buildAST(std::vector<std::string> tokens);
void printAST(ASTNode* node, int level = 0);

#endif