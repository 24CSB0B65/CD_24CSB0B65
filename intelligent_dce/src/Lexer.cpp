#include "Lexer.h"
#include <cctype>

using namespace std;

vector<string> tokenize(string code)
{
    vector<string> tokens;
    int i = 0;
    int n = (int)code.size();

    // Punctuation chars that are always split as single tokens
    // (unless they form a two-char operator)
    static const string PUNCT = ";=(){},:<>+*!-/";

    while (i < n)
    {
        // ── Skip whitespace ──────────────────────────────────────────
        if (isspace(code[i])) { i++; continue; }

        // ── Two-character operators (check BEFORE single-char) ───────
        if (i + 1 < n)
        {
            string two(1, code[i]);
            two += code[i + 1];
            if (two == "++" || two == "--" ||
                two == "<=" || two == ">=" ||
                two == "==" || two == "!=" ||
                two == "+=" || two == "-=")
            {
                tokens.push_back(two);
                i += 2;
                continue;
            }
        }

        // ── Single-character punctuation / operator ──────────────────
        if (PUNCT.find(code[i]) != string::npos)
        {
            tokens.push_back(string(1, code[i]));
            i++;
            continue;
        }

        // ── Identifier / keyword / integer literal ───────────────────
        string word;
        while (i < n &&
               !isspace(code[i]) &&
               PUNCT.find(code[i]) == string::npos)
        {
            // Stop before a two-char operator (e.g. 'i' before '++')
            if (i + 1 < n)
            {
                string two(1, code[i]);
                two += code[i + 1];
                if (two == "++" || two == "--" ||
                    two == "<=" || two == ">=" ||
                    two == "==" || two == "!=" ||
                    two == "+=" || two == "-=")
                    break;
            }
            word += code[i++];
        }
        if (!word.empty()) tokens.push_back(word);
    }

    return tokens;
}