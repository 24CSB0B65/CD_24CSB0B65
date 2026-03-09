#ifndef ASTNODE_H
#define ASTNODE_H

#include <vector>
#include <string>

struct ASTNode
{
    std::string type;
    std::string value;

    // Security property
    std::string securityTag;

    std::vector<ASTNode*> children;

    ASTNode(std::string t, std::string v = "")
    {
        type = t;
        value = v;
        securityTag = "SAFE";
    }
};

#endif