#include "proof_manifest.h"
#include <stdexcept>

using json = nlohmann::json;

static std::string getStr(const json& j, const std::string& key, const std::string& def = "") {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    return def;
}

static int getInt(const json& j, const std::string& key, int def = 0) {
    if (j.contains(key) && j[key].is_number_integer()) return j[key].get<int>();
    return def;
}

static bool getBool(const json& j, const std::string& key, bool def = false) {
    if (j.contains(key) && j[key].is_boolean()) return j[key].get<bool>();
    return def;
}

ProofManifest parseProofManifest(const std::string& jsonContent) {
    json j;
    try {
        j = json::parse(jsonContent);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("Failed to parse proof manifest JSON: ") + e.what());
    }

    ProofManifest m;
    m.lumo_proof_version = getStr(j, "lumo_proof_version", "1.0.0");

    // Source integrity
    if (j.contains("source_integrity")) {
        auto& si = j["source_integrity"];
        m.source_integrity.source_file = getStr(si, "source_file");
        m.source_integrity.source_sha256 = getStr(si, "source_sha256");
        m.source_integrity.source_line_count = getInt(si, "source_line_count");
        m.source_integrity.ast_node_count = getInt(si, "ast_node_count");
        m.source_integrity.ast_structure_hash = getStr(si, "ast_structure_hash");
    }

    // Declarations
    if (j.contains("declarations")) {
        auto& decls = j["declarations"];

        if (decls.contains("variables") && decls["variables"].is_array()) {
            for (auto& v : decls["variables"]) {
                ProofVariableDecl vd;
                vd.name = getStr(v, "name");
                vd.declared_at_line = getInt(v, "declared_at_line");
                vd.type = getStr(v, "type", "any");
                vd.is_mutated = getBool(v, "is_mutated");
                if (v.contains("mutation_lines") && v["mutation_lines"].is_array()) {
                    for (auto& ml : v["mutation_lines"]) {
                        if (ml.is_number_integer()) vd.mutation_lines.push_back(ml.get<int>());
                    }
                }
                m.declarations.variables.push_back(std::move(vd));
            }
        }

        if (decls.contains("functions") && decls["functions"].is_array()) {
            for (auto& f : decls["functions"]) {
                ProofFunctionDecl fd;
                fd.name = getStr(f, "name");
                fd.declared_at_line = getInt(f, "declared_at_line");
                fd.param_count = getInt(f, "param_count");
                if (f.contains("param_names") && f["param_names"].is_array()) {
                    for (auto& p : f["param_names"]) {
                        if (p.is_string()) fd.param_names.push_back(p.get<std::string>());
                    }
                }
                m.declarations.functions.push_back(std::move(fd));
            }
        }
    }

    // Type assertions
    if (j.contains("type_assertions") && j["type_assertions"].is_array()) {
        for (auto& ta : j["type_assertions"]) {
            ProofTypeAssertion a;
            a.line = getInt(ta, "line");
            a.expression = getStr(ta, "expression");
            a.asserted_type = getStr(ta, "asserted_type");
            a.reasoning = getStr(ta, "reasoning");
            m.type_assertions.push_back(std::move(a));
        }
    }

    // Safety properties
    if (j.contains("safety_properties")) {
        auto& sp = j["safety_properties"];

        auto parseSafety = [&](const std::string& key) -> SafetyProperty {
            if (sp.contains(key) && sp[key].is_object()) {
                return { getBool(sp[key], "verified") };
            }
            return { false };
        };

        m.safety_properties.no_uninitialized_access = parseSafety("no_uninitialized_access");
        m.safety_properties.no_division_by_zero = parseSafety("no_division_by_zero");
        m.safety_properties.no_out_of_bounds_access = parseSafety("no_out_of_bounds_access");
        m.safety_properties.no_unused_variables = parseSafety("no_unused_variables");
        m.safety_properties.no_unreachable_code = parseSafety("no_unreachable_code");
        m.safety_properties.no_infinite_loops = parseSafety("no_infinite_loops");
        m.safety_properties.no_unresolved_holes = parseSafety("no_unresolved_holes");

        if (sp.contains("no_unresolved_holes") && sp["no_unresolved_holes"].is_object()) {
            m.safety_properties.hole_count = getInt(sp["no_unresolved_holes"], "hole_count");
        }

        // Parse evidence arrays
        if (sp.contains("no_uninitialized_access") && sp["no_uninitialized_access"].contains("evidence")) {
            for (auto& e : sp["no_uninitialized_access"]["evidence"]) {
                SafetyEvidence ev;
                ev.variable = getStr(e, "variable");
                ev.first_use_line = getInt(e, "first_use_line");
                ev.declaration_line = getInt(e, "declaration_line");
                ev.declaration_precedes_use = getBool(e, "declaration_precedes_use", true);
                m.safety_properties.uninitialized_evidence.push_back(ev);
            }
        }

        if (sp.contains("no_division_by_zero") && sp["no_division_by_zero"].contains("evidence")) {
            for (auto& e : sp["no_division_by_zero"]["evidence"]) {
                DivisionEvidence de;
                de.line = getInt(e, "line");
                de.divisor_expression = getStr(e, "divisor_expression");
                de.divisor_is_literal_nonzero = getBool(e, "divisor_is_literal_nonzero");
                de.guarded_by = getStr(e, "guarded_by");
                m.safety_properties.division_evidence.push_back(de);
            }
        }

        if (sp.contains("no_infinite_loops") && sp["no_infinite_loops"].contains("evidence")) {
            for (auto& e : sp["no_infinite_loops"]["evidence"]) {
                LoopTerminationEvidence le;
                le.loop_line = getInt(e, "loop_line");
                le.loop_type = getStr(e, "loop_type");
                le.termination_argument = getStr(e, "termination_argument");
                le.variant_expression = getStr(e, "variant_expression");
                le.terminates = getBool(e, "terminates", true);
                m.safety_properties.loop_evidence.push_back(le);
            }
        }
    }

    // Contracts
    if (j.contains("contracts")) {
        auto& ct = j["contracts"];

        if (ct.contains("functions") && ct["functions"].is_array()) {
            for (auto& f : ct["functions"]) {
                FunctionContract fc;
                fc.name = getStr(f, "name");
                fc.return_type = getStr(f, "return_type", "any");
                fc.terminates = getBool(f, "terminates", true);
                fc.termination_argument = getStr(f, "termination_argument");
                if (f.contains("preconditions") && f["preconditions"].is_array()) {
                    for (auto& p : f["preconditions"])
                        if (p.is_string()) fc.preconditions.push_back(p.get<std::string>());
                }
                if (f.contains("postconditions") && f["postconditions"].is_array()) {
                    for (auto& p : f["postconditions"])
                        if (p.is_string()) fc.postconditions.push_back(p.get<std::string>());
                }
                m.contracts.functions.push_back(std::move(fc));
            }
        }

        if (ct.contains("loop_invariants") && ct["loop_invariants"].is_array()) {
            for (auto& li : ct["loop_invariants"]) {
                LoopInvariant inv;
                inv.loop_line = getInt(li, "loop_line");
                inv.invariant = getStr(li, "invariant");
                inv.variant = getStr(li, "variant");
                inv.terminates = getBool(li, "terminates", true);
                m.contracts.loop_invariants.push_back(inv);
            }
        }
    }

    // Pattern compliance
    if (j.contains("pattern_compliance") && j["pattern_compliance"].is_array()) {
        for (auto& pc : j["pattern_compliance"]) {
            PatternComplianceEntry pce;
            pce.pattern_id = getStr(pc, "pattern_id");
            pce.satisfied = getBool(pc, "satisfied");
            pce.evidence = getStr(pc, "evidence");
            m.pattern_compliance.push_back(std::move(pce));
        }
    }

    // Vulnerability analysis
    if (j.contains("vulnerability_analysis")) {
        auto& va = j["vulnerability_analysis"];

        if (va.contains("input_sources") && va["input_sources"].is_array()) {
            for (auto& is : va["input_sources"]) {
                InputSource src;
                src.line = getInt(is, "line");
                src.variable = getStr(is, "variable");
                src.type_after_read = getStr(is, "type_after_read");
                src.reasoning = getStr(is, "reasoning");
                m.vulnerability_analysis.input_sources.push_back(src);
            }
        }

        if (va.contains("injection_vectors") && va["injection_vectors"].is_array()) {
            for (auto& iv : va["injection_vectors"])
                if (iv.is_string()) m.vulnerability_analysis.injection_vectors.push_back(iv.get<std::string>());
        }

        if (va.contains("resource_management") && va["resource_management"].is_object()) {
            m.vulnerability_analysis.unbounded_allocations =
                getBool(va["resource_management"], "unbounded_allocations");
            m.vulnerability_analysis.resource_reasoning =
                getStr(va["resource_management"], "reasoning");
        }
    }

    return m;
}
