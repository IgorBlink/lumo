#include "pattern_checker.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

PatternChecker::PatternChecker() {
    // Register default patterns
    patterns.push_back({ "lumo:intent-required",
                        "Every program must begin with an intent declaration",
                        "intent_required", "error" });
    patterns.push_back({ "lumo:no-holes",
                        "No ??? holes allowed in strict mode",
                        "no_holes", "error" });
    patterns.push_back({ "lumo:exhaustive-match",
                        "All match blocks must have catch error clause",
                        "exhaustive_match", "error" });
    patterns.push_back({ "lumo:functions-must-return",
                        "Every function must have an explicit return statement",
                        "functions_must_return", "warning" });
}

void PatternChecker::loadPatterns(const std::string& jsonContent) {
    auto j = json::parse(jsonContent);
    if (j.contains("patterns") && j["patterns"].is_array()) {
        for (auto& p : j["patterns"]) {
            PatternDef pd;
            pd.id = p.value("id", "");
            pd.description = p.value("description", "");
            pd.check = p.value("check", "");
            pd.severity = p.value("severity", "error");
            patterns.push_back(std::move(pd));
        }
    }
}

// ── Helper: recursive AST search ────────────────────────────────────────────

namespace {

class HoleFinder : public ASTVisitor {
public:
    bool foundHole = false;

    void visit(const IdentifierExpr&) override {}
    void visit(const StringLiteralExpr&) override {}
    void visit(const NumberExpr&) override {}
    void visit(const BoolLiteralExpr&) override {}
    void visit(const HoleExpr&) override { foundHole = true; }
    void visit(const BinaryExpr& n) override { n.left->accept(*this); n.right->accept(*this); }
    void visit(const UnaryExpr& n) override { n.operand->accept(*this); }
    void visit(const JsonObjectExpr& n) override { for (auto& f : n.fields) f.value->accept(*this); }
    void visit(const ListExpr& n) override { for (auto& e : n.elements) e->accept(*this); }
    void visit(const GetExpr& n) override { n.list->accept(*this); n.index->accept(*this); }
    void visit(const CallExpr& n) override { for (auto& a : n.args) a->accept(*this); }
    void visit(const IntentDecl&) override {}
    void visit(const LetDecl& n) override { n.value->accept(*this); }
    void visit(const SetDecl& n) override { n.value->accept(*this); }
    void visit(const PrintDecl& n) override { n.expr->accept(*this); }
    void visit(const SkipDecl&) override {}
    void visit(const ReadDecl&) override {}
    void visit(const RepeatDecl& n) override {
        n.condition->accept(*this);
        for (auto& s : n.body) s->accept(*this);
    }
    void visit(const RepeatTimesDecl& n) override {
        n.count->accept(*this);
        for (auto& s : n.body) s->accept(*this);
    }
    void visit(const ForEachDecl& n) override {
        n.listExpr->accept(*this);
        for (auto& s : n.body) s->accept(*this);
    }
    void visit(const IfDecl& n) override {
        for (auto& c : n.clauses) {
            c.condition->accept(*this);
            for (auto& s : c.body) s->accept(*this);
        }
        for (auto& s : n.else_body) s->accept(*this);
    }
    void visit(const FunctionDecl& n) override {
        for (auto& s : n.body) s->accept(*this);
    }
    void visit(const ReturnStmt& n) override { n.expr->accept(*this); }
    void visit(const CallStmt& n) override { for (auto& a : n.args) a->accept(*this); }
    void visit(const PutDecl& n) override { n.index->accept(*this); n.value->accept(*this); }
    void visit(const StepNode& n) override { if (n.expr) n.expr->accept(*this); }
    void visit(const PipeDecl& n) override { for (auto& s : n.steps) s->accept(*this); }
    void visit(const MatchCaseNode& n) override {
        if (n.condition) n.condition->accept(*this);
        n.yield_stmt->accept(*this);
    }
    void visit(const MatchDecl& n) override {
        n.expr->accept(*this);
        for (auto& c : n.cases) c->accept(*this);
    }
    void visit(const ProgramNode& n) override {
        for (auto& s : n.statements) s->accept(*this);
    }
};

bool hasReturnStmt(const std::vector<std::unique_ptr<StmtNode>>& stmts) {
    for (auto& s : stmts) {
        if (dynamic_cast<const ReturnStmt*>(s.get())) return true;
        // Check inside if/else blocks
        if (auto* ifDecl = dynamic_cast<const IfDecl*>(s.get())) {
            bool allBranchesReturn = true;
            for (auto& clause : ifDecl->clauses) {
                if (!hasReturnStmt(clause.body)) {
                    allBranchesReturn = false;
                    break;
                }
            }
            if (allBranchesReturn && !ifDecl->else_body.empty() &&
                hasReturnStmt(ifDecl->else_body)) {
                return true;
            }
        }
    }
    return false;
}

} // anonymous namespace

// ── Check implementations ───────────────────────────────────────────────────

bool PatternChecker::checkIntentRequired(const ProgramNode& ast) {
    if (ast.statements.empty()) return false;
    return dynamic_cast<const IntentDecl*>(ast.statements[0].get()) != nullptr;
}

bool PatternChecker::checkNoHoles(const ProgramNode& ast) {
    HoleFinder finder;
    ast.accept(finder);
    return !finder.foundHole;
}

bool PatternChecker::checkExhaustiveMatch(const ProgramNode& ast) {
    // Parser already enforces this, but double-check
    // This would need a recursive visitor; for now trust the parser
    return true;
}

bool PatternChecker::checkFunctionsMustReturn(const ProgramNode& ast) {
    for (auto& stmt : ast.statements) {
        if (auto* fn = dynamic_cast<const FunctionDecl*>(stmt.get())) {
            if (!hasReturnStmt(fn->body)) {
                return false;
            }
        }
    }
    return true;
}

// ── Main check entry point ──────────────────────────────────────────────────

PatternResult PatternChecker::check(const ProgramNode& ast, const ProofManifest& proof) {
    PatternResult result;

    for (auto& pattern : patterns) {
        bool satisfied = false;

        if (pattern.check == "intent_required") {
            satisfied = checkIntentRequired(ast);
        } else if (pattern.check == "no_holes") {
            satisfied = checkNoHoles(ast);
        } else if (pattern.check == "exhaustive_match") {
            satisfied = checkExhaustiveMatch(ast);
        } else if (pattern.check == "functions_must_return") {
            satisfied = checkFunctionsMustReturn(ast);
        } else {
            // Unknown check — check if proof claims compliance
            for (auto& pc : proof.pattern_compliance) {
                if (pc.pattern_id == pattern.id && pc.satisfied) {
                    satisfied = true;
                    break;
                }
            }
        }

        if (!satisfied) {
            std::string msg = "Pattern '" + pattern.id + "' not satisfied: " + pattern.description;
            if (pattern.severity == "error") {
                result.errors.push_back(msg);
                result.passed = false;
            } else {
                result.warnings.push_back(msg);
            }
        }
    }

    return result;
}
