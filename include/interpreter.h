#pragma once
#include "ast.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <stdexcept>

using LumoValue = std::variant<double, std::string, bool>;

std::string lumoValueToString(const LumoValue& v);

class HaltException : public std::exception {
public:
    std::string message;
    explicit HaltException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class Interpreter : public ASTVisitor {
public:
    void execute(const ProgramNode& program);

    void visit(const IdentifierExpr& node) override;
    void visit(const StringLiteralExpr& node) override;
    void visit(const NumberExpr& node) override;
    void visit(const BoolLiteralExpr& node) override;
    void visit(const HoleExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const JsonObjectExpr& node) override;

    void visit(const IntentDecl& node) override;
    void visit(const LetDecl& node) override;
    void visit(const SetDecl& node) override;
    void visit(const PrintDecl& node) override;
    void visit(const ExprStatement& node) override;
    void visit(const RepeatDecl& node) override;

    void visit(const StepNode& node) override;
    void visit(const PipeDecl& node) override;

    void visit(const MatchCaseNode& node) override;
    void visit(const MatchDecl& node) override;

    void visit(const ProgramNode& node) override;

private:
    std::unordered_map<std::string, LumoValue> env;
    LumoValue lastValue;
    LumoValue pipeValue; // register for active pipe

    LumoValue eval(const ExprNode& expr);
    void execStmt(const StmtNode& stmt);

    bool isTruthy(const LumoValue& v) const;
    bool valuesEqual(const LumoValue& a, const LumoValue& b) const;
};
