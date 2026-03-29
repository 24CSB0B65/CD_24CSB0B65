#include "Analysis.h"
#include <iostream>
#include <fstream>
#include <set>

using namespace std;

set<string> live;

// Single output file stream — truncated on each run so previous output is cleared
static ofstream outFile("liveness_output.txt", ios::trunc);

// ── Forward declaration ──────────────────────────────────────────────────────
static void processLivenessBackward(vector<ASTNode*>& stmts,
                                     set<string>&       liveSet,
                                     bool               verbose,
                                     int                stepOffset);

// ── Core backward liveness pass ─────────────────────────────────────────────
// Walks `stmts` in reverse.  Handles DECL, RETURN and FOR nodes.
// For FOR loops it iterates the body until the live-set stabilises
// (models the loop back-edge).
static void processLivenessBackward(vector<ASTNode*>& stmts,
                                     set<string>&       liveSet,
                                     bool               verbose,
                                     int                stepOffset)
{
    auto print = [&](const string& msg)
    {
        if (!verbose) return;
        cout    << msg;
        outFile << msg;
    };

    for (int i = (int)stmts.size() - 1; i >= 0; i--)
    {
        ASTNode* stmt = stmts[i];

        // ── RETURN ────────────────────────────────────────────────────
        if (stmt->type == "RETURN")
        {
            string var = stmt->children[0]->value;
            liveSet.insert(var);
            print("Step " + to_string(stepOffset + i) +
                  " | RETURN " + var +
                  "  =>  mark '" + var + "' as LIVE\n");
        }

        // ── DECL ──────────────────────────────────────────────────────
        else if (stmt->type == "DECL")
        {
            string    defined = stmt->children[1]->value;
            ASTNode*  rhs     = stmt->children[2];

            if (liveSet.count(defined))
            {
                string msg = "Step " + to_string(stepOffset + i) +
                             " | DECL  " + defined +
                             " = "  + rhs->value +
                             "  =>  '" + defined + "' is LIVE";

                if (rhs->type == "IDENTIFIER")
                {
                    liveSet.insert(rhs->value);
                    msg += ",  propagate: mark '" + rhs->value + "' as LIVE";
                }
                print(msg + "\n");
            }
            else
            {
                print("Step " + to_string(stepOffset + i) +
                      " | DECL  " + defined + " = " + rhs->value +
                      "  =>  '" + defined + "' is DEAD (not used downstream)\n");
            }
        }

        // ── FOR ───────────────────────────────────────────────────────
        else if (stmt->type == "FOR")
        {
            ASTNode* initDecl   = stmt->children[0]; // DECL  (int i = 0)
            ASTNode* condNode   = stmt->children[1]; // CONDITION
            ASTNode* updateNode = stmt->children[2]; // UPDATE
            ASTNode* bodyNode   = stmt->children[3]; // BODY

            // 1. Iterate body backward until the live-set stabilises.
            //    This models the loop back-edge: a variable used in a
            //    later iteration may force liveness in an earlier one.
            set<string> prevLive;
            do {
                prevLive = liveSet;
                int dummy = 0;
                processLivenessBackward(bodyNode->children, liveSet, false, dummy);
            } while (liveSet != prevLive);

            // 2. Condition operands are uses.
            if (!condNode->children.empty())
            {
                liveSet.insert(condNode->children[0]->value);  // loop variable
                if ((int)condNode->children.size() > 1 &&
                    condNode->children[1]->type == "IDENTIFIER")
                    liveSet.insert(condNode->children[1]->value); // limit var
            }

            // 3. Update: the loop variable is both used and redefined.
            if (!updateNode->children.empty())
                liveSet.insert(updateNode->children[0]->value);

            // 4. Init: loop variable is live (used by condition/update).
            //    If the init RHS is itself a variable, mark it live too.
            if ((int)initDecl->children.size() > 2 &&
                initDecl->children[2]->type == "IDENTIFIER")
                liveSet.insert(initDecl->children[2]->value);

            string loopVar = ((int)initDecl->children.size() > 1)
                                 ? initDecl->children[1]->value : "?";
            print("Step " + to_string(stepOffset + i) +
                  " | FOR   " + loopVar +
                  "  =>  loop variable and body liveness processed\n");
        }

        // ── Print live set after each step (verbose mode) ─────────────
        if (verbose)
        {
            string ls = "         live set = { ";
            for (const string& v : liveSet) ls += v + " ";
            ls += "}\n";
            print(ls);
        }
    }
}

// ── Public: livenessAnalysis ─────────────────────────────────────────────────
void livenessAnalysis(vector<ASTNode*> stmts)
{
    auto print = [&](const string& msg)
    {
        cout    << msg;
        outFile << msg;
    };

    print("\n--- LIVENESS ANALYSIS (backward traversal) ---\n");

    int step = 0;
    processLivenessBackward(stmts, live, /*verbose=*/true, step);

    // Final live set
    string finalStr = "\nFinal live set = { ";
    for (const string& v : live) finalStr += v + " ";
    finalStr += "}\n";
    print(finalStr);

    outFile.flush();
}

// ── Helper: collect all DECL and FOR-init variables from a stmt list ─────────
static void collectDeclVars(vector<ASTNode*>& stmts,
                             vector<ASTNode*>& out)
{
    for (ASTNode* stmt : stmts)
    {
        if (stmt->type == "DECL")
            out.push_back(stmt);
        else if (stmt->type == "FOR")
        {
            // FOR init counts as a declaration
            out.push_back(stmt->children[0]);
            // Recurse into body
            collectDeclVars(stmt->children[3]->children, out);
        }
    }
}

// ── Public: deadCodeElimination ──────────────────────────────────────────────
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
        else if (stmt->type == "FOR")
        {
            // For-loop bodies may contain live uses; keep conservatively.
            // A more precise analysis would inspect the body, but basic DCE
            // retains any loop whose body or iteration variable is live.
            string loopVar = stmt->children[0]->children[1]->value;
            print("KEEP    =>  for (" + loopVar + " ...)\n");
        }

        optimized.push_back(stmt);
    }

    stmts = optimized;

    // ── Print optimised program ───────────────────────────────────────
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
        else if (stmt->type == "FOR")
        {
            string loopVar  = stmt->children[0]->children[1]->value;
            string initVal  = stmt->children[0]->children[2]->value;
            string condOp   = stmt->children[1]->value;
            string condLim  = stmt->children[1]->children.size() > 1
                                  ? stmt->children[1]->children[1]->value : "?";
            string updateOp = stmt->children[2]->value;
            print("for (int " + loopVar + " = " + initVal + "; " +
                  loopVar + " " + condOp + " " + condLim + "; " +
                  updateOp + ") { ... }\n");
        }
    }

    outFile.flush();
}