#ifndef CFG_H
#define CFG_H

#include <vector>

std::vector<std::vector<int>> buildCFG(int n);
void dfs(int node, std::vector<std::vector<int>>& cfg, std::vector<bool>& vis);

#endif