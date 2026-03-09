#include "Lexer.h"
#include <sstream>

using namespace std;

// BUG FIX: The original tokenizer used (ss >> token) which splits ONLY on
// whitespace. This means "10;" stays as one token instead of "10" and ";".
// Since the sample input has no space before semicolons (e.g. "int a = 10;"),
// the parser reads tokens[i+3] = "10;" instead of "10", and tokens[i+4] = "int"
// (the next declaration) instead of ";". This causes i+=5 to skip over the
// entire next declaration, so "int b = 5;" and "int d = a;" are lost, and
// values like "10;", "8;", "d;" carry the semicolon into the AST nodes.
//
// Fix: after splitting on whitespace, further split each raw token character
// by character — emit punctuation characters (;, =, (, ), {, }, etc.) as
// their own separate tokens.

vector<string> tokenize(string code)
{
    vector<string> tokens;
    string raw;
    stringstream ss(code);

    // Step 1: split on whitespace as before
    while (ss >> raw)
    {
        // Step 2: re-scan each raw token and peel off leading/trailing punctuation
        string word;
        for (char ch : raw)
        {
            if (ch == ';' || ch == '=' || ch == '(' || ch == ')' ||
                ch == '{' || ch == '}' || ch == ',' || ch == ':')
            {
                // flush any accumulated word first
                if (!word.empty())
                {
                    tokens.push_back(word);
                    word.clear();
                }
                // emit punctuation as its own token
                tokens.push_back(string(1, ch));
            }
            else
            {
                word += ch;
            }
        }
        // flush remainder
        if (!word.empty())
        {
            tokens.push_back(word);
            word.clear();
        }
    }

    return tokens;
}