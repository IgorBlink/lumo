#pragma once
#include "ast.h"
#include "proof_manifest.h"
#include <string>
#include <vector>

struct PatternDef {
    std::string id;
    std::string description;
    std::string check;      // check type identifier
    std::string severity;   // "error" or "warning"
};

struct PatternResult {
    bool passed = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class PatternChecker {
public:
    PatternChecker();

    // Load additional patterns from a JSON file
    void loadPatterns(const std::string& jsonContent);

    // Run all pattern checks against the AST and proof
    PatternResult check(const ProgramNode& ast, const ProofManifest& proof);

private:
    std::vector<PatternDef> patterns;

    // Built-in check implementations
    bool checkIntentRequired(const ProgramNode& ast);
    bool checkNoHoles(const ProgramNode& ast);
    bool checkExhaustiveMatch(const ProgramNode& ast);
    bool checkFunctionsMustReturn(const ProgramNode& ast);
};
