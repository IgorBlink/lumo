#pragma once
#include "ast.h"
#include <iostream>
#include <string>

class ASTPrinter : public ASTVisitor {
public:
    void print(const ASTNode& node);

    // Expressions
    void visit(const IdentifierExpr& node) override;
    void visit(const StringLiteralExpr& node) override;
    void visit(const NumberExpr& node) override;
    void visit(const BoolLiteralExpr& node) override;
    void visit(const HoleExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const JsonObjectExpr& node) override;
    void visit(const ListExpr& node) override;
    void visit(const GetExpr& node) override;
    void visit(const CallExpr& node) override;

    // Statements
    void visit(const IntentDecl& node) override;
    void visit(const LetDecl& node) override;
    void visit(const SetDecl& node) override;
    void visit(const PrintDecl& node) override;
    void visit(const SkipDecl& node) override;
    void visit(const ReadDecl& node) override;
    void visit(const RepeatDecl& node) override;
    void visit(const RepeatTimesDecl& node) override;
    void visit(const ForEachDecl& node) override;
    void visit(const IfDecl& node) override;
    void visit(const FunctionDecl& node) override;
    void visit(const ReturnStmt& node) override;
    void visit(const CallStmt& node) override;
    void visit(const PutDecl& node) override;

    // Pipe
    void visit(const StepNode& node) override;
    void visit(const PipeDecl& node) override;

    // Match
    void visit(const MatchCaseNode& node) override;
    void visit(const MatchDecl& node) override;

    // Program
    void visit(const ProgramNode& node) override;

private:
    int indent = 0;
    void printIndent() const;
};
