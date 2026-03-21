#include "interpreter.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <stdexcept>

// ─── Helpers ──────────────────────────────────────────────────────────────────

std::string lumoValueToString(const LumoValue& v) {
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == std::floor(d) && std::abs(d) < 1e15) {
            std::ostringstream oss;
            oss << static_cast<long long>(d);
            return oss.str();
        }
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }
    if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    }
    if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? "true" : "false";
    }
    if (std::holds_alternative<std::shared_ptr<LumoList>>(v)) {
        auto lst = std::get<std::shared_ptr<LumoList>>(v);
        std::string s = "[";
        for (size_t i = 0; i < lst->elements.size(); ++i) {
            s += lumoValueToString(lst->elements[i]);
            if (i + 1 < lst->elements.size()) s += ", ";
        }
        s += "]";
        return s;
    }
    // Fix 4: serialize LumoObject as JSON
    if (std::holds_alternative<std::shared_ptr<LumoObject>>(v)) {
        auto obj = std::get<std::shared_ptr<LumoObject>>(v);
        std::string s = "{";
        bool first = true;
        for (const auto& kv : obj->fields) {
            if (!first) s += ", ";
            s += "\"" + kv.first + "\": " + lumoValueToString(kv.second);
            first = false;
        }
        s += "}";
        return s;
    }
    return "";
}

bool Interpreter::isTruthy(const LumoValue& v) const {
    if (std::holds_alternative<bool>(v))   return std::get<bool>(v);
    if (std::holds_alternative<double>(v)) return std::get<double>(v) != 0.0;
    if (std::holds_alternative<std::string>(v)) return !std::get<std::string>(v).empty();
    if (std::holds_alternative<std::shared_ptr<LumoList>>(v))
        return !std::get<std::shared_ptr<LumoList>>(v)->elements.empty();
    if (std::holds_alternative<std::shared_ptr<LumoObject>>(v))
        return !std::get<std::shared_ptr<LumoObject>>(v)->fields.empty();
    return false;
}

bool Interpreter::valuesEqual(const LumoValue& a, const LumoValue& b) const {
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
        return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b))
        return std::get<std::string>(a) == std::get<std::string>(b);
    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
        return std::get<bool>(a) == std::get<bool>(b);
    if (std::holds_alternative<std::shared_ptr<LumoList>>(a) &&
        std::holds_alternative<std::shared_ptr<LumoList>>(b)) {
        auto la = std::get<std::shared_ptr<LumoList>>(a);
        auto lb = std::get<std::shared_ptr<LumoList>>(b);
        if (la->elements.size() != lb->elements.size()) return false;
        for (size_t i = 0; i < la->elements.size(); ++i) {
            if (!valuesEqual(la->elements[i], lb->elements[i])) return false;
        }
        return true;
    }
    return false;
}

LumoValue Interpreter::eval(const ExprNode& expr) {
    expr.accept(*this);
    return lastValue;
}

void Interpreter::execStmt(const StmtNode& stmt) {
    stmt.accept(*this);
}

// Fix 5: Functions use isolated local scope.
// When called, a new environment is created containing only the declared parameters.
// Functions can read top-level (program-scope) variables but cannot mutate them.
// Local `set` statements affect only the local scope. The only way a function
// communicates a result back to the caller is via `return`.
//
// Implementation: copy-on-call. The function body executes with a fresh env
// containing only parameters, plus the captured global-scope snapshot for reads.
// After the call, the caller's env is fully restored, so no mutations leak out.
LumoValue Interpreter::callFunction(const std::string& name,
                                     const std::vector<std::unique_ptr<ExprNode>>& argExprs) {
    auto it = functions.find(name);
    if (it == functions.end()) {
        throw std::runtime_error("Undefined function: " + name);
    }
    const FunctionDecl* func = it->second;

    if (argExprs.size() != func->params.size()) {
        throw std::runtime_error(
            "Function '" + name + "' expects " + std::to_string(func->params.size()) +
            " argument(s), got " + std::to_string(argExprs.size()));
    }

    // Evaluate arguments in the CALLER's env before switching scope
    std::vector<LumoValue> argVals;
    argVals.reserve(argExprs.size());
    for (const auto& arg : argExprs) {
        argVals.push_back(eval(*arg));
    }

    // Capture the global scope (top-level program env) for read fallback
    auto savedEnv      = env;
    auto savedGlobalEnv = globalEnv;
    bool savedInFunction = inFunction;

    // The global snapshot for reads inside this function is:
    //   - if we're at top level: the current env
    //   - if we're already inside a function: the already-captured globalEnv
    if (!inFunction) {
        globalEnv = env;
    }
    // (if inFunction, globalEnv already holds the top-level snapshot)

    // Local env contains only declared parameters
    env.clear();
    for (size_t i = 0; i < func->params.size(); ++i) {
        env[func->params[i]] = argVals[i];
    }
    inFunction = true;

    LumoValue returnValue = false;
    try {
        for (const auto& stmt : func->body) {
            execStmt(*stmt);
        }
    } catch (const ReturnException& ret) {
        returnValue = ret.value;
    }

    env        = savedEnv;
    globalEnv  = savedGlobalEnv;
    inFunction = savedInFunction;
    return returnValue;
}

// ─── Entry point ──────────────────────────────────────────────────────────────

void Interpreter::execute(const ProgramNode& program) {
    program.accept(*this);
}

// ─── Expressions ─────────────────────────────────────────────────────────────

void Interpreter::visit(const NumberExpr& node) {
    lastValue = std::stod(node.value);
}

void Interpreter::visit(const StringLiteralExpr& node) {
    lastValue = node.value;
}

void Interpreter::visit(const BoolLiteralExpr& node) {
    lastValue = node.value;
}

void Interpreter::visit(const IdentifierExpr& node) {
    // Fix 7: `value` is only valid inside a pipe map/filter/transform step
    if (node.name == "value") {
        if (!inPipeStep) {
            throw std::runtime_error(
                "'value' is only valid inside a pipe map, filter, or transform step");
        }
        lastValue = pipeValue;
        return;
    }

    // Look in local env first
    auto it = env.find(node.name);
    if (it != env.end()) {
        lastValue = it->second;
        return;
    }

    // Fix 5: fall back to global env when inside a function
    if (inFunction) {
        auto git = globalEnv.find(node.name);
        if (git != globalEnv.end()) {
            lastValue = git->second;
            return;
        }
    }

    throw std::runtime_error("Undefined variable: " + node.name);
}

void Interpreter::visit(const HoleExpr& node) {
    std::cerr << "[HOLE] Unresolved hole at line " << node.line
              << ": \"" << node.instruction << "\"" << std::endl;
    throw HaltException("Unresolved hole");
}

void Interpreter::visit(const BinaryExpr& node) {
    LumoValue left = eval(*node.left);

    // Short-circuit for `and` / `or`
    if (node.op == "and") {
        if (!isTruthy(left)) { lastValue = false; return; }
        lastValue = eval(*node.right);
        return;
    }
    if (node.op == "or") {
        if (isTruthy(left)) { lastValue = left; return; }
        lastValue = eval(*node.right);
        return;
    }

    LumoValue right = eval(*node.right);

    if (node.op == "equals") {
        lastValue = valuesEqual(left, right);
        return;
    }
    if (node.op == "above") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            lastValue = std::get<double>(left) > std::get<double>(right);
        } else {
            lastValue = false;
        }
        return;
    }
    if (node.op == "below") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            lastValue = std::get<double>(left) < std::get<double>(right);
        } else {
            lastValue = false;
        }
        return;
    }
    // Fix 1: atleast (>=) and atmost (<=)
    if (node.op == "atleast") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            lastValue = std::get<double>(left) >= std::get<double>(right);
        } else {
            lastValue = false;
        }
        return;
    }
    if (node.op == "atmost") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            lastValue = std::get<double>(left) <= std::get<double>(right);
        } else {
            lastValue = false;
        }
        return;
    }

    // `plus` handles string concatenation
    if (node.op == "plus") {
        if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
            lastValue = lumoValueToString(left) + lumoValueToString(right);
            return;
        }
    }

    // All remaining operators require numeric operands
    auto requireDouble = [&](const LumoValue& v, const std::string& side) -> double {
        if (std::holds_alternative<double>(v)) return std::get<double>(v);
        throw std::runtime_error("Arithmetic operator '" + node.op +
                                 "' requires a number on the " + side + " side, got: " +
                                 lumoValueToString(v));
    };

    double l = requireDouble(left, "left");
    double r = requireDouble(right, "right");

    if (node.op == "plus")   { lastValue = l + r; return; }
    if (node.op == "minus")  { lastValue = l - r; return; }
    if (node.op == "times")  { lastValue = l * r; return; }
    if (node.op == "divby") {
        if (r == 0.0) throw std::runtime_error("Division by zero");
        lastValue = l / r;
        return;
    }
    if (node.op == "modulo") {
        if (r == 0.0) throw std::runtime_error("Modulo by zero");
        lastValue = std::fmod(l, r);
        return;
    }

    throw std::runtime_error("Unknown binary operator: " + node.op);
}

void Interpreter::visit(const UnaryExpr& node) {
    LumoValue operand = eval(*node.operand);
    if (node.op == "not") {
        lastValue = !isTruthy(operand);
        return;
    }
    throw std::runtime_error("Unknown unary operator: " + node.op);
}

// Fix 4: produce a LumoObject instead of a serialized string
void Interpreter::visit(const JsonObjectExpr& node) {
    auto obj = std::make_shared<LumoObject>();
    for (const auto& field : node.fields) {
        obj->fields[field.key] = eval(*field.value);
    }
    lastValue = obj;
}

void Interpreter::visit(const ListExpr& node) {
    auto lst = std::make_shared<LumoList>();
    for (const auto& elem : node.elements) {
        lst->elements.push_back(eval(*elem));
    }
    lastValue = lst;
}

// Fix 4: dispatch on container type (list → numeric index, object → string key)
void Interpreter::visit(const GetExpr& node) {
    LumoValue containerVal = eval(*node.list);
    LumoValue keyVal = eval(*node.index);

    if (std::holds_alternative<std::shared_ptr<LumoObject>>(containerVal)) {
        auto obj = std::get<std::shared_ptr<LumoObject>>(containerVal);
        if (!std::holds_alternative<std::string>(keyVal)) {
            throw std::runtime_error("JSON object key must be a string");
        }
        const std::string& key = std::get<std::string>(keyVal);
        auto it = obj->fields.find(key);
        if (it == obj->fields.end()) {
            throw std::runtime_error("Key not found in object: \"" + key + "\"");
        }
        lastValue = it->second;
        return;
    }

    if (std::holds_alternative<std::shared_ptr<LumoList>>(containerVal)) {
        auto lst = std::get<std::shared_ptr<LumoList>>(containerVal);
        if (!std::holds_alternative<double>(keyVal)) {
            throw std::runtime_error("List index must be a number");
        }
        size_t idx = static_cast<size_t>(std::get<double>(keyVal));
        if (idx >= lst->elements.size()) {
            throw std::runtime_error("List index out of bounds: " + std::to_string(idx));
        }
        lastValue = lst->elements[idx];
        return;
    }

    throw std::runtime_error("'get' requires a list or JSON object");
}

void Interpreter::visit(const CallExpr& node) {
    lastValue = callFunction(node.name, node.args);
}

// ─── Statements ──────────────────────────────────────────────────────────────

void Interpreter::visit(const IntentDecl& node) {
    std::cerr << "[INTENT] " << node.intent_string << std::endl;
}

void Interpreter::visit(const LetDecl& node) {
    env[node.name] = eval(*node.value);
}

void Interpreter::visit(const SetDecl& node) {
    // Fix 5: inside a function, allow shadowing a global by creating a local binding
    if (env.find(node.name) == env.end()) {
        if (inFunction && globalEnv.count(node.name)) {
            env[node.name] = eval(*node.value);
            return;
        }
        throw std::runtime_error("Cannot set undefined variable: " + node.name +
                                 ". Use 'let' to declare it first.");
    }
    env[node.name] = eval(*node.value);
}

void Interpreter::visit(const PrintDecl& node) {
    std::cout << lumoValueToString(eval(*node.expr)) << std::endl;
}

void Interpreter::visit(const SkipDecl& node) {
    // explicit no-op
}

// Fix 6: read one line from stdin into an existing variable
void Interpreter::visit(const ReadDecl& node) {
    if (env.find(node.name) == env.end()) {
        if (!(inFunction && globalEnv.count(node.name))) {
            throw std::runtime_error("Cannot read into undefined variable: " + node.name +
                                     ". Use 'let' to declare it first.");
        }
    }
    std::string line;
    if (std::getline(std::cin, line)) {
        env[node.name] = line;
    } else {
        env[node.name] = std::string("");
    }
}

void Interpreter::visit(const RepeatDecl& node) {
    while (true) {
        if (!isTruthy(eval(*node.condition))) break;
        for (const auto& stmt : node.body) {
            execStmt(*stmt);
        }
    }
}

// Fix 2: repeat <count> times
void Interpreter::visit(const RepeatTimesDecl& node) {
    LumoValue countVal = eval(*node.count);
    if (!std::holds_alternative<double>(countVal)) {
        throw std::runtime_error("'repeat ... times' count must be a number");
    }
    double d = std::get<double>(countVal);
    if (d != std::floor(d) || d < 0.0) {
        throw std::runtime_error("'repeat ... times' count must be a non-negative integer");
    }
    long long count = static_cast<long long>(d);
    for (long long i = 0; i < count; ++i) {
        for (const auto& stmt : node.body) {
            execStmt(*stmt);
        }
    }
}

// Fix 2: for each <name> in <list-expr>
void Interpreter::visit(const ForEachDecl& node) {
    LumoValue listVal = eval(*node.listExpr);
    if (!std::holds_alternative<std::shared_ptr<LumoList>>(listVal)) {
        throw std::runtime_error("'for each' requires a list value");
    }
    auto lst = std::get<std::shared_ptr<LumoList>>(listVal);

    for (const auto& elem : lst->elements) {
        env[node.varName] = elem;
        for (const auto& stmt : node.body) {
            execStmt(*stmt);
        }
    }
    // Remove the loop variable from env after the loop ends
    env.erase(node.varName);
}

void Interpreter::visit(const IfDecl& node) {
    for (const auto& clause : node.clauses) {
        if (isTruthy(eval(*clause.condition))) {
            for (const auto& stmt : clause.body) {
                execStmt(*stmt);
            }
            return;
        }
    }
    for (const auto& stmt : node.else_body) {
        execStmt(*stmt);
    }
}

void Interpreter::visit(const FunctionDecl& node) {
    functions[node.name] = &node;
}

void Interpreter::visit(const ReturnStmt& node) {
    throw ReturnException(eval(*node.expr));
}

void Interpreter::visit(const CallStmt& node) {
    callFunction(node.name, node.args);
}

// Fix 4: dispatch on container type (list → numeric index, object → string key)
void Interpreter::visit(const PutDecl& node) {
    auto it = env.find(node.name);
    if (it == env.end()) {
        throw std::runtime_error("Undefined variable: " + node.name);
    }

    LumoValue keyVal = eval(*node.index);
    LumoValue val    = eval(*node.value);

    if (std::holds_alternative<std::shared_ptr<LumoObject>>(it->second)) {
        auto obj = std::get<std::shared_ptr<LumoObject>>(it->second);
        if (!std::holds_alternative<std::string>(keyVal)) {
            throw std::runtime_error("JSON object key must be a string");
        }
        obj->fields[std::get<std::string>(keyVal)] = val;
        return;
    }

    if (std::holds_alternative<std::shared_ptr<LumoList>>(it->second)) {
        auto lst = std::get<std::shared_ptr<LumoList>>(it->second);
        if (!std::holds_alternative<double>(keyVal)) {
            throw std::runtime_error("List index must be a number");
        }
        size_t idx = static_cast<size_t>(std::get<double>(keyVal));
        if (idx >= lst->elements.size()) {
            throw std::runtime_error("List index out of bounds: " + std::to_string(idx));
        }
        lst->elements[idx] = val;
        return;
    }

    throw std::runtime_error("'put' requires a list or JSON object variable, but '" +
                             node.name + "' is neither");
}

// ─── Pipe ─────────────────────────────────────────────────────────────────────

void Interpreter::visit(const PipeDecl& node) {
    for (const auto& step : node.steps) {
        step->accept(*this);
    }
}

void Interpreter::visit(const StepNode& node) {
    switch (node.op) {
        case StepOpType::START_WITH:
            if (node.expr) pipeValue = eval(*node.expr);
            break;

        case StepOpType::TRANSFORM:
            // Fix 7: set inPipeStep so `value` is accessible
            if (node.expr) {
                inPipeStep = true;
                pipeValue = eval(*node.expr);
                inPipeStep = false;
            }
            break;

        case StepOpType::MAP:
            if (node.expr && std::holds_alternative<std::shared_ptr<LumoList>>(pipeValue)) {
                auto src = std::get<std::shared_ptr<LumoList>>(pipeValue);
                auto dst = std::make_shared<LumoList>();
                for (const auto& elem : src->elements) {
                    pipeValue = elem;
                    inPipeStep = true;          // Fix 7
                    dst->elements.push_back(eval(*node.expr));
                    inPipeStep = false;
                }
                pipeValue = dst;
            }
            break;

        case StepOpType::FILTER:
            if (node.expr && std::holds_alternative<std::shared_ptr<LumoList>>(pipeValue)) {
                auto src = std::get<std::shared_ptr<LumoList>>(pipeValue);
                auto dst = std::make_shared<LumoList>();
                for (const auto& elem : src->elements) {
                    pipeValue = elem;
                    inPipeStep = true;          // Fix 7
                    bool keep = isTruthy(eval(*node.expr));
                    inPipeStep = false;
                    if (keep) dst->elements.push_back(elem);
                }
                pipeValue = dst;
            }
            break;

        // Fix 3: yield <identifier> — store pipeValue into the named variable
        case StepOpType::YIELD:
            if (node.expr) {
                auto* ident = dynamic_cast<const IdentifierExpr*>(node.expr.get());
                if (ident) {
                    env[ident->name] = pipeValue;
                }
            }
            break;
    }
}

// ─── Match ────────────────────────────────────────────────────────────────────

void Interpreter::visit(const MatchDecl& node) {
    LumoValue matchVal = eval(*node.expr);

    for (const auto& c : node.cases) {
        if (c->is_catch_error) {
            execStmt(*c->yield_stmt);
            return;
        }
        if (valuesEqual(matchVal, eval(*c->condition))) {
            execStmt(*c->yield_stmt);
            return;
        }
    }
}

void Interpreter::visit(const MatchCaseNode& node) {
    // Driven by MatchDecl; never called directly.
}

// ─── Program ─────────────────────────────────────────────────────────────────

void Interpreter::visit(const ProgramNode& node) {
    for (const auto& stmt : node.statements) {
        execStmt(*stmt);
    }
}
