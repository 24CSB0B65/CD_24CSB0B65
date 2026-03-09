#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include "Lexer.h"
#include "Parser.h"
#include "ASTNode.h"
#include "Analysis.h"

using namespace std;

void printASTToFile(ASTNode* node, ofstream& fout, int level = 0)
{
    for(int i=0;i<level;i++)
        fout<<"  ";

    fout<<node->type;

    if(node->value!="")
        fout<<" ("<<node->value<<")";

    fout<<endl;

    for(auto c: node->children)
        printASTToFile(c,fout,level+1);
}

int main()
{
    string code =
        "int a = 10; "
        "int b = 5; "
        "int c = 8; "
        "return a;";

    cout<<"SOURCE CODE\n"<<code<<endl;

    vector<string> tokens = tokenize(code);

    ofstream tokenFile("tokens.txt");
    ofstream astFile("ast.txt");
    ofstream optFile("optimized_code.txt");

    cout<<"\nTOKENS\n";
    for(auto t: tokens)
    {
        cout<<t<<endl;
        tokenFile<<t<<endl;
    }

    ASTNode* root = buildAST(tokens);

    cout<<"\nAST\n";
    printAST(root);

    astFile<<"AST\n";
    printASTToFile(root,astFile);

    vector<ASTNode*> stmts = root->children;

    cout<<"\nLIVENESS ANALYSIS\n";
    livenessAnalysis(stmts);

    cout<<"\nDEAD CODE ELIMINATION\n";
    deadCodeElimination(stmts);

    cout<<"\nOPTIMIZED PROGRAM\n";

    for(auto stmt: stmts)
    {
        if(stmt->type=="DECL")
        {
            string line =
                stmt->children[0]->value+" "+
                stmt->children[1]->value+
                " = "+
                stmt->children[2]->value+";";

            cout<<line<<endl;
            optFile<<line<<endl;
        }

        else if(stmt->type=="RETURN")
        {
            string line = "return "+stmt->children[0]->value+";";

            cout<<line<<endl;
            optFile<<line<<endl;
        }
    }

    tokenFile.close();
    astFile.close();
    optFile.close();

    return 0;
}