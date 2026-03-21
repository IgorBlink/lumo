#include "ast_printer.h"
#include <iostream>

void ASTPrinter::printIndent() const {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
}

void ASTPrinter::print(const ASTNode& node) {
    node.accept(*this);
}

// ─── Expressions ─────────────────────────────────────────────────────────────

void ASTPrinter::visit(const IdentifierExpr& node) {
    std::cout << "Identifier(" << node.name << ")";
}

void ASTPrinter::visit(const StringLiteralExpr& node) {
    std::cout << "String(\"" << node.value << "\")";
}

void ASTPrinter::visit(const NumberExpr& node) {
    std::cout << "Number(" << node.value << ")";
}

void ASTPrinter::visit(const BoolLiteralExpr& node) {
    std::cout << "Bool(" << (node.value ? "true" : "false") << ")";
}

void ASTPrinter::visit(const HoleExpr& node) {
    std::cout << "Hole(\"" << node.instruction << "\")";
}

void ASTPrinter::visit(const BinaryExpr& node) {
    std::cout << "Binary(" << node.op << ", ";
    node.left->accept(*this);
    std::cout << ", ";
    node.right->accept(*this);
    std::cout << ")";
}

void ASTPrinter::visit(const UnaryExpr& node) {
    std::cout << "Unary(" << node.op << ", ";
    node.operand->accept(*this);
    std::cout << ")";
}

void ASTPrinter::visit(const JsonObjectExpr& node) {
    std::cout << "JsonObject({";
    for (size_t i = 0; i < node.fields.size(); ++i) {
        std::cout << "\"" << node.fields[i].key << "\": ";
        node.fields[i].value->accept(*this);
        if (i + 1 < node.fields.size()) std::cout << ", ";
    }
    std::cout << "})";
}

// ─── Statements ──────────────────────────────────────────────────────────────

void ASTPrinter::visit(const IntentDecl& node) {
    printIndent();
    std::cout << "Intent(\"" << node.intent_string << "\")\n";
}

void ASTPrinter::visit(const LetDecl& node) {
    printIndent();
    std::cout << "Let(" << node.name << " = ";
    node.value->accept(*this);
    std::cout << ")\n";
}

void ASTPrinter::visit(const SetDecl& node) {
    printIndent();
    std::cout << "Set(" << node.name << " = ";
    node.value->accept(*this);
    std::cout << ")\n";
}

void ASTPrinter::visit(const PrintDecl& node) {
    printIndent();
    std::cout << "Print(";
    node.expr->accept(*this);
    std::cout << ")\n";
}

void ASTPrinter::visit(const ExprStatement& node) {
    printIndent();
    std::cout << "ExprStatement(";
    node.expr->accept(*this);
    std::cout << ")\n";
}

void ASTPrinter::visit(const RepeatDecl& node) {
    printIndent();
    std::cout << "Repeat(while ";
    node.condition->accept(*this);
    std::cout << ") {\n";
    indent++;
    for (const auto& stmt : node.body) {
        stmt->accept(*this);
    }
    indent--;
    printIndent();
    std::cout << "}\n";
}

// ─── Pipe ────────────────────────────────────────────────────────────────────

void ASTPrinter::visit(const StepNode& node) {
    printIndent();
    std::cout << "Step(";
    switch (node.op) {
        case StepOpType::START_WITH: std::cout << "start with"; break;
        case StepOpType::MAP:        std::cout << "map";        break;
        case StepOpType::FILTER:     std::cout << "filter";     break;
        case StepOpType::TRANSFORM:  std::cout << "transform";  break;
        case StepOpType::YIELD:      std::cout << "yield";      break;
    }
    if (node.expr) {
        std::cout << ", ";
        node.expr->accept(*this);
    }
    if (node.context) {
        std::cout << ", using=[";
        for (size_t i = 0; i < node.context->variables.size(); ++i) {
            std::cout << node.context->variables[i];
            if (i + 1 < node.context->variables.size()) std::cout << ", ";
        }
        std::cout << "]";
    }
    std::cout << ")\n";
}

void ASTPrinter::visit(const PipeDecl& node) {
    printIndent();
    std::cout << "Pipe(" << node.name << ") {\n";
    indent++;
    for (const auto& step : node.steps) step->accept(*this);
    indent--;
    printIndent();
    std::cout << "}\n";
}

// ─── Match ───────────────────────────────────────────────────────────────────

void ASTPrinter::visit(const MatchCaseNode& node) {
    printIndent();
    if (node.is_catch_error) {
        std::cout << "Case(catch error, yield=\n";
    } else {
        std::cout << "Case(when=";
        node.condition->accept(*this);
        std::cout << ", yield=\n";
    }
    indent++;
    node.yield_stmt->accept(*this);
    indent--;
    printIndent();
    std::cout << ")\n";
}

void ASTPrinter::visit(const MatchDecl& node) {
    printIndent();
    std::cout << "Match(";
    node.expr->accept(*this);
    std::cout << ") {\n";
    indent++;
    for (const auto& c : node.cases) c->accept(*this);
    indent--;
    printIndent();
    std::cout << "}\n";
}

// ─── Program ─────────────────────────────────────────────────────────────────

void ASTPrinter::visit(const ProgramNode& node) {
    std::cout << "Program {\n";
    indent++;
    for (const auto& stmt : node.statements) stmt->accept(*this);
    indent--;
    std::cout << "}\n";
}
