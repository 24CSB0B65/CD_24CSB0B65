#include "Analysis.h"
#include <iostream>
#include <set>

using namespace std;

set<string> live;

void livenessAnalysis(vector<ASTNode*> stmts)
{
    for (int i = stmts.size() - 1; i >= 0; i--)
    {
        if (stmts[i]->type == "RETURN")
        {
            string var = stmts[i]->children[0]->value;
            live.insert(var);
        }
    }
}

void deadCodeElimination(vector<ASTNode*>& stmts)
{
    vector<ASTNode*> optimized;

    for (auto stmt : stmts)
    {
        if (stmt->type == "DECL")
        {
            string var = stmt->children[1]->value;

            if (live.find(var) == live.end())
            {
                cout << "Dead variable: " << var << endl;
                continue;
            }
        }

        optimized.push_back(stmt);
    }

    stmts = optimized;
}