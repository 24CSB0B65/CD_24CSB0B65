#include "Parser.h"
#include <iostream>

using namespace std;

ASTNode* buildAST(vector<string> tokens)
{
    ASTNode* root = new ASTNode("PROGRAM");

    for (int i = 0; i < (int)tokens.size();)
    {
        if (tokens[i] == "int")
        {
            // BUG FIX 1: Guard against out-of-bounds access before indexing i+1, i+3
            if (i + 4 >= (int)tokens.size())
            {
                cerr << "Error: incomplete declaration at token index " << i << endl;
                break;
            }

            ASTNode* decl = new ASTNode("DECL");

            ASTNode* type = new ASTNode("TYPE", tokens[i]);
            ASTNode* id   = new ASTNode("IDENTIFIER", tokens[i + 1]);

            // BUG FIX 2: tokens[i+3] may be a variable name (e.g., "int d = a ;")
            // Distinguish between a literal value and an identifier used on the RHS.
            // If the RHS token is a digit string it is a VALUE; otherwise it is an IDENTIFIER.
            bool isLiteral = !tokens[i + 3].empty() &&
                             (isdigit(tokens[i + 3][0]) ||
                              (tokens[i + 3][0] == '-' && tokens[i + 3].size() > 1));

            ASTNode* val = isLiteral
                               ? new ASTNode("VALUE",      tokens[i + 3])
                               : new ASTNode("IDENTIFIER", tokens[i + 3]);

            decl->children.push_back(type);
            decl->children.push_back(id);
            decl->children.push_back(val);

            root->children.push_back(decl);
            i += 5;   // int <id> = <val> ;
        }
        else if (tokens[i] == "return")
        {
            // BUG FIX 3: Guard against out-of-bounds access
            if (i + 1 >= (int)tokens.size())
            {
                cerr << "Error: incomplete return statement at token index " << i << endl;
                break;
            }

            ASTNode* ret = new ASTNode("RETURN");
            ASTNode* id  = new ASTNode("IDENTIFIER", tokens[i + 1]);
            ret->children.push_back(id);

            root->children.push_back(ret);
            i += 3;   // return <id> ;
        }
        else
        {
            i++;
        }
    }

    return root;
}

// printAST is correct — no changes needed here.
void printAST(ASTNode* node, int level)
{
    for (int i = 0; i < level; i++)
        cout << "  ";

    cout << node->type;

    if (node->value != "")
        cout << " (" << node->value << ")";

    cout << endl;

    for (auto child : node->children)
        printAST(child, level + 1);
}