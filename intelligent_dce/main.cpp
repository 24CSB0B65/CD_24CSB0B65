#include <iostream>
#include <vector>
#include <string>

#include "Lexer.h"
#include "Parser.h"
#include "ASTNode.h"
#include "Analysis.h"

using namespace std;

int main()
{
    string code =
        "int a = 10; "
        "int b = 5; "
        "int c = 8; "
        "return a;";

    cout << "SOURCE CODE\n";
    cout << code << endl;

    vector<string> tokens = tokenize(code);

    cout << "\nTOKENS\n";
    for (auto t : tokens)
        cout << t << endl;

    ASTNode* root = buildAST(tokens);

    cout << "\nAST\n";
    printAST(root);

    vector<ASTNode*> stmts = root->children;

    cout << "\nLIVENESS ANALYSIS\n";
    livenessAnalysis(stmts);

    cout << "\nDEAD CODE ELIMINATION\n";
    deadCodeElimination(stmts);

    cout << "\nOPTIMIZED PROGRAM\n";

    for (auto stmt : stmts)
    {
        if (stmt->type == "DECL")
        {
            cout << stmt->children[0]->value << " "
                 << stmt->children[1]->value
                 << " = "
                 << stmt->children[2]->value
                 << ";" << endl;
        }
        else if (stmt->type == "RETURN")
        {
            cout << "return "
                 << stmt->children[0]->value
                 << ";" << endl;
        }
    }

    return 0;
}