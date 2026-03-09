// =======================================================================
// DatasetBuilder.cpp  —  Week 10: Dataset Preparation
//
// Steps performed:
//   1. Run classical liveness analysis  →  live set (ground truth)
//   2. Extract CodeFeatures via FeatureExtractor
//   3. For every declared variable, create one DatasetRow and assign
//      label "LIVE" or "DEAD" based on the live set
//   4. Validate: re-run liveness and confirm labels match
//   5. Save to CSV for ML training
// =======================================================================

#include "DatasetBuilder.h"
#include "FeatureExtractor.h"
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

using namespace std;

// -----------------------------------------------------------------------
// INTERNAL: run classical liveness analysis, return live set
// (mirrors Analysis.cpp logic — self-contained so no global state used)
// -----------------------------------------------------------------------
static set<string> runLiveness(vector<ASTNode*> stmts)
{
    set<string> live;

    for (int i = (int)stmts.size() - 1; i >= 0; i--)
    {
        if (stmts[i]->type == "RETURN")
        {
            live.insert(stmts[i]->children[0]->value);
        }
        else if (stmts[i]->type == "DECL")
        {
            string defined = stmts[i]->children[1]->value;
            if (live.count(defined))
            {
                ASTNode* rhs = stmts[i]->children[2];
                if (rhs->type == "IDENTIFIER")
                    live.insert(rhs->value);
                // defined stays in live (Week 9 bug fix)
            }
        }
    }
    return live;
}

// -----------------------------------------------------------------------
// buildSnippetRows
// -----------------------------------------------------------------------
vector<DatasetRow> buildSnippetRows(int                    snippetId,
                                     vector<ASTNode*>       stmts,
                                     vector<vector<int>>    cfg)
{
    vector<DatasetRow> rows;

    // Step 1: ground-truth live set
    set<string> live = runLiveness(stmts);

    // Step 2: extract snippet-level features
    CodeFeatures feat = extractFeatures(stmts, cfg);

    // Step 3: one row per declared variable
    for (ASTNode* stmt : stmts)
    {
        if (stmt->type != "DECL") continue;

        string varName = stmt->children[1]->value;

        DatasetRow row;
        row.snippetId        = snippetId;
        row.variableName     = varName;
        row.totalVariables   = feat.totalVariables;
        row.totalStatements  = feat.totalStatements;
        row.cfgDepth         = feat.cfgDepth;
        row.cfgNodes         = feat.cfgNodes;
        row.sideEffectCount  = feat.sideEffectCount;
        row.usageRatio       = feat.usageRatio;

        // per-variable counts from feature maps
        row.varDeclCount = feat.varDeclCount.count(varName)
                               ? feat.varDeclCount.at(varName) : 0;
        row.varUseCount  = feat.varUseCount.count(varName)
                               ? feat.varUseCount.at(varName)  : 0;

        // label via ground truth
        row.label = live.count(varName) ? "LIVE" : "DEAD";

        rows.push_back(row);
    }

    return rows;
}

// -----------------------------------------------------------------------
// validateLabels — re-run liveness and confirm every label is consistent
// -----------------------------------------------------------------------
bool validateLabels(vector<DatasetRow>& rows, vector<ASTNode*> stmts)
{
    set<string> live = runLiveness(stmts);
    bool allValid = true;

    cout << "\n--- LABEL VALIDATION ---\n";

    for (DatasetRow& row : rows)
    {
        string expected = live.count(row.variableName) ? "LIVE" : "DEAD";

        if (row.label == expected)
        {
            cout << "  [OK]   " << row.variableName
                 << "  =>  " << row.label << "\n";
        }
        else
        {
            cout << "  [MISMATCH]  " << row.variableName
                 << "  =>  labeled=" << row.label
                 << "  expected=" << expected << "  (correcting)\n";
            row.label = expected;   // auto-correct
            allValid  = false;
        }
    }

    cout << (allValid ? "\nAll labels validated successfully.\n"
                      : "\nSome labels were corrected.\n");
    return allValid;
}

// -----------------------------------------------------------------------
// printDataset
// -----------------------------------------------------------------------
void printDataset(const vector<DatasetRow>& rows)
{
    cout << "\n========================================\n";
    cout << "           LABELED DATASET\n";
    cout << "========================================\n";
    cout << left
         << setw(6)  << "ID"
         << setw(8)  << "VAR"
         << setw(8)  << "DECLS"
         << setw(8)  << "USES"
         << setw(8)  << "STMTS"
         << setw(8)  << "VARS"
         << setw(8)  << "CFG_D"
         << setw(8)  << "CFG_N"
         << setw(8)  << "SIDE"
         << setw(8)  << "RATIO"
         << setw(8)  << "LABEL"
         << "\n";
    cout << string(86, '-') << "\n";

    for (const DatasetRow& r : rows)
    {
        cout << left
             << setw(6)  << r.snippetId
             << setw(8)  << r.variableName
             << setw(8)  << r.varDeclCount
             << setw(8)  << r.varUseCount
             << setw(8)  << r.totalStatements
             << setw(8)  << r.totalVariables
             << setw(8)  << r.cfgDepth
             << setw(8)  << r.cfgNodes
             << setw(8)  << r.sideEffectCount
             << setw(8)  << fixed << setprecision(2) << r.usageRatio
             << setw(8)  << r.label
             << "\n";
    }
    cout << "========================================\n";
    cout << "Total rows: " << rows.size() << "\n";
}

// -----------------------------------------------------------------------
// saveDatasetCSV
// -----------------------------------------------------------------------
void saveDatasetCSV(const vector<DatasetRow>& rows,
                    const string&             filename,
                    bool                      writeHeader)
{
    ofstream out(filename, ios::app);

    if (writeHeader)
    {
        out << "snippet_id,variable_name,var_decl_count,var_use_count,"
            << "total_statements,total_variables,cfg_depth,cfg_nodes,"
            << "side_effect_count,usage_ratio,label\n";
    }

    for (const DatasetRow& r : rows)
    {
        out << r.snippetId        << ","
            << r.variableName     << ","
            << r.varDeclCount     << ","
            << r.varUseCount      << ","
            << r.totalStatements  << ","
            << r.totalVariables   << ","
            << r.cfgDepth         << ","
            << r.cfgNodes         << ","
            << r.sideEffectCount  << ","
            << fixed << setprecision(4) << r.usageRatio << ","
            << r.label            << "\n";
    }

    out.close();
    cout << "\n[DatasetBuilder] Dataset saved to '" << filename << "'\n";
    cout << "[DatasetBuilder] Total rows written: " << rows.size() << "\n";
}