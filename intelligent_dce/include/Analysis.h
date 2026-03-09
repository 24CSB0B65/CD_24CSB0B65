#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <vector>
#include "ASTNode.h"

void livenessAnalysis(std::vector<ASTNode*> stmts);
void deadCodeElimination(std::vector<ASTNode*>& stmts);

#endif