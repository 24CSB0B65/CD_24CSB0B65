// =======================================================================
// FeatureExtractor.cpp  —  Week 9: Feature Engineering
//
// Three groups of features are extracted:
//   1. Static/structural  — statement counts, CFG shape
//   2. Variable usage     — declaration and use counts per variable
//   3. Side effects       — variables tagged by the Security module
// =======================================================================

#include "FeatureExtractor.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <queue>

using namespace std;

// -----------------------------------------------------------------------
// HELPER: compute CFG depth (longest path from node 0) via BFS/DP
// -----------------------------------------------------------------------
static int computeCFGDepth(const vector<vector<int>>& cfg)
{
    int n = (int)cfg.size();
    if (n == 0) return 0;

    // dist[i] = longest path reaching node i
    vector<int> dist(n, 0);

    for (int node = 0; node < n; node++)
        for (int next : cfg[node])
            if (dist[next] < dist[node] + 1)
                dist[next] = dist[node] + 1;

    int maxDepth = 0;
    for (int d : dist)
        if (d > maxDepth) maxDepth = d;

    return maxDepth;
}

// -----------------------------------------------------------------------
// extractFeatures — main function
// -----------------------------------------------------------------------
CodeFeatures extractFeatures(vector<ASTNode*>         stmts,
                              vector<vector<int>>      cfg)
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

    // --- Walk every statement ---
    for (ASTNode* stmt : stmts)
    {
        f.totalStatements++;

        if (stmt->type == "DECL")
        {
            // children[1] = IDENTIFIER (variable being defined)
            // children[2] = VALUE or IDENTIFIER (RHS)
            string definedVar = stmt->children[1]->value;
            f.varDeclCount[definedVar]++;
            f.totalVariables++;

            // Count RHS use: if RHS is an IDENTIFIER, that variable is used
            ASTNode* rhs = stmt->children[2];
            if (rhs->type == "IDENTIFIER")
                f.varUseCount[rhs->value]++;

            // Side effect check (from Security module tag)
            if (stmt->securityTag == "SIDE_EFFECT")
                f.sideEffectCount++;
        }
        else if (stmt->type == "RETURN")
        {
            f.returnCount++;
            // The returned variable counts as a use
            string retVar = stmt->children[0]->value;
            f.varUseCount[retVar]++;
        }
    }

    // --- Derived: unused vs used variables ---
    for (auto& kv : f.varDeclCount)
    {
        const string& var = kv.first;
        if (f.varUseCount.find(var) == f.varUseCount.end()
            || f.varUseCount[var] == 0)
            f.unusedVarCount++;
        else
            f.usedVarCount++;
    }

    if (f.totalVariables > 0)
        f.usageRatio = (float)f.usedVarCount / (float)f.totalVariables;

    return f;
}

// -----------------------------------------------------------------------
// printFeatures — console output
// -----------------------------------------------------------------------
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

    cout << "\n--- Variable Use Counts (RHS + return) ---\n";
    for (auto& kv : f.varUseCount)
        cout << "  " << kv.first << "  used " << kv.second << " time(s)\n";

    cout << "\n--- Derived Features ---\n";
    cout << "  Used Variables    : " << f.usedVarCount   << "\n";
    cout << "  Unused Variables  : " << f.unusedVarCount << "\n";
    cout << fixed << setprecision(2);
    cout << "  Usage Ratio       : " << f.usageRatio     << "\n";
    cout << "========================================\n";
}

// -----------------------------------------------------------------------
// saveFeaturesCSV — write one row per snippet into a CSV file
// -----------------------------------------------------------------------
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