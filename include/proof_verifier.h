#pragma once
#include "ast.h"
#include "proof_manifest.h"
#include "static_analyzer.h"
#include "ast_hasher.h"
#include <string>
#include <vector>

struct VerificationEntry {
    enum class Level { VERIFIED, ATTESTED, ERROR };
    Level level;
    std::string message;
};

struct VerificationResult {
    bool passed = false;
    int mechanicallyVerified = 0;
    int attestationsAccepted = 0;
    std::vector<VerificationEntry> entries;

    void addVerified(const std::string& msg);
    void addAttested(const std::string& msg);
    void addError(const std::string& msg);
    bool hasErrors() const;
    std::string report() const;
};

class ProofVerifier {
public:
    ProofVerifier(const ProgramNode& ast,
                  const AnalysisResult& analysis,
                  const ProofManifest& proof,
                  const std::string& sourceText);

    VerificationResult verify();

private:
    const ProgramNode& ast;
    const AnalysisResult& analysis;
    const ProofManifest& proof;
    const std::string& sourceText;
    VerificationResult result;

    // Verification phases
    void verifyIntegrity();
    void verifyStructure();
    void verifyTypes();
    void verifySafety();
    void verifyContracts();

    // Helpers
    int countSourceLines() const;
    std::string computeSourceHash() const;
    ASTHashResult computeASTHash() const;
};
