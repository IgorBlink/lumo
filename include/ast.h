#pragma once

#include <string>
#include <vector>
#include <memory>

class ASTVisitor;

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) const = 0;
};

// ─── Expressions ─────────────────────────────────────────────────────────────

class ExprNode : public ASTNode {};

class IdentifierExpr : public ExprNode {
public:
    std::string name;
    IdentifierExpr(const std::string& name) : name(name) {}
    void accept(ASTVisitor& visitor) const override;
};

class StringLiteralExpr : public ExprNode {
public:
    std::string value;
    StringLiteralExpr(const std::string& value) : value(value) {}
    void accept(ASTVisitor& visitor) const override;
};

class NumberExpr : public ExprNode {
public:
    std::string value;
    NumberExpr(const std::string& value) : value(value) {}
    void accept(ASTVisitor& visitor) const override;
};

class BoolLiteralExpr : public ExprNode {
public:
    bool value;
    BoolLiteralExpr(bool value) : value(value) {}
    void accept(ASTVisitor& visitor) const override;
};

class HoleExpr : public ExprNode {
public:
    std::string instruction;
    int line;
    HoleExpr(const std::string& instruction, int line = 0)
        : instruction(instruction), line(line) {}
    void accept(ASTVisitor& visitor) const override;
};

class BinaryExpr : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> left;
    std::unique_ptr<ExprNode> right;
    BinaryExpr(const std::string& op,
               std::unique_ptr<ExprNode> left,
               std::unique_ptr<ExprNode> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    void accept(ASTVisitor& visitor) const override;
};

class UnaryExpr : public ExprNode {
public:
    std::string op;
    std::unique_ptr<ExprNode> operand;
    UnaryExpr(const std::string& op, std::unique_ptr<ExprNode> operand)
        : op(op), operand(std::move(operand)) {}
    void accept(ASTVisitor& visitor) const override;
};

class JsonField {
public:
    std::string key;
    std::unique_ptr<ExprNode> value;
    JsonField(const std::string& key, std::unique_ptr<ExprNode> value)
        : key(key), value(std::move(value)) {}
};

class JsonObjectExpr : public ExprNode {
public:
    std::vector<JsonField> fields;
    void accept(ASTVisitor& visitor) const override;
};

// ─── Statements ──────────────────────────────────────────────────────────────

class StmtNode : public ASTNode {};

class IntentDecl : public StmtNode {
public:
    std::string intent_string;
    IntentDecl(const std::string& intent_string) : intent_string(intent_string) {}
    void accept(ASTVisitor& visitor) const override;
};

class LetDecl : public StmtNode {
public:
    std::string name;
    std::unique_ptr<ExprNode> value;
    LetDecl(const std::string& name, std::unique_ptr<ExprNode> value)
        : name(name), value(std::move(value)) {}
    void accept(ASTVisitor& visitor) const override;
};

class SetDecl : public StmtNode {
public:
    std::string name;
    std::unique_ptr<ExprNode> value;
    SetDecl(const std::string& name, std::unique_ptr<ExprNode> value)
        : name(name), value(std::move(value)) {}
    void accept(ASTVisitor& visitor) const override;
};

class PrintDecl : public StmtNode {
public:
    std::unique_ptr<ExprNode> expr;
    PrintDecl(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}
    void accept(ASTVisitor& visitor) const override;
};

class ExprStatement : public StmtNode {
public:
    std::unique_ptr<ExprNode> expr;
    ExprStatement(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}
    void accept(ASTVisitor& visitor) const override;
};

class RepeatDecl : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::vector<std::unique_ptr<StmtNode>> body;
    RepeatDecl(std::unique_ptr<ExprNode> condition)
        : condition(std::move(condition)) {}
    void accept(ASTVisitor& visitor) const override;
};

// ─── Pipe ────────────────────────────────────────────────────────────────────

class ContextNode {
public:
    std::vector<std::string> variables;
    ContextNode(const std::vector<std::string>& variables) : variables(variables) {}
};

enum class StepOpType {
    START_WITH,
    MAP,
    FILTER,
    TRANSFORM,
    YIELD
};

class StepNode : public ASTNode {
public:
    StepOpType op;
    std::unique_ptr<ExprNode> expr;
    std::unique_ptr<ContextNode> context;
    StepNode(StepOpType op, std::unique_ptr<ExprNode> expr, std::unique_ptr<ContextNode> context)
        : op(op), expr(std::move(expr)), context(std::move(context)) {}
    void accept(ASTVisitor& visitor) const override;
};

class PipeDecl : public StmtNode {
public:
    std::string name;
    std::vector<std::unique_ptr<StepNode>> steps;
    PipeDecl(const std::string& name) : name(name) {}
    void accept(ASTVisitor& visitor) const override;
};

// ─── Match ───────────────────────────────────────────────────────────────────

class MatchCaseNode : public ASTNode {
public:
    bool is_catch_error;
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> yield_stmt;
    MatchCaseNode(bool is_catch_error,
                  std::unique_ptr<ExprNode> condition,
                  std::unique_ptr<StmtNode> yield_stmt)
        : is_catch_error(is_catch_error),
          condition(std::move(condition)),
          yield_stmt(std::move(yield_stmt)) {}
    void accept(ASTVisitor& visitor) const override;
};

class MatchDecl : public StmtNode {
public:
    std::unique_ptr<ExprNode> expr;
    std::vector<std::unique_ptr<MatchCaseNode>> cases;
    MatchDecl(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}
    void accept(ASTVisitor& visitor) const override;
};

// ─── Program ─────────────────────────────────────────────────────────────────

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;
    void accept(ASTVisitor& visitor) const override;
};

// ─── Visitor ─────────────────────────────────────────────────────────────────

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const StringLiteralExpr& node) = 0;
    virtual void visit(const NumberExpr& node) = 0;
    virtual void visit(const BoolLiteralExpr& node) = 0;
    virtual void visit(const HoleExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const JsonObjectExpr& node) = 0;

    virtual void visit(const IntentDecl& node) = 0;
    virtual void visit(const LetDecl& node) = 0;
    virtual void visit(const SetDecl& node) = 0;
    virtual void visit(const PrintDecl& node) = 0;
    virtual void visit(const ExprStatement& node) = 0;
    virtual void visit(const RepeatDecl& node) = 0;

    virtual void visit(const StepNode& node) = 0;
    virtual void visit(const PipeDecl& node) = 0;

    virtual void visit(const MatchCaseNode& node) = 0;
    virtual void visit(const MatchDecl& node) = 0;

    virtual void visit(const ProgramNode& node) = 0;
};
