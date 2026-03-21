#pragma once
#include "ast.h"
#include <iostream>
#include <string>

class ASTPrinter : public ASTVisitor {
public:
    void print(const ASTNode& node);

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
    int indent = 0;
    void printIndent() const;
};
