#include "Lexer.h"
#include <sstream>

using namespace std;

vector<string> tokenize(string code)
{
    vector<string> tokens;
    string token;
    stringstream ss(code);

    while (ss >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}