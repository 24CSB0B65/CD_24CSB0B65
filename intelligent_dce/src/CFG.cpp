#include "CFG.h"

using namespace std;

vector<vector<int>> buildCFG(int n) {

    vector<vector<int>> cfg(n);

    for (int i = 0; i < n - 1; i++)
        cfg[i].push_back(i + 1);

    return cfg;
}

void dfs(int node, vector<vector<int>>& cfg, vector<bool>& vis) {

    vis[node] = true;

    for (auto x : cfg[node])
        if (!vis[x])
            dfs(x, cfg, vis);
}