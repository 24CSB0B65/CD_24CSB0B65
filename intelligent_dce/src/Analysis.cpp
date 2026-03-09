#include "Analysis.h"
#include <iostream>
#include <set>

using namespace std;

set<string> live;

void livenessAnalysis(vector<ASTNode*> stmts)
{
    // Traverse backwards (standard liveness analysis direction).
    for (int i = (int)stmts.size() - 1; i >= 0; i--)
    {
        if (stmts[i]->type == "RETURN")
        {
            // The variable being returned is live.
            string var = stmts[i]->children[0]->value;
            live.insert(var);
        }
        else if (stmts[i]->type == "DECL")
        {
            // Rule: if the defined variable is live (needed downstream),
            // keep it in the live set (so DCE will preserve it) and also
            // propagate liveness to any IDENTIFIER used on the RHS.
            //
            // BUG FIX: The previous code called live.erase(defined) here.
            // That removed the variable from live before deadCodeElimination
            // could check it, causing every DECL to be wrongly eliminated.
            //
            // Trace of the old (broken) behaviour:
            //   'return d'   -> live = {d}
            //   'int d = a'  -> erase d, insert a  -> live = {a}
            //   'int c = 8'  -> c not in live       -> live = {a}
            //   'int b = 5'  -> b not in live       -> live = {a}
            //   'int a = 10' -> erase a, rhs=VALUE  -> live = {}
            //   DCE: live is empty -> ALL variables reported dead. WRONG.
            //
            // Fix: do NOT erase 'defined'. It must remain in live so that
            // deadCodeElimination keeps the declaration.
            string defined = stmts[i]->children[1]->value;

            if (live.count(defined))
            {
                // Propagate upward: if RHS is a variable reference, it is
                // also needed and must be kept.
                ASTNode* rhs = stmts[i]->children[2];
                if (rhs->type == "IDENTIFIER")
                {
                    live.insert(rhs->value);
                }
                // 'defined' stays in live -- DCE will see it and keep the DECL.
            }
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
                continue;   // drop this statement
            }
        }

        optimized.push_back(stmt);
    }

    stmts = optimized;
}