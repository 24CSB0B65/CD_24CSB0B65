#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

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
    // Read code from file
    ifstream inputFile("examples/sample_program.txt");

    if(!inputFile)
    {
        cout<<"Error opening sample program file\n";
        return 1;
    }

    stringstream buffer;
    buffer << inputFile.rdbuf();
    string code = buffer.str();

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

    vector<ASTNode*>& stmts = root->children;

    cout<<"\nLIVENESS ANALYSIS\n";
    livenessAnalysis(stmts);

    cout<<"\nDEAD CODE ELIMINATION\n";
    deadCodeElimination(stmts);

    cout<<"\nOPTIMIZED PROGRAM\n";

    for(auto stmt: stmts)
    {
        if(stmt->type=="DECL" && stmt->children.size() >= 3)
{
        string type = stmt->children[0]->value;
        string var  = stmt->children[1]->value;
        string val  = stmt->children[2]->value;

        string line = type + " " + var + " = " + val + ";";

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
    inputFile.close();

    return 0;
}