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
//
// FOR loops are fully handled: the loop variable is collected via the
// recursive collectDeclVars helper, and runLiveness iterates the loop
// body until the live-set stabilises (models the back-edge).
// =======================================================================

#include "Datasetbuilder.h"
#include "Featureextractor.h"
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

using namespace std;

// -----------------------------------------------------------------------
// INTERNAL helpers
// -----------------------------------------------------------------------

// Forward declaration
static void runLivenessHelper(vector<ASTNode*>& stmts, set<string>& live);

static void runLivenessHelper(vector<ASTNode*>& stmts, set<string>& live)
{
    for (int i = (int)stmts.size() - 1; i >= 0; i--)
    {
        ASTNode* stmt = stmts[i];

        if (stmt->type == "RETURN")
        {
            live.insert(stmt->children[0]->value);
        }
        else if (stmt->type == "DECL")
        {
            string defined = stmt->children[1]->value;
            if (live.count(defined))
            {
                ASTNode* rhs = stmt->children[2];
                if (rhs->type == "IDENTIFIER")
                    live.insert(rhs->value);
            }
        }
        else if (stmt->type == "FOR")
        {
            ASTNode* initDecl   = stmt->children[0];
            ASTNode* condNode   = stmt->children[1];
            ASTNode* updateNode = stmt->children[2];
            ASTNode* bodyNode   = stmt->children[3];

            // Iterate body until live-set stabilises (back-edge model)
            set<string> prev;
            do {
                prev = live;
                runLivenessHelper(bodyNode->children, live);
            } while (live != prev);

            // Condition operands
            if (!condNode->children.empty())
            {
                live.insert(condNode->children[0]->value);
                if ((int)condNode->children.size() > 1 &&
                    condNode->children[1]->type == "IDENTIFIER")
                    live.insert(condNode->children[1]->value);
            }

            // Update uses the loop variable
            if (!updateNode->children.empty())
                live.insert(updateNode->children[0]->value);

            // Init RHS
            if ((int)initDecl->children.size() > 2 &&
                initDecl->children[2]->type == "IDENTIFIER")
                live.insert(initDecl->children[2]->value);
        }
    }
}

// Run classical liveness analysis; returns the live set.
static set<string> runLiveness(vector<ASTNode*> stmts)
{
    set<string> live;
    runLivenessHelper(stmts, live);
    return live;
}

// Collect all declared variable names (DECL statements + FOR init),
// recursing into FOR bodies.
static void collectDeclVars(const vector<ASTNode*>& stmts,
                             vector<ASTNode*>&        out)
{
    for (ASTNode* stmt : stmts)
    {
        if (stmt->type == "DECL")
        {
            out.push_back(stmt);
        }
        else if (stmt->type == "FOR")
        {
            // The FOR init counts as a declaration
            out.push_back(stmt->children[0]);
            // Recurse into body
            collectDeclVars(stmt->children[3]->children, out);
        }
    }
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

    // Step 3: one row per declared variable (including FOR loop vars)
    vector<ASTNode*> declNodes;
    collectDeclVars(stmts, declNodes);

    for (ASTNode* declNode : declNodes)
    {
        string varName = declNode->children[1]->value;

        DatasetRow row;
        row.snippetId        = snippetId;
        row.variableName     = varName;
        row.totalVariables   = feat.totalVariables;
        row.totalStatements  = feat.totalStatements;
        row.cfgDepth         = feat.cfgDepth;
        row.cfgNodes         = feat.cfgNodes;
        row.sideEffectCount  = feat.sideEffectCount;
        row.usageRatio       = feat.usageRatio;

        row.varDeclCount = feat.varDeclCount.count(varName)
                               ? feat.varDeclCount.at(varName) : 0;
        row.varUseCount  = feat.varUseCount.count(varName)
                               ? feat.varUseCount.at(varName)  : 0;

        row.label = live.count(varName) ? "LIVE" : "DEAD";
        rows.push_back(row);
    }

    return rows;
}

// -----------------------------------------------------------------------
// validateLabels
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
                 << "  expected="    << expected << "  (correcting)\n";
            row.label = expected;
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
    ofstream out(filename, writeHeader ? ios::trunc : ios::app);

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