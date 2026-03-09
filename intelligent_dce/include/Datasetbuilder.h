#ifndef DATASETBUILDER_H
#define DATASETBUILDER_H

// =======================================================================
// DatasetBuilder.h  —  Week 10: Dataset Preparation
// Creates a labeled dataset of code snippets for ML training.
// Each row = one variable in one snippet, labeled LIVE or DEAD
// using classical DCE (liveness analysis) as ground truth.
// =======================================================================

#include <vector>
#include <string>
#include "ASTNode.h"
#include "FeatureExtractor.h"

// -----------------------------------------------------------------------
// DatasetRow — one labeled sample (one variable in one snippet)
// -----------------------------------------------------------------------
struct DatasetRow
{
    // Identity
    int         snippetId;      // which code snippet this came from
    std::string variableName;   // the variable being labeled

    // Features (copied from CodeFeatures for this snippet)
    int   totalVariables;
    int   totalStatements;
    int   cfgDepth;
    int   cfgNodes;
    int   sideEffectCount;
    int   varDeclCount;         // how many times THIS variable is declared
    int   varUseCount;          // how many times THIS variable is used
    float usageRatio;           // snippet-level usage ratio

    // Ground-truth label (set by classical DCE / liveness analysis)
    std::string label;          // "LIVE" or "DEAD"
};

// -----------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------

// Build one snippet's rows using liveness as ground truth
std::vector<DatasetRow> buildSnippetRows(
    int                           snippetId,
    std::vector<ASTNode*>         stmts,
    std::vector<std::vector<int>> cfg
);

// Validate labels: re-run liveness and confirm every label is consistent
bool validateLabels(std::vector<DatasetRow>& rows,
                    std::vector<ASTNode*>    stmts);

// Print dataset rows to console
void printDataset(const std::vector<DatasetRow>& rows);

// Save full dataset to CSV
void saveDatasetCSV(const std::vector<DatasetRow>& rows,
                    const std::string&              filename,
                    bool                            writeHeader);

#endif