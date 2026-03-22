#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "interpreter.h"
#include "static_analyzer.h"
#include "ast_hasher.h"
#include "proof_manifest.h"
#include "proof_verifier.h"
#include "pattern_checker.h"
#include "sha256.h"
#include <nlohmann/json.hpp>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// Generate a proof template JSON from the AST and static analysis
static std::string generateProofTemplate(const ProgramNode& ast,
                                         const AnalysisResult& analysis,
                                         const std::string& source,
                                         const std::string& filename) {
    using json = nlohmann::json;

    ASTHasher hasher;
    auto hashResult = hasher.compute(ast);

    // Count source lines
    int lineCount = 1;
    for (char c : source) if (c == '\n') ++lineCount;

    json j;
    j["lumo_proof_version"] = "1.0.0";

    // Source integrity
    j["source_integrity"] = {
        {"source_file", filename},
        {"source_sha256", sha256::hash(source)},
        {"source_line_count", lineCount},
        {"ast_node_count", hashResult.nodeCount},
        {"ast_structure_hash", hashResult.structureHash}
    };

    // Declarations
    json vars = json::array();
    for (auto& [name, info] : analysis.variables) {
        json v;
        v["name"] = name;
        v["declared_at_line"] = info.declarationLine;
        v["type"] = lumoTypeToString(info.type);
        v["is_mutated"] = analysis.mutatedVars.count(name) > 0;
        json mlines = json::array();
        auto it = analysis.mutationLines.find(name);
        if (it != analysis.mutationLines.end()) {
            for (int ml : it->second) mlines.push_back(ml);
        }
        v["mutation_lines"] = mlines;
        vars.push_back(v);
    }

    json funcs = json::array();
    for (auto& [name, sig] : analysis.functions) {
        json f;
        f["name"] = name;
        f["declared_at_line"] = sig.declarationLine;
        f["param_names"] = sig.paramNames;
        f["param_count"] = sig.paramCount;
        funcs.push_back(f);
    }

    j["declarations"] = {{"variables", vars}, {"functions", funcs}};

    // Type assertions — placeholder
    j["type_assertions"] = json::array();

    // Safety properties — fill in what the analyzer can determine
    j["safety_properties"] = {
        {"no_uninitialized_access", {{"verified", false}, {"evidence", json::array()}}},
        {"no_division_by_zero", {{"verified", false}, {"evidence", json::array()}}},
        {"no_out_of_bounds_access", {{"verified", false}, {"evidence", json::array()}}},
        {"no_infinite_loops", {{"verified", false}, {"evidence", json::array()}}},
        {"no_unresolved_holes", {{"verified", false}, {"hole_count", analysis.holeCount}}},
        {"no_unused_variables", {{"verified", false}}},
        {"no_unreachable_code", {{"verified", false}}}
    };

    // Contracts — placeholder
    j["contracts"] = {{"functions", json::array()}, {"loop_invariants", json::array()}};

    // Pattern compliance — placeholder
    j["pattern_compliance"] = json::array();

    // Vulnerability analysis — placeholder
    j["vulnerability_analysis"] = {
        {"input_sources", json::array()},
        {"injection_vectors", json::array()},
        {"resource_management", {{"unbounded_allocations", false}, {"reasoning", ""}}}
    };

    return j.dump(2);
}

int main(int argc, char* argv[]) {
    bool printAst  = false;
    bool validate  = false;
    bool strict    = false;
    bool checkOnly = false;
    bool genTemplate = false;
    std::string filename;
    std::string proofPath;
    std::string patternsPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--ast") {
            printAst = true;
        } else if (arg == "--validate") {
            validate = true;
        } else if (arg == "--strict") {
            strict = true;
        } else if (arg == "--check") {
            checkOnly = true;
            strict = true;
        } else if (arg == "--generate-proof-template") {
            genTemplate = true;
        } else if (arg == "--proof" && i + 1 < argc) {
            proofPath = argv[++i];
        } else if (arg == "--patterns" && i + 1 < argc) {
            patternsPath = argv[++i];
        } else {
            filename = arg;
        }
    }

    std::string source;

    if (!filename.empty()) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
    } else {
        // Read from stdin when no file is provided
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        source = buffer.str();
        if (source.empty()) {
            std::cerr << "Usage: lumo [options] <file.lumo>" << std::endl;
            std::cerr << "Options:" << std::endl;
            std::cerr << "  --ast                      Print AST" << std::endl;
            std::cerr << "  --validate                 Syntax validation only" << std::endl;
            std::cerr << "  --strict                   Require proof manifest" << std::endl;
            std::cerr << "  --check                    Strict + no execution" << std::endl;
            std::cerr << "  --proof <file>             Explicit proof path" << std::endl;
            std::cerr << "  --patterns <file>          Additional patterns" << std::endl;
            std::cerr << "  --generate-proof-template  Output proof skeleton" << std::endl;
            return 1;
        }
    }

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto program = parser.parse();

    if (printAst) {
        ASTPrinter printer;
        program->accept(printer);
        return 0;
    }

    // --validate: parse only
    if (validate && !strict) {
        if (parser.hasErrors()) {
            std::cerr << "INVALID" << std::endl;
            return 1;
        }
        std::cout << "OK" << std::endl;
        return 0;
    }

    // --generate-proof-template: parse + analyze, output template JSON
    if (genTemplate) {
        StaticAnalyzer analyzer;
        auto analysisResult = analyzer.analyze(*program);
        std::cout << generateProofTemplate(*program, analysisResult, source, filename) << std::endl;
        return 0;
    }

    // ── Strict mode pipeline ─────────────────────────────────────────────────
    if (strict) {
        // Step 1: Static analysis
        StaticAnalyzer analyzer;
        auto analysisResult = analyzer.analyze(*program);

        // Report static analysis errors
        bool hasStaticErrors = false;
        for (auto& e : analysisResult.errors) {
            if (e.severity == AnalysisError::Severity::ERROR) {
                std::cerr << "Static analysis error (line " << e.line << "): " << e.message << std::endl;
                hasStaticErrors = true;
            } else {
                std::cerr << "Static analysis warning (line " << e.line << "): " << e.message << std::endl;
            }
        }
        if (hasStaticErrors) {
            std::cerr << "Static analysis failed. Fix errors before providing a proof." << std::endl;
            return 1;
        }

        // Step 2: Load proof manifest
        if (proofPath.empty()) {
            proofPath = filename + ".proof.json";
        }
        std::string proofContent = readFile(proofPath);
        if (proofContent.empty()) {
            std::cerr << "Strict mode requires a proof manifest." << std::endl;
            std::cerr << "Expected: " << proofPath << std::endl;
            std::cerr << "Generate a template with: lumo --generate-proof-template " << filename << std::endl;
            return 1;
        }

        ProofManifest manifest;
        try {
            manifest = parseProofManifest(proofContent);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse proof manifest: " << e.what() << std::endl;
            return 1;
        }

        // Step 3: Verify proof
        ProofVerifier verifier(*program, analysisResult, manifest, source);
        auto verificationResult = verifier.verify();

        // Step 4: Pattern check
        PatternChecker patternChecker;
        if (!patternsPath.empty()) {
            std::string patternsContent = readFile(patternsPath);
            if (!patternsContent.empty()) {
                patternChecker.loadPatterns(patternsContent);
            }
        }
        auto patternResult = patternChecker.check(*program, manifest);

        // Print verification report
        std::cerr << verificationResult.report();

        // Print pattern results
        for (auto& w : patternResult.warnings) {
            std::cerr << "  [PATTERN WARNING] " << w << std::endl;
        }
        for (auto& e : patternResult.errors) {
            std::cerr << "  [PATTERN ERROR] " << e << std::endl;
        }

        if (verificationResult.hasErrors() || !patternResult.passed) {
            return 1;
        }

        // --check: don't execute, just verify
        if (checkOnly) {
            return 0;
        }
    }

    // ── Execute ──────────────────────────────────────────────────────────────
    try {
        Interpreter interpreter;
        interpreter.execute(*program);
    } catch (const HaltException& e) {
        std::cerr << "Program halted: " << e.message << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
