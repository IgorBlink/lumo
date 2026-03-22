#include "ast_hasher.h"
#include "sha256.h"

void ASTHasher::record(const std::string& nodeType) {
    ++nodeCount;
    signature += nodeType + ";";
}

ASTHashResult ASTHasher::compute(const ProgramNode& program) {
    nodeCount = 0;
    signature.clear();
    program.accept(*this);
    return { nodeCount, sha256::hash(signature) };
}

// ── Expressions ─────────────────────────────────────────────────────────────

void ASTHasher::visit(const IdentifierExpr&)      { record("IdentifierExpr"); }
void ASTHasher::visit(const StringLiteralExpr&)    { record("StringLiteralExpr"); }
void ASTHasher::visit(const NumberExpr&)           { record("NumberExpr"); }
void ASTHasher::visit(const BoolLiteralExpr&)      { record("BoolLiteralExpr"); }
void ASTHasher::visit(const HoleExpr&)             { record("HoleExpr"); }

void ASTHasher::visit(const BinaryExpr& node) {
    record("BinaryExpr:" + node.op);
    node.left->accept(*this);
    node.right->accept(*this);
}

void ASTHasher::visit(const UnaryExpr& node) {
    record("UnaryExpr:" + node.op);
    node.operand->accept(*this);
}

void ASTHasher::visit(const JsonObjectExpr& node) {
    record("JsonObjectExpr");
    for (auto& f : node.fields) f.value->accept(*this);
}

void ASTHasher::visit(const ListExpr& node) {
    record("ListExpr");
    for (auto& e : node.elements) e->accept(*this);
}

void ASTHasher::visit(const GetExpr& node) {
    record("GetExpr");
    node.list->accept(*this);
    node.index->accept(*this);
}

void ASTHasher::visit(const CallExpr& node) {
    record("CallExpr:" + node.name);
    for (auto& a : node.args) a->accept(*this);
}

// ── Statements ──────────────────────────────────────────────────────────────

void ASTHasher::visit(const IntentDecl&)  { record("IntentDecl"); }
void ASTHasher::visit(const SkipDecl&)    { record("SkipDecl"); }

void ASTHasher::visit(const LetDecl& node) {
    record("LetDecl:" + node.name);
    node.value->accept(*this);
}

void ASTHasher::visit(const SetDecl& node) {
    record("SetDecl:" + node.name);
    node.value->accept(*this);
}

void ASTHasher::visit(const PrintDecl& node) {
    record("PrintDecl");
    node.expr->accept(*this);
}

void ASTHasher::visit(const ReadDecl& node) {
    record("ReadDecl:" + node.name);
}

void ASTHasher::visit(const RepeatDecl& node) {
    record("RepeatDecl");
    node.condition->accept(*this);
    for (auto& s : node.body) s->accept(*this);
}

void ASTHasher::visit(const RepeatTimesDecl& node) {
    record("RepeatTimesDecl");
    node.count->accept(*this);
    for (auto& s : node.body) s->accept(*this);
}

void ASTHasher::visit(const ForEachDecl& node) {
    record("ForEachDecl:" + node.varName);
    node.listExpr->accept(*this);
    for (auto& s : node.body) s->accept(*this);
}

void ASTHasher::visit(const IfDecl& node) {
    record("IfDecl");
    for (auto& clause : node.clauses) {
        clause.condition->accept(*this);
        for (auto& s : clause.body) s->accept(*this);
    }
    for (auto& s : node.else_body) s->accept(*this);
}

void ASTHasher::visit(const FunctionDecl& node) {
    record("FunctionDecl:" + node.name);
    for (auto& s : node.body) s->accept(*this);
}

void ASTHasher::visit(const ReturnStmt& node) {
    record("ReturnStmt");
    node.expr->accept(*this);
}

void ASTHasher::visit(const CallStmt& node) {
    record("CallStmt:" + node.name);
    for (auto& a : node.args) a->accept(*this);
}

void ASTHasher::visit(const PutDecl& node) {
    record("PutDecl:" + node.name);
    node.index->accept(*this);
    node.value->accept(*this);
}

// ── Pipe ────────────────────────────────────────────────────────────────────

void ASTHasher::visit(const StepNode& node) {
    record("StepNode");
    if (node.expr) node.expr->accept(*this);
}

void ASTHasher::visit(const PipeDecl& node) {
    record("PipeDecl:" + node.name);
    for (auto& s : node.steps) s->accept(*this);
}

// ── Match ───────────────────────────────────────────────────────────────────

void ASTHasher::visit(const MatchCaseNode& node) {
    record(node.is_catch_error ? "MatchCaseNode:catch" : "MatchCaseNode:when");
    if (node.condition) node.condition->accept(*this);
    node.yield_stmt->accept(*this);
}

void ASTHasher::visit(const MatchDecl& node) {
    record("MatchDecl");
    node.expr->accept(*this);
    for (auto& c : node.cases) c->accept(*this);
}

// ── Program ─────────────────────────────────────────────────────────────────

void ASTHasher::visit(const ProgramNode& node) {
    record("ProgramNode");
    for (auto& s : node.statements) s->accept(*this);
}
