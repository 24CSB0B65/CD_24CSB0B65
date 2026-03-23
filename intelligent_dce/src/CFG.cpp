#include "CFG.h"
#include <functional>

using namespace std;

// ── Original linear CFG (kept for backward compatibility) ────────────────────
vector<vector<int>> buildCFG(int n)
{
    vector<vector<int>> cfg(n);
    for (int i = 0; i < n - 1; i++)
        cfg[i].push_back(i + 1);
    return cfg;
}

// ── Helpers for statement-aware CFG ─────────────────────────────────────────

// Count total "atomic" nodes: every DECL/RETURN is 1, every FOR header is 1,
// plus all body nodes recursively.
static int countNodes(const vector<ASTNode*>& stmts)
{
    int count = 0;
    for (ASTNode* s : stmts)
    {
        count++;   // every statement (including FOR header) is one node
        if (s->type == "FOR")
            count += countNodes(s->children[3]->children);
    }
    return count;
}

// Add edges.  `pos` tracks the running node index (passed by reference).
static void buildEdges(const vector<ASTNode*>& stmts,
                        vector<vector<int>>&     cfg,
                        int&                     pos)
{
    int n = (int)cfg.size();

    for (ASTNode* s : stmts)
    {
        int cur = pos++;

        if (s->type == "FOR")
        {
            int headerIdx = cur;

            // ── sequential edge: header → first body node ─────────────
            // (already added by the linear-chain pass; nothing extra here)

            // ── recurse into body (handles nested loops too) ──────────
            buildEdges(s->children[3]->children, cfg, pos);

            // pos is now just past the last body node
            int afterLoopIdx = pos;
            int lastBodyIdx  = pos - 1;

            // ── back edge: last-body → header ─────────────────────────
            if (lastBodyIdx > headerIdx)
                cfg[lastBodyIdx].push_back(headerIdx);

            // ── exit edge: header → after-loop (condition false) ──────
            if (afterLoopIdx < n)
                cfg[headerIdx].push_back(afterLoopIdx);
        }
        // Non-FOR nodes: the linear chain from `buildCFGFromStmts` already
        // added the sequential edge; nothing else needed here.
    }
}

// ── Statement-aware CFG builder (handles FOR loops with back-edges) ──────────
vector<vector<int>> buildCFGFromStmts(const vector<ASTNode*>& stmts)
{
    int n = countNodes(stmts);
    if (n == 0) return {};

    // 1. Build a linear chain as the baseline
    vector<vector<int>> cfg(n);
    for (int i = 0; i < n - 1; i++)
        cfg[i].push_back(i + 1);

    // 2. Layer on back-edges and exit-edges for every FOR loop
    int pos = 0;
    buildEdges(stmts, cfg, pos);

    return cfg;
}

// ── DFS (unchanged) ──────────────────────────────────────────────────────────
void dfs(int node, vector<vector<int>>& cfg, vector<bool>& vis)
{
    vis[node] = true;
    for (auto x : cfg[node])
        if (!vis[x])
            dfs(x, cfg, vis);
}