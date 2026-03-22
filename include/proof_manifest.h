#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

// ── Source Integrity ────────────────────────────────────────────────────────

struct ProofSourceIntegrity {
    std::string source_file;
    std::string source_sha256;
    int source_line_count = 0;
    int ast_node_count = 0;
    std::string ast_structure_hash;
};

// ── Declarations ────────────────────────────────────────────────────────────

struct ProofVariableDecl {
    std::string name;
    int declared_at_line = 0;
    std::string type; // "number", "string", "boolean", "list", "object", "any"
    bool is_mutated = false;
    std::vector<int> mutation_lines;
};

struct ProofFunctionDecl {
    std::string name;
    int declared_at_line = 0;
    std::vector<std::string> param_names;
    int param_count = 0;
};

struct ProofDeclarations {
    std::vector<ProofVariableDecl> variables;
    std::vector<ProofFunctionDecl> functions;
};

// ── Type Assertions ─────────────────────────────────────────────────────────

struct ProofTypeAssertion {
    int line = 0;
    std::string expression;
    std::string asserted_type;
    std::string reasoning;
};

// ── Safety Properties ───────────────────────────────────────────────────────

struct SafetyEvidence {
    std::string variable;
    int first_use_line = 0;
    int declaration_line = 0;
    bool declaration_precedes_use = true;
};

struct DivisionEvidence {
    int line = 0;
    std::string divisor_expression;
    bool divisor_is_literal_nonzero = false;
    std::string guarded_by; // optional: line of guard condition
};

struct LoopTerminationEvidence {
    int loop_line = 0;
    std::string loop_type; // "repeat_while", "for_each", "repeat_times"
    std::string termination_argument;
    std::string variant_expression;
    bool terminates = true;
};

struct SafetyProperty {
    bool verified = false;
};

struct ProofSafetyProperties {
    SafetyProperty no_uninitialized_access;
    std::vector<SafetyEvidence> uninitialized_evidence;

    SafetyProperty no_division_by_zero;
    std::vector<DivisionEvidence> division_evidence;

    SafetyProperty no_out_of_bounds_access;
    SafetyProperty no_unused_variables;
    SafetyProperty no_unreachable_code;

    SafetyProperty no_infinite_loops;
    std::vector<LoopTerminationEvidence> loop_evidence;

    SafetyProperty no_unresolved_holes;
    int hole_count = 0;
};

// ── Contracts ───────────────────────────────────────────────────────────────

struct FunctionContract {
    std::string name;
    std::vector<std::string> preconditions;
    std::vector<std::string> postconditions;
    std::string return_type;
    bool terminates = true;
    std::string termination_argument;
};

struct LoopInvariant {
    int loop_line = 0;
    std::string invariant;
    std::string variant;
    bool terminates = true;
};

struct ProofContracts {
    std::vector<FunctionContract> functions;
    std::vector<LoopInvariant> loop_invariants;
};

// ── Pattern Compliance ──────────────────────────────────────────────────────

struct PatternComplianceEntry {
    std::string pattern_id;
    bool satisfied = false;
    std::string evidence;
};

// ── Vulnerability Analysis ──────────────────────────────────────────────────

struct InputSource {
    int line = 0;
    std::string variable;
    std::string type_after_read;
    std::string reasoning;
};

struct ProofVulnerabilityAnalysis {
    std::vector<InputSource> input_sources;
    std::vector<std::string> injection_vectors;
    bool unbounded_allocations = false;
    std::string resource_reasoning;
};

// ── Top-level Manifest ──────────────────────────────────────────────────────

struct ProofManifest {
    std::string lumo_proof_version;
    ProofSourceIntegrity source_integrity;
    ProofDeclarations declarations;
    std::vector<ProofTypeAssertion> type_assertions;
    ProofSafetyProperties safety_properties;
    ProofContracts contracts;
    std::vector<PatternComplianceEntry> pattern_compliance;
    ProofVulnerabilityAnalysis vulnerability_analysis;
};

// ── Parsing ─────────────────────────────────────────────────────────────────

ProofManifest parseProofManifest(const std::string& jsonContent);
