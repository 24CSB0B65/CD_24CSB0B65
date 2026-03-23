// =======================================================================
// main.cpp  —  Full pipeline: Weeks 1–10
//
// Flow:
//   1.  Read source file
//   2.  Lexer     → tokens
//   3.  Parser    → AST
//   4.  CFG       → control flow graph
//   5.  Security  → side-effect tagging
//   6.  Liveness  → live set
//   7.  DCE       → optimized program            (Analysis.cpp)
//   8.  Features  → CodeFeatures struct          (Week 9)
//   9.  Dataset   → labeled CSV rows             (Week 10)
// =======================================================================
/*int main() {
    int a = 10;
    int b = 20;
    int c = a ;  

    int d = 50;      
    int e = c ;   

    int f = 100;     

    return e;
}*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "Lexer.h"
#include "Parser.h"
#include "CFG.h"
#include "Security.h"
#include "Analysis.h"
#include "FeatureExtractor.h"
#include "DatasetBuilder.h"

using namespace std;

int main()
{
    // ----------------------------------------------------------------
    // STEP 1 — Read source file
    // ----------------------------------------------------------------
    ifstream srcFile("examples/sample_program.txt");
    if (!srcFile.is_open())
    {
        cerr << "Error: cannot open sample_program.txt\n";
        return 1;
    }
    stringstream buf;
    buf << srcFile.rdbuf();
    string sourceCode = buf.str();
    srcFile.close();

    cout << "========================================\n";
    cout << "SOURCE CODE\n";
    cout << "========================================\n";
    cout << sourceCode << "\n";

    // ----------------------------------------------------------------
    // STEP 2 — Lexer
    // ----------------------------------------------------------------
    vector<string> tokens = tokenize(sourceCode);

    cout << "\n========================================\n";
    cout << "TOKENS\n";
    cout << "========================================\n";
    for (const string& t : tokens)
        cout << t << "\n";

    // ----------------------------------------------------------------
    // STEP 3 — Parser → AST
    // ----------------------------------------------------------------
    ASTNode* root = buildAST(tokens);

    cout << "\n========================================\n";
    cout << "AST\n";
    cout << "========================================\n";
    printAST(root);

    // Collect statement list from root children
    vector<ASTNode*> stmts = root->children;

    // ----------------------------------------------------------------
    // STEP 4 — Build CFG  (one node per statement)
    // ----------------------------------------------------------------
    vector<vector<int>> cfg = buildCFG((int)stmts.size());

    // ----------------------------------------------------------------
    // STEP 5 — Security analysis
    // ----------------------------------------------------------------
    securityAnalysis(stmts);

    // ----------------------------------------------------------------
    // STEP 6 & 7 — Liveness + Dead Code Elimination
    // ----------------------------------------------------------------
    livenessAnalysis(stmts);
    deadCodeElimination(stmts);

    // Sync back to root so printAST reflects the optimized tree
    root->children = stmts;

    // ----------------------------------------------------------------
    // STEP 8 — Feature Extraction  (Week 9)
    // ----------------------------------------------------------------
    cout << "\n========================================\n";
    cout << "WEEK 9: FEATURE EXTRACTION\n";
    cout << "========================================\n";

    // Rebuild CFG for the optimized statement list
    vector<vector<int>> optCFG = buildCFG((int)stmts.size());

    CodeFeatures features = extractFeatures(stmts, optCFG);
    printFeatures(features);
    saveFeaturesCSV(features, "features.csv", true);

    // ----------------------------------------------------------------
    // STEP 9 — Dataset Building + Labeling  (Week 10)
    // ----------------------------------------------------------------
    cout << "\n========================================\n";
    cout << "WEEK 10: DATASET PREPARATION\n";
    cout << "========================================\n";

    
    vector<ASTNode*> allStmts = root->children;   // already optimized here;
    // reload original AST for full dataset
    ASTNode* origRoot = buildAST(tokens);
    vector<ASTNode*> origStmts = origRoot->children;
    vector<vector<int>> origCFG = buildCFG((int)origStmts.size());

    vector<DatasetRow> dataset = buildSnippetRows(1, origStmts, origCFG);
    validateLabels(dataset, origStmts);
    printDataset(dataset);
    saveDatasetCSV(dataset, "dataset.csv", true);

    cout << "\n========================================\n";
    cout << "Pipeline complete.\n";
    cout << "Output files: liveness_output.txt, features.csv, dataset.csv\n";
    cout << "========================================\n";

    return 0;
}