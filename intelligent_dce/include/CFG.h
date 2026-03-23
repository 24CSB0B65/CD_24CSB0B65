#ifndef CFG_H
#define CFG_H

#include <vector>
#include "ASTNode.h"

// Original linear CFG — kept for backward compatibility
std::vector<std::vector<int>> buildCFG(int n);

// Statement-aware CFG: builds proper back-edges for FOR loops
std::vector<std::vector<int>> buildCFGFromStmts(const std::vector<ASTNode*>& stmts);

void dfs(int node, std::vector<std::vector<int>>& cfg, std::vector<bool>& vis);

#endif