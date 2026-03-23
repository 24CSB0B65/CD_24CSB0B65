// =======================================================================
// FeatureExtractor.cpp  —  Week 9: Feature Engineering
//
// Three groups of features are extracted:
//   1. Static/structural  — statement counts, CFG shape
//   2. Variable usage     — declaration and use counts per variable
//   3. Side effects       — variables tagged by the Security module
//
// FOR loops are handled recursively: the init variable is counted as a
// declaration, the condition/update variables are counted as uses, and
// every statement inside the body is processed exactly as if it were at
// the top level.
// =======================================================================

#include "FeatureExtractor.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

// ── Helper: CFG depth via DP on a DAG ───────────────────────────────────────
static int computeCFGDepth(const vector<vector<int>>& cfg)
{
    int n = (int)cfg.size();
    if (n == 0) return 0;

    vector<int> dist(n, 0);
    for (int node = 0; node < n; node++)
        for (int next : cfg[node])
            if (next > node && dist[next] < dist[node] + 1)   // skip back-edges
                dist[next] = dist[node] + 1;

    int maxDepth = 0;
    for (int d : dist)
        if (d > maxDepth) maxDepth = d;

    return maxDepth;
}

// ── Recursive helper: walk a statement list and fill CodeFeatures ────────────
static void walkStmts(const vector<ASTNode*>& stmts, CodeFeatures& f)
{
    for (ASTNode* stmt : stmts)
    {
        f.totalStatements++;

        if (stmt->type == "DECL")
        {
            string definedVar = stmt->children[1]->value;
            f.varDeclCount[definedVar]++;
            f.totalVariables++;

            // RHS use
            ASTNode* rhs = stmt->children[2];
            if (rhs->type == "IDENTIFIER")
                f.varUseCount[rhs->value]++;

            // Security side-effect tag
            if (stmt->securityTag == "SIDE_EFFECT")
                f.sideEffectCount++;
        }
        else if (stmt->type == "RETURN")
        {
            f.returnCount++;
            f.varUseCount[stmt->children[0]->value]++;
        }
        else if (stmt->type == "FOR")
        {
            // ── Init: int <var> = <val> ─────────────────────────────
            ASTNode* initDecl = stmt->children[0];
            if ((int)initDecl->children.size() > 1)
            {
                string loopVar = initDecl->children[1]->value;
                f.varDeclCount[loopVar]++;
                f.totalVariables++;

                if ((int)initDecl->children.size() > 2 &&
                    initDecl->children[2]->type == "IDENTIFIER")
                    f.varUseCount[initDecl->children[2]->value]++;
            }

            // ── Condition: both operands are uses ───────────────────
            ASTNode* condNode = stmt->children[1];
            if (!condNode->children.empty())
            {
                f.varUseCount[condNode->children[0]->value]++;
                if ((int)condNode->children.size() > 1 &&
                    condNode->children[1]->type == "IDENTIFIER")
                    f.varUseCount[condNode->children[1]->value]++;
            }

            // ── Update: loop variable is used ───────────────────────
            ASTNode* updateNode = stmt->children[2];
            if (!updateNode->children.empty())
                f.varUseCount[updateNode->children[0]->value]++;

            // ── Body: recurse ────────────────────────────────────────
            walkStmts(stmt->children[3]->children, f);
        }
    }
}

// ── Public: extractFeatures ──────────────────────────────────────────────────
CodeFeatures extractFeatures(vector<ASTNode*>      stmts,
                              vector<vector<int>>   cfg)
{
    CodeFeatures f;
    f.totalVariables   = 0;
    f.totalStatements  = 0;
    f.returnCount      = 0;
    f.sideEffectCount  = 0;
    f.unusedVarCount   = 0;
    f.usedVarCount     = 0;
    f.usageRatio       = 0.0f;
    f.cfgNodes         = (int)cfg.size();
    f.cfgDepth         = computeCFGDepth(cfg);

    walkStmts(stmts, f);

    // ── Derived: unused vs used variables ────────────────────────────
    for (auto& kv : f.varDeclCount)
    {
        const string& var = kv.first;
        bool used = f.varUseCount.find(var) != f.varUseCount.end()
                    && f.varUseCount[var] > 0;
        if (used) f.usedVarCount++;
        else      f.unusedVarCount++;
    }

    if (f.totalVariables > 0)
        f.usageRatio = (float)f.usedVarCount / (float)f.totalVariables;

    return f;
}

// ── Public: printFeatures ────────────────────────────────────────────────────
void printFeatures(const CodeFeatures& f)
{
    cout << "\n========================================\n";
    cout << "       FEATURE EXTRACTION REPORT\n";
    cout << "========================================\n";

    cout << "\n--- Static / Structural Features ---\n";
    cout << "  Total Statements  : " << f.totalStatements  << "\n";
    cout << "  Total Variables   : " << f.totalVariables   << "\n";
    cout << "  Return Statements : " << f.returnCount      << "\n";
    cout << "  CFG Nodes         : " << f.cfgNodes         << "\n";
    cout << "  CFG Depth         : " << f.cfgDepth         << "\n";
    cout << "  Side Effects      : " << f.sideEffectCount  << "\n";

    cout << "\n--- Variable Declaration Counts ---\n";
    for (auto& kv : f.varDeclCount)
        cout << "  " << kv.first << "  declared " << kv.second << " time(s)\n";

    cout << "\n--- Variable Use Counts (RHS + condition + return) ---\n";
    for (auto& kv : f.varUseCount)
        cout << "  " << kv.first << "  used " << kv.second << " time(s)\n";

    cout << "\n--- Derived Features ---\n";
    cout << "  Used Variables    : " << f.usedVarCount   << "\n";
    cout << "  Unused Variables  : " << f.unusedVarCount << "\n";
    cout << fixed << setprecision(2);
    cout << "  Usage Ratio       : " << f.usageRatio     << "\n";
    cout << "========================================\n";
}

// ── Public: saveFeaturesCSV ──────────────────────────────────────────────────
void saveFeaturesCSV(const CodeFeatures& f,
                     const string&       filename,
                     bool                writeHeader)
{
    ofstream out(filename, ios::app);

    if (writeHeader)
    {
        out << "total_statements,total_variables,return_count,"
            << "cfg_nodes,cfg_depth,side_effect_count,"
            << "used_var_count,unused_var_count,usage_ratio\n";
    }

    out << f.totalStatements  << ","
        << f.totalVariables   << ","
        << f.returnCount      << ","
        << f.cfgNodes         << ","
        << f.cfgDepth         << ","
        << f.sideEffectCount  << ","
        << f.usedVarCount     << ","
        << f.unusedVarCount   << ","
        << fixed << setprecision(4) << f.usageRatio << "\n";

    out.close();
    cout << "\n[FeatureExtractor] Features saved to '" << filename << "'\n";
}