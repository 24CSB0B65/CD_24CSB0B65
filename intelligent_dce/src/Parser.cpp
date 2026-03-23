#include "Parser.h"
#include <iostream>
#include <cctype>

using namespace std;

// ── Helpers ─────────────────────────────────────────────────────────────────

static bool isNumericLiteral(const string& s)
{
    if (s.empty()) return false;
    int start = (s[0] == '-') ? 1 : 0;
    if (start == 1 && (int)s.size() == 1) return false;
    for (int k = start; k < (int)s.size(); k++)
        if (!isdigit(s[k])) return false;
    return true;
}

// Build a VALUE node for literals, IDENTIFIER node for variables
static ASTNode* makeValNode(const string& tok)
{
    return isNumericLiteral(tok)
               ? new ASTNode("VALUE",      tok)
               : new ASTNode("IDENTIFIER", tok);
}

// ── Recursive statement parser ───────────────────────────────────────────────
// Parses tokens[i..] into children of `parent` until EOF or a closing '}'.
static void parseStatements(const vector<string>& tokens, int& i, ASTNode* parent)
{
    int sz = (int)tokens.size();

    while (i < sz)
    {
        // Stop at closing brace so the caller can consume it
        if (tokens[i] == "}") break;

        // ── int <var> = <val> ; ──────────────────────────────────────
        if (tokens[i] == "int")
        {
            // Need at least: int <id> = <val> ;  (5 tokens from i)
            if (i + 4 >= sz)
            {
                cerr << "Error: incomplete declaration at token index " << i << "\n";
                i++;
                continue;
            }

            ASTNode* decl = new ASTNode("DECL");
            decl->children.push_back(new ASTNode("TYPE",       tokens[i]));       // int
            decl->children.push_back(new ASTNode("IDENTIFIER", tokens[i + 1]));   // var
            decl->children.push_back(makeValNode(tokens[i + 3]));                  // val
            parent->children.push_back(decl);
            i += 5;  // consume: int <id> = <val> ;
            continue;
        }

        // ── return <var> ; ───────────────────────────────────────────
        if (tokens[i] == "return")
        {
            if (i + 1 >= sz)
            {
                cerr << "Error: incomplete return at token index " << i << "\n";
                i++;
                continue;
            }

            ASTNode* ret = new ASTNode("RETURN");
            ret->children.push_back(new ASTNode("IDENTIFIER", tokens[i + 1]));
            parent->children.push_back(ret);
            i += 3;  // consume: return <id> ;
            continue;
        }

        // ── for ( INIT ; COND ; UPDATE ) { BODY } ───────────────────
        if (tokens[i] == "for")
        {
            ASTNode* forNode = new ASTNode("FOR");
            i++;  // skip 'for'

            // skip '('
            if (i < sz && tokens[i] == "(") i++;

            // ── INIT: int <var> = <val> ; ────────────────────────────
            ASTNode* initDecl = new ASTNode("DECL");
            if (i < sz && tokens[i] == "int" && i + 4 < sz)
            {
                initDecl->children.push_back(new ASTNode("TYPE",       tokens[i]));
                initDecl->children.push_back(new ASTNode("IDENTIFIER", tokens[i + 1]));
                // skip '='  (tokens[i+2])
                initDecl->children.push_back(makeValNode(tokens[i + 3]));
                i += 5;  // int <id> = <val> ;
            }
            forNode->children.push_back(initDecl);  // children[0]

            // ── CONDITION: <var> <op> <val> ; ────────────────────────
            ASTNode* condNode = new ASTNode("CONDITION");
            if (i + 2 < sz && tokens[i + 1] != ";")
            {
                condNode->children.push_back(new ASTNode("IDENTIFIER", tokens[i]));
                condNode->value = tokens[i + 1];            // operator
                condNode->children.push_back(makeValNode(tokens[i + 2]));
                i += 3;
            }
            if (i < sz && tokens[i] == ";") i++;  // skip ';'
            forNode->children.push_back(condNode);  // children[1]

            // ── UPDATE: <var>++ / <var>-- / <var> += <val> ───────────
            ASTNode* updateNode = new ASTNode("UPDATE");
            if (i < sz && tokens[i] != ")")
            {
                string uVar = tokens[i];  i++;
                string uOp  = (i < sz) ? tokens[i] : "++"; i++;

                updateNode->value = uVar + uOp;
                updateNode->children.push_back(new ASTNode("IDENTIFIER", uVar));
                updateNode->children.push_back(new ASTNode("OPERATOR",   uOp));

                // compound assignment has an extra operand  (i += 2)
                if ((uOp == "+=" || uOp == "-=") && i < sz && tokens[i] != ")")
                {
                    updateNode->children.push_back(makeValNode(tokens[i]));
                    i++;
                }
            }
            if (i < sz && tokens[i] == ")") i++;  // skip ')'
            forNode->children.push_back(updateNode);  // children[2]

            // ── BODY: { <stmts> } ─────────────────────────────────────
            ASTNode* bodyNode = new ASTNode("BODY");
            if (i < sz && tokens[i] == "{")
            {
                i++;  // skip '{'
                parseStatements(tokens, i, bodyNode);   // recursive
                if (i < sz && tokens[i] == "}") i++;   // skip '}'
            }
            forNode->children.push_back(bodyNode);  // children[3]

            parent->children.push_back(forNode);
            continue;
        }

        i++;  // skip unrecognised token
    }
}

// ── Public API ──────────────────────────────────────────────────────────────

ASTNode* buildAST(vector<string> tokens)
{
    ASTNode* root = new ASTNode("PROGRAM");
    int i = 0;
    parseStatements(tokens, i, root);
    return root;
}

void printAST(ASTNode* node, int level)
{
    for (int i = 0; i < level; i++) cout << "  ";

    cout << node->type;
    if (!node->value.empty()) cout << " (" << node->value << ")";
    cout << "\n";

    for (auto child : node->children)
        printAST(child, level + 1);
}