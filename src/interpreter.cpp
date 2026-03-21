#include "interpreter.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <stdexcept>

// ─── Helpers ──────────────────────────────────────────────────────────────────

std::string lumoValueToString(const LumoValue& v) {
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        // Print as integer if it is a whole number
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
    return "";
}

bool Interpreter::isTruthy(const LumoValue& v) const {
    if (std::holds_alternative<bool>(v))   return std::get<bool>(v);
    if (std::holds_alternative<double>(v)) return std::get<double>(v) != 0.0;
    if (std::holds_alternative<std::string>(v)) return !std::get<std::string>(v).empty();
    return false;
}

bool Interpreter::valuesEqual(const LumoValue& a, const LumoValue& b) const {
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
        return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b))
        return std::get<std::string>(a) == std::get<std::string>(b);
    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b))
        return std::get<bool>(a) == std::get<bool>(b);
    return false;
}

LumoValue Interpreter::eval(const ExprNode& expr) {
    expr.accept(*this);
    return lastValue;
}

void Interpreter::execStmt(const StmtNode& stmt) {
    stmt.accept(*this);
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
    // `value` refers to the pipe register; `result` is looked up normally
    if (node.name == "value") {
        lastValue = pipeValue;
        return;
    }
    auto it = env.find(node.name);
    if (it == env.end()) {
        throw std::runtime_error("Undefined variable: " + node.name);
    }
    lastValue = it->second;
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
    if (node.op == "greater than") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            lastValue = std::get<double>(left) > std::get<double>(right);
        } else {
            lastValue = lumoValueToString(left) > lumoValueToString(right);
        }
        return;
    }
    if (node.op == "less than") {
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
            lastValue = std::get<double>(left) < std::get<double>(right);
        } else {
            lastValue = lumoValueToString(left) < lumoValueToString(right);
        }
        return;
    }

    // Arithmetic operators require numeric operands
    auto requireDouble = [&](const LumoValue& v, const std::string& side) -> double {
        if (std::holds_alternative<double>(v)) return std::get<double>(v);
        throw std::runtime_error("Arithmetic operator '" + node.op +
                                 "' requires a number on the " + side + " side, got: " +
                                 lumoValueToString(v));
    };

    double l = requireDouble(left, "left");
    double r = requireDouble(right, "right");

    if (node.op == "plus")       { lastValue = l + r; return; }
    if (node.op == "minus")      { lastValue = l - r; return; }
    if (node.op == "times")      { lastValue = l * r; return; }
    if (node.op == "divided by") {
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

void Interpreter::visit(const JsonObjectExpr& node) {
    // Represent as a simple string for now
    std::string result = "{";
    for (size_t i = 0; i < node.fields.size(); ++i) {
        result += "\"" + node.fields[i].key + "\": ";
        result += lumoValueToString(eval(*node.fields[i].value));
        if (i + 1 < node.fields.size()) result += ", ";
    }
    result += "}";
    lastValue = result;
}

// ─── Statements ──────────────────────────────────────────────────────────────

void Interpreter::visit(const IntentDecl& node) {
    std::cerr << "[INTENT] " << node.intent_string << std::endl;
}

void Interpreter::visit(const LetDecl& node) {
    env[node.name] = eval(*node.value);
}

void Interpreter::visit(const SetDecl& node) {
    if (env.find(node.name) == env.end()) {
        throw std::runtime_error("Cannot set undefined variable: " + node.name +
                                 ". Use 'let' to declare it first.");
    }
    env[node.name] = eval(*node.value);
}

void Interpreter::visit(const PrintDecl& node) {
    LumoValue val = eval(*node.expr);
    std::cout << lumoValueToString(val) << std::endl;
}

void Interpreter::visit(const ExprStatement& node) {
    eval(*node.expr); // evaluate and discard the result
}

void Interpreter::visit(const RepeatDecl& node) {
    while (true) {
        LumoValue cond = eval(*node.condition);
        if (!isTruthy(cond)) break;
        for (const auto& stmt : node.body) {
            execStmt(*stmt);
        }
    }
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
            if (node.expr) {
                // `value` identifier in the expression resolves to pipeValue
                pipeValue = eval(*node.expr);
            }
            break;

        case StepOpType::MAP:
            // Minimal: map without a function just keeps pipeValue
            break;

        case StepOpType::FILTER:
            // Minimal: filter without runtime predicate keeps pipeValue
            break;

        case StepOpType::YIELD:
            // Store pipeValue into env as `result` (or the named identifier)
            if (node.expr) {
                auto* ident = dynamic_cast<const IdentifierExpr*>(node.expr.get());
                if (ident) {
                    env[ident->name] = pipeValue;
                    break;
                }
            }
            env["result"] = pipeValue;
            break;
    }
}

// ─── Match ────────────────────────────────────────────────────────────────────

void Interpreter::visit(const MatchDecl& node) {
    LumoValue matchVal = eval(*node.expr);

    for (const auto& c : node.cases) {
        if (c->is_catch_error) {
            // Fallthrough — execute catch case if nothing matched
            execStmt(*c->yield_stmt);
            return;
        }
        LumoValue whenVal = eval(*c->condition);
        if (valuesEqual(matchVal, whenVal)) {
            execStmt(*c->yield_stmt);
            return;
        }
    }
    // No match and no catch — silently do nothing
}

void Interpreter::visit(const MatchCaseNode& node) {
    // MatchCaseNode is executed directly only via MatchDecl; this is a no-op if called standalone.
}

// ─── Program ─────────────────────────────────────────────────────────────────

void Interpreter::visit(const ProgramNode& node) {
    for (const auto& stmt : node.statements) {
        execStmt(*stmt);
    }
}
