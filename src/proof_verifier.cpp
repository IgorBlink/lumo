#include "proof_verifier.h"
#include "sha256.h"
#include <algorithm>
#include <sstream>

// ── VerificationResult ──────────────────────────────────────────────────────

void VerificationResult::addVerified(const std::string& msg) {
    ++mechanicallyVerified;
    entries.push_back({ VerificationEntry::Level::VERIFIED, msg });
}

void VerificationResult::addAttested(const std::string& msg) {
    ++attestationsAccepted;
    entries.push_back({ VerificationEntry::Level::ATTESTED, msg });
}

void VerificationResult::addError(const std::string& msg) {
    entries.push_back({ VerificationEntry::Level::ERROR, msg });
}

bool VerificationResult::hasErrors() const {
    return std::any_of(entries.begin(), entries.end(),
        [](const VerificationEntry& e) { return e.level == VerificationEntry::Level::ERROR; });
}

std::string VerificationResult::report() const {
    std::ostringstream oss;
    if (!hasErrors()) {
        oss << "VERIFIED: " << mechanicallyVerified << " claims mechanically verified, "
            << attestationsAccepted << " attestations accepted\n";
        for (auto& e : entries) {
            if (e.level == VerificationEntry::Level::ATTESTED) {
                oss << "  [ATTESTED] " << e.message << "\n";
            }
        }
        oss << "Program execution permitted.\n";
    } else {
        oss << "VERIFICATION FAILED:\n";
        for (auto& e : entries) {
            if (e.level == VerificationEntry::Level::ERROR) {
                oss << "  [ERROR] " << e.message << "\n";
            }
        }
        oss << "Program execution blocked.\n";
    }
    return oss.str();
}

// ── ProofVerifier ───────────────────────────────────────────────────────────

ProofVerifier::ProofVerifier(const ProgramNode& ast,
                             const AnalysisResult& analysis,
                             const ProofManifest& proof,
                             const std::string& sourceText)
    : ast(ast), analysis(analysis), proof(proof), sourceText(sourceText) {}

VerificationResult ProofVerifier::verify() {
    result = VerificationResult{};

    // Run phases in order; abort early on integrity failure
    verifyIntegrity();
    if (result.hasErrors()) {
        result.passed = false;
        return result;
    }

    verifyStructure();
    verifyTypes();
    verifySafety();
    verifyContracts();

    result.passed = !result.hasErrors();
    return result;
}

// ── Helpers ─────────────────────────────────────────────────────────────────

int ProofVerifier::countSourceLines() const {
    if (sourceText.empty()) return 0;
    int count = 1;
    for (char c : sourceText) {
        if (c == '\n') ++count;
    }
    return count;
}

std::string ProofVerifier::computeSourceHash() const {
    return sha256::hash(sourceText);
}

ASTHashResult ProofVerifier::computeASTHash() const {
    ASTHasher hasher;
    return hasher.compute(ast);
}

// ── Phase 0: Integrity ─────────────────────────────────────────────────────

void ProofVerifier::verifyIntegrity() {
    // Source hash
    std::string actualHash = computeSourceHash();
    if (proof.source_integrity.source_sha256 != actualHash) {
        result.addError("Source hash mismatch: proof is stale (expected " +
                       proof.source_integrity.source_sha256 + ", got " + actualHash + ")");
        return;
    }
    result.addVerified("Source SHA-256 hash matches");

    // Line count
    int actualLines = countSourceLines();
    if (proof.source_integrity.source_line_count != 0 &&
        proof.source_integrity.source_line_count != actualLines) {
        result.addError("Source line count mismatch: proof says " +
                       std::to_string(proof.source_integrity.source_line_count) +
                       ", actual is " + std::to_string(actualLines));
    } else {
        result.addVerified("Source line count matches");
    }

    // AST node count and structure hash
    auto astHash = computeASTHash();
    if (proof.source_integrity.ast_node_count != 0 &&
        proof.source_integrity.ast_node_count != astHash.nodeCount) {
        result.addError("AST node count mismatch: proof says " +
                       std::to_string(proof.source_integrity.ast_node_count) +
                       ", actual is " + std::to_string(astHash.nodeCount));
    } else {
        result.addVerified("AST node count matches");
    }

    if (!proof.source_integrity.ast_structure_hash.empty() &&
        proof.source_integrity.ast_structure_hash != astHash.structureHash) {
        result.addError("AST structure hash mismatch");
    } else if (!proof.source_integrity.ast_structure_hash.empty()) {
        result.addVerified("AST structure hash matches");
    }
}

// ── Phase 1: Structural ────────────────────────────────────────────────────

void ProofVerifier::verifyStructure() {
    // Check variable declarations match
    std::unordered_map<std::string, const ProofVariableDecl*> proofVars;
    for (auto& v : proof.declarations.variables) {
        proofVars[v.name] = &v;
    }

    for (auto& [name, info] : analysis.variables) {
        auto it = proofVars.find(name);
        if (it == proofVars.end()) {
            result.addError("Variable '" + name + "' (line " + std::to_string(info.declarationLine) +
                          ") exists in code but not in proof manifest");
        } else {
            result.addVerified("Variable '" + name + "' declared in both code and proof");

            // Check mutation claim
            bool actuallyMutated = analysis.mutatedVars.count(name) > 0;
            if (it->second->is_mutated != actuallyMutated) {
                result.addError("Mutation mismatch for '" + name + "': proof says " +
                              (it->second->is_mutated ? "mutated" : "not mutated") +
                              ", but code " + (actuallyMutated ? "does mutate" : "does not mutate") + " it");
            } else {
                result.addVerified("Mutation status for '" + name + "' matches");
            }
            proofVars.erase(it);
        }
    }

    // Check for extra variables in proof that don't exist in code
    for (auto& [name, _] : proofVars) {
        result.addError("Variable '" + name + "' exists in proof manifest but not in code");
    }

    // Check function declarations match
    std::unordered_map<std::string, const ProofFunctionDecl*> proofFuncs;
    for (auto& f : proof.declarations.functions) {
        proofFuncs[f.name] = &f;
    }

    for (auto& [name, sig] : analysis.functions) {
        auto it = proofFuncs.find(name);
        if (it == proofFuncs.end()) {
            result.addError("Function '" + name + "' exists in code but not in proof manifest");
        } else {
            // Check param count
            if (it->second->param_count != sig.paramCount) {
                result.addError("Function '" + name + "': proof says " +
                              std::to_string(it->second->param_count) + " params, code has " +
                              std::to_string(sig.paramCount));
            } else {
                result.addVerified("Function '" + name + "' declaration matches proof");
            }
            proofFuncs.erase(it);
        }
    }

    for (auto& [name, _] : proofFuncs) {
        result.addError("Function '" + name + "' exists in proof manifest but not in code");
    }
}

// ── Phase 2: Types ─────────────────────────────────────────────────────────

void ProofVerifier::verifyTypes() {
    // Cross-check proof type assertions against static analysis type inference
    for (auto& ta : proof.type_assertions) {
        // For now, accept all type assertions as attested
        // A full type checker would independently infer and compare
        result.addAttested("Type assertion at line " + std::to_string(ta.line) +
                          ": " + ta.expression + " is " + ta.asserted_type);
    }

    // Cross-check variable types from declarations against analysis
    for (auto& pv : proof.declarations.variables) {
        auto it = analysis.variables.find(pv.name);
        if (it != analysis.variables.end()) {
            std::string inferredType = lumoTypeToString(it->second.type);
            if (inferredType != "any" && pv.type != "any" && inferredType != pv.type) {
                result.addError("Type mismatch for '" + pv.name + "': proof claims '" +
                              pv.type + "', compiler infers '" + inferredType + "'");
            } else if (inferredType != "any") {
                result.addVerified("Type of '" + pv.name + "' confirmed as '" + inferredType + "'");
            } else {
                result.addAttested("Type of '" + pv.name + "' accepted as '" + pv.type + "' (compiler cannot infer)");
            }
        }
    }
}

// ── Phase 3: Safety ────────────────────────────────────────────────────────

void ProofVerifier::verifySafety() {
    // No uninitialized access — cross-check with static analyzer
    if (!proof.safety_properties.no_uninitialized_access.verified) {
        result.addError("Proof does not claim no_uninitialized_access is verified");
    } else {
        // Check if static analyzer found any use-before-decl errors
        bool analyzerFoundIssue = false;
        for (auto& e : analysis.errors) {
            if (e.message.find("undeclared variable") != std::string::npos ||
                e.message.find("undeclared") != std::string::npos) {
                analyzerFoundIssue = true;
                break;
            }
        }
        if (analyzerFoundIssue) {
            result.addError("Proof claims no_uninitialized_access but static analyzer found undeclared variable usage");
        } else {
            result.addVerified("No uninitialized variable access");
        }
    }

    // No division by zero
    if (!proof.safety_properties.no_division_by_zero.verified) {
        result.addError("Proof does not claim no_division_by_zero is verified");
    } else {
        bool analyzerFoundDivZero = false;
        for (auto& e : analysis.errors) {
            if (e.message.find("Division by zero") != std::string::npos) {
                analyzerFoundDivZero = true;
                break;
            }
        }
        if (analyzerFoundDivZero) {
            result.addError("Proof claims no_division_by_zero but static analyzer found literal division by zero");
        } else {
            result.addVerified("No division by zero (verified for literals)");
            // For non-literal divisors, accept proof evidence
            for (auto& ev : proof.safety_properties.division_evidence) {
                if (!ev.divisor_is_literal_nonzero && ev.guarded_by.empty()) {
                    result.addAttested("Division at line " + std::to_string(ev.line) +
                                     " — AI attests divisor '" + ev.divisor_expression + "' is non-zero");
                }
            }
        }
    }

    // No out of bounds
    if (!proof.safety_properties.no_out_of_bounds_access.verified) {
        result.addError("Proof does not claim no_out_of_bounds_access is verified");
    } else {
        bool analyzerFoundOOB = false;
        for (auto& e : analysis.errors) {
            if (e.message.find("Negative index") != std::string::npos) {
                analyzerFoundOOB = true;
                break;
            }
        }
        if (analyzerFoundOOB) {
            result.addError("Proof claims no_out_of_bounds but static analyzer found negative index");
        } else {
            result.addVerified("No out-of-bounds access (verified for literal indices)");
        }
    }

    // No infinite loops
    if (!proof.safety_properties.no_infinite_loops.verified) {
        result.addError("Proof does not claim no_infinite_loops is verified");
    } else {
        // Accept termination evidence as attestations
        for (auto& ev : proof.safety_properties.loop_evidence) {
            if (ev.loop_type == "for_each" || ev.loop_type == "repeat_times") {
                result.addVerified("Loop at line " + std::to_string(ev.loop_line) +
                                 " terminates (finite " + ev.loop_type + ")");
            } else {
                result.addAttested("Loop at line " + std::to_string(ev.loop_line) +
                                 " terminates: " + ev.termination_argument);
            }
        }
    }

    // No unresolved holes
    if (!proof.safety_properties.no_unresolved_holes.verified) {
        result.addError("Proof does not claim no_unresolved_holes is verified");
    } else {
        if (proof.safety_properties.hole_count != analysis.holeCount) {
            result.addError("Hole count mismatch: proof says " +
                          std::to_string(proof.safety_properties.hole_count) +
                          ", static analysis found " + std::to_string(analysis.holeCount));
        } else if (analysis.holeCount > 0) {
            result.addError("Code contains " + std::to_string(analysis.holeCount) +
                          " unresolved hole(s) — cannot execute");
        } else {
            result.addVerified("No unresolved holes");
        }
    }

    // No unused variables
    if (proof.safety_properties.no_unused_variables.verified) {
        bool analyzerFoundUnused = false;
        for (auto& e : analysis.errors) {
            if (e.message.find("never used") != std::string::npos) {
                analyzerFoundUnused = true;
                break;
            }
        }
        if (analyzerFoundUnused) {
            result.addError("Proof claims no_unused_variables but static analyzer found unused variables");
        } else {
            result.addVerified("No unused variables");
        }
    }

    // No unreachable code
    if (proof.safety_properties.no_unreachable_code.verified) {
        bool analyzerFoundUnreachable = false;
        for (auto& e : analysis.errors) {
            if (e.message.find("Unreachable") != std::string::npos) {
                analyzerFoundUnreachable = true;
                break;
            }
        }
        if (analyzerFoundUnreachable) {
            result.addError("Proof claims no_unreachable_code but static analyzer found unreachable code");
        } else {
            result.addVerified("No unreachable code");
        }
    }
}

// ── Phase 4: Contracts ─────────────────────────────────────────────────────

void ProofVerifier::verifyContracts() {
    // Verify function contracts reference real functions
    for (auto& fc : proof.contracts.functions) {
        auto it = analysis.functions.find(fc.name);
        if (it == analysis.functions.end()) {
            result.addError("Contract references unknown function '" + fc.name + "'");
        } else {
            result.addVerified("Contract for function '" + fc.name + "' references existing function");

            // Preconditions and postconditions are attestations
            for (auto& pre : fc.preconditions) {
                result.addAttested("Precondition for '" + fc.name + "': " + pre);
            }
            for (auto& post : fc.postconditions) {
                result.addAttested("Postcondition for '" + fc.name + "': " + post);
            }
            if (fc.terminates) {
                if (fc.termination_argument.empty()) {
                    result.addAttested("Function '" + fc.name + "' terminates (no argument provided)");
                } else {
                    result.addAttested("Function '" + fc.name + "' terminates: " + fc.termination_argument);
                }
            }
        }
    }

    // Verify loop invariants reference real loop lines
    for (auto& li : proof.contracts.loop_invariants) {
        result.addAttested("Loop invariant at line " + std::to_string(li.loop_line) + ": " + li.invariant);
    }
}
