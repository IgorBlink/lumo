#include "static_analyzer.h"
#include <algorithm>

std::string lumoTypeToString(LumoType t) {
    switch (t) {
        case LumoType::NUMBER:  return "number";
        case LumoType::STRING:  return "string";
        case LumoType::BOOLEAN: return "boolean";
        case LumoType::LIST:    return "list";
        case LumoType::OBJECT:  return "object";
        case LumoType::ANY:     return "any";
        case LumoType::NEVER:   return "never";
    }
    return "unknown";
}

bool AnalysisResult::hasErrors() const {
    return std::any_of(errors.begin(), errors.end(),
        [](const AnalysisError& e) { return e.severity == AnalysisError::Severity::ERROR; });
}

// ── Scope management ────────────────────────────────────────────────────────

void StaticAnalyzer::pushScope() {
    scopes.emplace_back();
}

void StaticAnalyzer::popScope() {
    if (!scopes.empty()) scopes.pop_back();
}

void StaticAnalyzer::declare(const std::string& name, LumoType type, int line) {
    if (!scopes.empty()) {
        scopes.back()[name] = { type, line };
    }
    result.variables[name] = { type, line };
}

bool StaticAnalyzer::isDeclared(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) return true;
    }
    return false;
}

LumoType StaticAnalyzer::lookupType(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.type;
    }
    return LumoType::ANY;
}

void StaticAnalyzer::addError(int line, const std::string& msg) {
    result.errors.push_back({ AnalysisError::Severity::ERROR, line, msg });
}

void StaticAnalyzer::addWarning(int line, const std::string& msg) {
    result.errors.push_back({ AnalysisError::Severity::WARNING, line, msg });
}

LumoType StaticAnalyzer::inferExprType(const ExprNode& expr) {
    expr.accept(const_cast<StaticAnalyzer&>(*this));
    return lastExprType;
}

// ── Entry point ─────────────────────────────────────────────────────────────

AnalysisResult StaticAnalyzer::analyze(const ProgramNode& program) {
    result = AnalysisResult{};
    scopes.clear();
    pushScope(); // global scope

    // First pass: register all function declarations
    for (auto& stmt : program.statements) {
        if (auto* fn = dynamic_cast<const FunctionDecl*>(stmt.get())) {
            FuncSig sig;
            sig.name = fn->name;
            sig.paramCount = static_cast<int>(fn->params.size());
            sig.paramNames = fn->params;
            sig.declarationLine = fn->line;
            result.functions[fn->name] = sig;
        }
    }

    // Second pass: analyze all statements
    for (auto& stmt : program.statements) {
        stmt->accept(*this);
    }

    // Check for unused variables
    for (auto& [name, info] : result.variables) {
        if (!result.referencedVars.count(name) && !result.functions.count(name)) {
            addWarning(info.declarationLine, "Variable '" + name + "' is declared but never used");
        }
    }

    popScope();
    return result;
}

// ── Expressions ─────────────────────────────────────────────────────────────

void StaticAnalyzer::visit(const IdentifierExpr& node) {
    if (node.name != "value" && !isDeclared(node.name)) {
        addError(node.line, "Use of undeclared variable '" + node.name + "'");
    }
    result.referencedVars.insert(node.name);
    lastExprType = lookupType(node.name);
}

void StaticAnalyzer::visit(const StringLiteralExpr&)  { lastExprType = LumoType::STRING; }
void StaticAnalyzer::visit(const NumberExpr&)          { lastExprType = LumoType::NUMBER; }
void StaticAnalyzer::visit(const BoolLiteralExpr&)     { lastExprType = LumoType::BOOLEAN; }

void StaticAnalyzer::visit(const HoleExpr& node) {
    ++result.holeCount;
    addError(node.line, "Unresolved hole -- code is incomplete");
    lastExprType = LumoType::NEVER;
}

void StaticAnalyzer::visit(const BinaryExpr& node) {
    LumoType leftType = inferExprType(*node.left);
    LumoType rightType = inferExprType(*node.right);

    if (node.op == "plus") {
        // plus with a string operand → string concatenation
        if (leftType == LumoType::STRING || rightType == LumoType::STRING) {
            lastExprType = LumoType::STRING;
        } else {
            lastExprType = LumoType::NUMBER;
        }
    } else if (node.op == "minus" || node.op == "times" ||
               node.op == "divby" || node.op == "modulo") {
        lastExprType = LumoType::NUMBER;
        // Check division by literal zero
        if (node.op == "divby" || node.op == "modulo") {
            if (auto* num = dynamic_cast<const NumberExpr*>(node.right.get())) {
                double val = std::stod(num->value);
                if (val == 0.0) {
                    addError(node.line, "Division by zero: " + node.op + " with literal 0");
                }
            }
        }
    } else if (node.op == "equals" || node.op == "above" || node.op == "below" ||
               node.op == "atleast" || node.op == "atmost") {
        lastExprType = LumoType::BOOLEAN;
    } else if (node.op == "and" || node.op == "or") {
        lastExprType = LumoType::BOOLEAN;
    } else {
        lastExprType = LumoType::ANY;
    }
}

void StaticAnalyzer::visit(const UnaryExpr&) {
    lastExprType = LumoType::BOOLEAN;
}

void StaticAnalyzer::visit(const JsonObjectExpr& node) {
    for (auto& f : node.fields) f.value->accept(*this);
    lastExprType = LumoType::OBJECT;
}

void StaticAnalyzer::visit(const ListExpr& node) {
    for (auto& e : node.elements) e->accept(*this);
    lastExprType = LumoType::LIST;
}

void StaticAnalyzer::visit(const GetExpr& node) {
    node.list->accept(*this);
    node.index->accept(*this);
    // Check for negative literal index
    if (auto* num = dynamic_cast<const NumberExpr*>(node.index.get())) {
        double val = std::stod(num->value);
        if (val < 0) {
            addError(node.line, "Negative index in 'get' expression");
        }
    }
    lastExprType = LumoType::ANY; // element type unknown statically
}

void StaticAnalyzer::visit(const CallExpr& node) {
    // Check function exists and arity matches
    auto it = result.functions.find(node.name);
    if (it != result.functions.end()) {
        if (static_cast<int>(node.args.size()) != it->second.paramCount) {
            addError(node.line, "Function '" + node.name + "' expects " +
                     std::to_string(it->second.paramCount) + " arguments, got " +
                     std::to_string(node.args.size()));
        }
    }
    for (auto& a : node.args) a->accept(*this);
    lastExprType = LumoType::ANY; // return type unknown without deeper analysis
}

// ── Statements ──────────────────────────────────────────────────────────────

void StaticAnalyzer::visit(const IntentDecl&) {}
void StaticAnalyzer::visit(const SkipDecl&) {}

void StaticAnalyzer::visit(const LetDecl& node) {
    if (afterReturn) {
        addWarning(node.line, "Unreachable code after 'return'");
    }
    LumoType type = inferExprType(*node.value);
    declare(node.name, type, node.line);
}

void StaticAnalyzer::visit(const SetDecl& node) {
    if (afterReturn) {
        addWarning(node.line, "Unreachable code after 'return'");
    }
    if (!isDeclared(node.name)) {
        addError(node.line, "Cannot set undeclared variable '" + node.name + "'; use 'let' first");
    }
    result.mutatedVars.insert(node.name);
    result.mutationLines[node.name].push_back(node.line);
    node.value->accept(*this);
}

void StaticAnalyzer::visit(const PrintDecl& node) {
    if (afterReturn) {
        addWarning(node.line, "Unreachable code after 'return'");
    }
    node.expr->accept(*this);
}

void StaticAnalyzer::visit(const ReadDecl& node) {
    if (afterReturn) {
        addWarning(node.line, "Unreachable code after 'return'");
    }
    if (!isDeclared(node.name)) {
        addError(node.line, "Cannot read into undeclared variable '" + node.name + "'");
    }
    // After read, variable becomes string
    if (isDeclared(node.name)) {
        declare(node.name, LumoType::STRING, node.line);
    }
    result.mutatedVars.insert(node.name);
    result.mutationLines[node.name].push_back(node.line);
}

void StaticAnalyzer::visit(const RepeatDecl& node) {
    node.condition->accept(*this);
    pushScope();
    for (auto& s : node.body) s->accept(*this);
    popScope();
}

void StaticAnalyzer::visit(const RepeatTimesDecl& node) {
    node.count->accept(*this);
    pushScope();
    for (auto& s : node.body) s->accept(*this);
    popScope();
}

void StaticAnalyzer::visit(const ForEachDecl& node) {
    node.listExpr->accept(*this);
    pushScope();
    declare(node.varName, LumoType::ANY, node.line);
    for (auto& s : node.body) s->accept(*this);
    popScope();
}

void StaticAnalyzer::visit(const IfDecl& node) {
    for (auto& clause : node.clauses) {
        clause.condition->accept(*this);
        pushScope();
        for (auto& s : clause.body) s->accept(*this);
        popScope();
    }
    if (!node.else_body.empty()) {
        pushScope();
        for (auto& s : node.else_body) s->accept(*this);
        popScope();
    }
}

void StaticAnalyzer::visit(const FunctionDecl& node) {
    pushScope();
    bool prevInFunction = inFunctionBody;
    bool prevAfterReturn = afterReturn;
    inFunctionBody = true;
    afterReturn = false;

    for (auto& p : node.params) {
        declare(p, LumoType::ANY, node.line);
    }
    for (auto& s : node.body) {
        s->accept(*this);
    }

    afterReturn = prevAfterReturn;
    inFunctionBody = prevInFunction;
    popScope();
}

void StaticAnalyzer::visit(const ReturnStmt& node) {
    node.expr->accept(*this);
    afterReturn = true;
}

void StaticAnalyzer::visit(const CallStmt& node) {
    if (afterReturn) {
        addWarning(node.line, "Unreachable code after 'return'");
    }
    auto it = result.functions.find(node.name);
    if (it != result.functions.end()) {
        if (static_cast<int>(node.args.size()) != it->second.paramCount) {
            addError(node.line, "Function '" + node.name + "' expects " +
                     std::to_string(it->second.paramCount) + " arguments, got " +
                     std::to_string(node.args.size()));
        }
    }
    for (auto& a : node.args) a->accept(*this);
}

void StaticAnalyzer::visit(const PutDecl& node) {
    if (!isDeclared(node.name)) {
        addError(node.line, "Cannot put into undeclared variable '" + node.name + "'");
    }
    result.mutatedVars.insert(node.name);
    result.mutationLines[node.name].push_back(node.line);
    node.index->accept(*this);
    node.value->accept(*this);
}

// ── Pipe ────────────────────────────────────────────────────────────────────

void StaticAnalyzer::visit(const StepNode& node) {
    if (node.expr) node.expr->accept(*this);
}

void StaticAnalyzer::visit(const PipeDecl& node) {
    pushScope();
    declare(node.name, LumoType::LIST, node.line);
    for (auto& s : node.steps) s->accept(*this);
    popScope();
}

// ── Match ───────────────────────────────────────────────────────────────────

void StaticAnalyzer::visit(const MatchCaseNode& node) {
    if (node.condition) node.condition->accept(*this);
    node.yield_stmt->accept(*this);
}

void StaticAnalyzer::visit(const MatchDecl& node) {
    node.expr->accept(*this);
    for (auto& c : node.cases) c->accept(*this);
}

// ── Program ─────────────────────────────────────────────────────────────────

void StaticAnalyzer::visit(const ProgramNode& node) {
    for (auto& s : node.statements) s->accept(*this);
}
