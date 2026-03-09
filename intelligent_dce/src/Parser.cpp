#include "Parser.h"
#include <iostream>

using namespace std;

ASTNode* buildAST(vector<string> tokens)
{
    ASTNode* root = new ASTNode("PROGRAM");

    for (int i = 0; i < tokens.size(); i++)
    {
        if (tokens[i] == "int")
        {
            ASTNode* decl = new ASTNode("DECL");

            ASTNode* type = new ASTNode("TYPE", tokens[i]);
            ASTNode* var = new ASTNode("VAR", tokens[i + 1]);
            ASTNode* value = new ASTNode("VALUE", tokens[i + 3]);

            decl->children.push_back(type);
            decl->children.push_back(var);
            decl->children.push_back(value);

            root->children.push_back(decl);
            i += 4;
        }

        else if (tokens[i] == "return")
        {
            ASTNode* ret = new ASTNode("RETURN");
            ASTNode* var = new ASTNode("VAR", tokens[i + 1]);

            ret->children.push_back(var);
            root->children.push_back(ret);
        }
    }

    return root;
}

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