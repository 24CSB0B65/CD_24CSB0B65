#include "Analysis.h"
#include <iostream>
#include <fstream>
#include <set>

using namespace std;

set<string> live;

// Single output file stream — opened once, shared across both functions
static ofstream outFile("liveness_output.txt");

void livenessAnalysis(vector<ASTNode*> stmts)
{
    // Print to both console AND file using a lambda
    auto print = [&](const string& msg)
    {
        cout    << msg;
        outFile << msg;
    };

    print("\n--- LIVENESS ANALYSIS (backward traversal) ---\n");

    for (int i = (int)stmts.size() - 1; i >= 0; i--)
    {
        if (stmts[i]->type == "RETURN")
        {
            string var = stmts[i]->children[0]->value;
            live.insert(var);
            print("Step " + to_string(i) + " | RETURN " + var +
                  "  =>  mark '" + var + "' as LIVE\n");
        }
        else if (stmts[i]->type == "DECL")
        {
            string defined = stmts[i]->children[1]->value;
            ASTNode* rhs   = stmts[i]->children[2];

            if (live.count(defined))
            {
                string msg = "Step " + to_string(i) + " | DECL  " + defined +
                             " = " + rhs->value +
                             "  =>  '" + defined + "' is LIVE";

                if (rhs->type == "IDENTIFIER")
                {
                    live.insert(rhs->value);
                    msg += ",  propagate: mark '" + rhs->value + "' as LIVE";
                }
                print(msg + "\n");
            }
            else
            {
                print("Step " + to_string(i) + " | DECL  " + defined +
                      " = " + rhs->value +
                      "  =>  '" + defined + "' is DEAD (not used downstream)\n");
            }
        }

        // Print live set after each step
        string liveStr = "         live set = { ";
        for (const string& v : live) liveStr += v + " ";
        liveStr += "}\n";
        print(liveStr);
    }

    // Print final live set
    string finalStr = "\nFinal live set = { ";
    for (const string& v : live) finalStr += v + " ";
    finalStr += "}\n";
    print(finalStr);

    outFile.flush();  // ensure everything is written to file
}

void deadCodeElimination(vector<ASTNode*>& stmts)
{
    auto print = [&](const string& msg)
    {
        cout    << msg;
        outFile << msg;
    };

    print("\n--- DEAD CODE ELIMINATION ---\n");

    vector<ASTNode*> optimized;

    for (auto stmt : stmts)
    {
        if (stmt->type == "DECL")
        {
            string var = stmt->children[1]->value;
            string rhs = stmt->children[2]->value;

            if (live.find(var) == live.end())
            {
                print("REMOVE  =>  int " + var + " = " + rhs +
                      ";  (dead: never used)\n");
                continue;
            }
            else
            {
                print("KEEP    =>  int " + var + " = " + rhs + ";\n");
            }
        }
        else if (stmt->type == "RETURN")
        {
            string var = stmt->children[0]->value;
            print("KEEP    =>  return " + var + ";\n");
        }

        optimized.push_back(stmt);
    }

    stmts = optimized;

    print("\n--- OPTIMIZED PROGRAM ---\n");
    for (auto stmt : stmts)
    {
        if (stmt->type == "DECL")
        {
            print("int " + stmt->children[1]->value +
                  " = "  + stmt->children[2]->value + ";\n");
        }
        else if (stmt->type == "RETURN")
        {
            print("return " + stmt->children[0]->value + ";\n");
        }
    }

    outFile.flush();
}