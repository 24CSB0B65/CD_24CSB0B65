#include "Security.h"
#include <iostream>

using namespace std;

void securityAnalysis(vector<ASTNode*>& stmts) {

    cout << "\nSECURITY ANALYSIS\n";

    for (auto s : stmts) {

        if (s->value == "*" || s->value == "&") {

            s->securityTag = "SIDE_EFFECT";

            cout << s->value << " -> SIDE EFFECT\n";
        }
    }
}