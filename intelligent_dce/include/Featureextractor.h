#ifndef FEATUREEXTRACTOR_H
#define FEATUREEXTRACTOR_H

// =======================================================================
// FeatureExtractor.h  —  Week 9: Feature Engineering
// Extracts static, structural, and variable-usage features from the AST
// and CFG for use by the ML dataset builder (Week 10).
// =======================================================================

#include <vector>
#include <string>
#include <map>
#include "ASTNode.h"

// -----------------------------------------------------------------------
// CodeFeatures — holds every extracted feature for one code snippet
// -----------------------------------------------------------------------
struct CodeFeatures
{
    // --- Static / Structural ---
    int totalVariables;      // number of declared variables
    int totalStatements;     // total DECL + RETURN count
    int returnCount;         // number of return statements
    int cfgDepth;            // longest path through the CFG
    int cfgNodes;            // total nodes in the CFG
    int sideEffectCount;     // nodes tagged SIDE_EFFECT by Security module

    // --- Variable usage ---
    std::map<std::string, int> varDeclCount;  // declarations per variable
    std::map<std::string, int> varUseCount;   // RHS uses per variable

    // --- Derived ---
    int   unusedVarCount;   // declared but never used on any RHS
    int   usedVarCount;     // declared and used at least once
    float usageRatio;       // usedVarCount / totalVariables
};

// -----------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------
CodeFeatures extractFeatures(std::vector<ASTNode*>         stmts,
                              std::vector<std::vector<int>> cfg);

void printFeatures(const CodeFeatures& f);

void saveFeaturesCSV(const CodeFeatures&  f,
                     const std::string&   filename,
                     bool                 writeHeader);

#endif