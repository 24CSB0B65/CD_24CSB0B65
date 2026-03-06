#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <cctype>

using namespace std;

/* =========================
   AST NODE
   ========================= */
struct ASTNode {
    string value;
    vector<ASTNode*> children;
    bool live = true;
    bool reachable = true;
    string securityTag = "SAFE";

    ASTNode(string v) : value(v) {}
};

/* =========================
   LEXICAL ANALYSIS (WEEK 2)
   ========================= */
enum TokenType { KEYWORD, IDENTIFIER, NUMBER, OPERATOR, PUNCTUATION, END };

struct Token {
    string value;
    TokenType type;
};

vector<string> keywords = {"int", "return"};

bool isKeyword(const string &word) {
    for (auto &k : keywords)
        if (k == word) return true;
    return false;
}

vector<Token> lexicalAnalyzer(const string &code) {
    vector<Token> tokens;
    string temp = "";

    for (size_t i = 0; i < code.size(); i++) {
        char c = code[i];

        if (isspace(c) || c == ';') {
            if (!temp.empty()) {
                Token t;
                t.value = temp;
                t.type = isKeyword(temp) ? KEYWORD :
                         (isdigit(temp[0]) ? NUMBER : IDENTIFIER);
                tokens.push_back(t);
                temp.clear();
            }
            if (c == ';')
                tokens.push_back({";", PUNCTUATION});
        }
        else if (c == '=' || c == '*' || c == '&') {
            if (!temp.empty()) {
                tokens.push_back({temp, IDENTIFIER});
                temp.clear();
            }
            tokens.push_back({string(1, c), OPERATOR});
        }
        else {
            temp += c;
        }
    }

    if (!temp.empty()) {
        tokens.push_back({temp, IDENTIFIER});
    }

    tokens.push_back({"", END}); // VERY IMPORTANT
    return tokens;
}

string tokenTypeToString(TokenType type) {
    switch (type) {
        case KEYWORD: return "KEYWORD";
        case IDENTIFIER: return "IDENTIFIER";
        case NUMBER: return "NUMBER";
        case OPERATOR: return "OPERATOR";
        case PUNCTUATION: return "PUNCTUATION";
        case END: return "END";
        default: return "UNKNOWN";
    }
}

void printTokens(const vector<Token> &tokens) {
    for (auto &t : tokens) {
        if (t.type != END)
            cout << t.value << " : " << tokenTypeToString(t.type) << endl;
    }
    cout << endl;
}

/* =========================
   PARSER
   ========================= */
class Parser {
    vector<Token> tokens;
    int idx = 0;

public:
    Parser(vector<Token> t) : tokens(t) {}

    Token peek() { return tokens[idx]; }
    Token get() { return tokens[idx++]; }

    ASTNode* parseStatement() {
        Token t = peek();

        // int a = 10;
        if (t.value == "int") {
            get();
            string id = get().value;
            ASTNode* n = new ASTNode("decl:" + id);

            if (peek().value == "=") {
                get();
                n->children.push_back(new ASTNode(get().value));
            }
            get(); // ;
            return n;
        }

        // assignment OR *p = 20
        if (t.type == IDENTIFIER || t.value == "*") {
            string id;

            if (t.value == "*") {
                get();                  // *
                id = "*" + get().value; // p → *p
            } else {
                id = get().value;
            }

            ASTNode* n = new ASTNode("assign:" + id);

            if (peek().value == "=") {
                get();
                n->children.push_back(new ASTNode(get().value));
            }
            get(); // ;
            return n;
        }

        // return a;
        if (t.value == "return") {
            get();
            ASTNode* n = new ASTNode("return");
            n->children.push_back(new ASTNode(get().value));
            get(); // ;
            return n;
        }

        return nullptr;
    }

    vector<ASTNode*> parse() {
        vector<ASTNode*> stmts;

        while (peek().type != END) {
            int oldIdx = idx;
            ASTNode* s = parseStatement();
            if (s)
                stmts.push_back(s);
            else
                idx++; // SAFETY

            if (idx == oldIdx) idx++; // EXTRA SAFETY
        }
        return stmts;
    }
};

/* =========================
   CFG (WEEK 3)
   ========================= */
vector<vector<int>> buildCFG(int n) {
    vector<vector<int>> cfg(n);
    for (int i = 0; i < n - 1; i++)
        cfg[i].push_back(i + 1);
    return cfg;
}

/* =========================
   REACHABILITY (WEEK 4)
   ========================= */
void dfs(int u, vector<vector<int>> &cfg, vector<bool> &vis) {
    vis[u] = true;
    for (int v : cfg[u])
        if (!vis[v]) dfs(v, cfg, vis);
}

/* =========================
   LIVENESS (WEEK 5–6)
   ========================= */
void livenessAnalysis(vector<ASTNode*> &stmts) {
    set<string> live;

    for (int i = stmts.size() - 1; i >= 0; i--) {
        ASTNode* s = stmts[i];

        if (s->value == "return") {
            live.insert(s->children[0]->value);
            continue;
        }

        string def;
        if (s->value.find("decl:") == 0)
            def = s->value.substr(5);
        else if (s->value.find("assign:") == 0)
            def = s->value.substr(7);

        if (!s->children.empty()) {
            string used = s->children[0]->value;
            if (!isdigit(used[0]))
                live.insert(used);
        }

        s->live = (live.find(def) != live.end());
        live.erase(def);
    }
}

/* ========================= 
  WEEK 7: DCE
   ========================= 
   */ void deadCodeElimination(vector<ASTNode*> &stmts) { 
    cout << "\nAfter Classical Dead Code Elimination:\n"; 
    for (auto s : stmts) {
         if (s->reachable && (s->live || s->value == "return"))
          cout << " " << s->value << endl;
         }
         }

/* =========================
   WEEK 8: SECURITY ANALYSIS
   ========================= */
void securityAnalysis(vector<ASTNode*> &stmts) {
    for (auto s : stmts) {

        if (!s->reachable && !s->live)
            s->securityTag = "SUSPICIOUS_UNREACHABLE_CODE";

        if (s->value.find("*") != string::npos ||
            (!s->children.empty() &&
             s->children[0]->value.find("&") != string::npos)) {

            s->securityTag = "UNSAFE_POINTER_DEPENDENCY";
        }

        if (!s->live &&
            s->value.find("assign:") == 0 &&
            s->securityTag == "SAFE") {

            s->securityTag = "POTENTIALLY_MALICIOUS";
        }
    }
}

void printAST(ASTNode* node, string prefix = "", bool isLast = true) {
    if (!node) return;

    cout << prefix;
    if (!prefix.empty()) {
        cout << (isLast ? "+-- " : "|-- ");
    }
    cout << node->value << endl;

    for (size_t i = 0; i < node->children.size(); i++) {
        bool isLastChild = (i == node->children.size() - 1);
        printAST(
            node->children[i],
            prefix + (isLast ? "    " : "|   "),
            isLastChild
        );
    }
}

/* =========================
   MAIN
   ========================= */
int main() {

    // POINTER FAILURE CASE (Week 8)
    string code =
        "int a = 10; "
        "int p = &a; "
        "*p = 20; "
        "return a;";

    auto tokens = lexicalAnalyzer(code);

    cout << "TOKENS:\n";
    printTokens(tokens);

    Parser parser(tokens);
    auto stmts = parser.parse();

    cout << "\nAST:\n";
for (auto s : stmts) {
    printAST(s);
}


    // CFG
    auto cfg = buildCFG(stmts.size());

    // Reachability
    vector<bool> vis(stmts.size(), false);
    dfs(0, cfg, vis);
    for (int i = 0; i < stmts.size(); i++)
        stmts[i]->reachable = vis[i];

    // Liveness
    livenessAnalysis(stmts);
    deadCodeElimination(stmts);

    // Security (Week 8)
    securityAnalysis(stmts);

    cout << "\nWEEK 8: LIMITATION & SECURITY ANALYSIS\n";
    for (auto s : stmts) {
        cout << s->value;
        if (!s->live) cout << " [DEAD]";
        if (!s->reachable) cout << " [UNREACHABLE]";
        cout << " => " << s->securityTag << endl;
    }
    
    return 0;
}