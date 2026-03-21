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

// list 1, 2, 3
class ListExpr : public ExprNode {
public:
    std::vector<std::unique_ptr<ExprNode>> elements;
    void accept(ASTVisitor& visitor) const override;
};

// get <list-expr> at <index-expr>
class GetExpr : public ExprNode {
public:
    std::unique_ptr<ExprNode> list;
    std::unique_ptr<ExprNode> index;
    GetExpr(std::unique_ptr<ExprNode> list, std::unique_ptr<ExprNode> index)
        : list(std::move(list)), index(std::move(index)) {}
    void accept(ASTVisitor& visitor) const override;
};

// call <name> [passing arg1, arg2, ...]  — expression form
class CallExpr : public ExprNode {
public:
    std::string name;
    std::vector<std::unique_ptr<ExprNode>> args;
    CallExpr(const std::string& name, std::vector<std::unique_ptr<ExprNode>> args)
        : name(name), args(std::move(args)) {}
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

// skip — explicit no-op statement
class SkipDecl : public StmtNode {
public:
    void accept(ASTVisitor& visitor) const override;
};

// read <name>  — Fix 6: reads one line from stdin into an existing variable
class ReadDecl : public StmtNode {
public:
    std::string name;
    ReadDecl(const std::string& name) : name(name) {}
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

// Fix 2: repeat <count> times … end
class RepeatTimesDecl : public StmtNode {
public:
    std::unique_ptr<ExprNode> count;
    std::vector<std::unique_ptr<StmtNode>> body;
    RepeatTimesDecl(std::unique_ptr<ExprNode> count)
        : count(std::move(count)) {}
    void accept(ASTVisitor& visitor) const override;
};

// Fix 2: for each <name> in <list-expr> … end
class ForEachDecl : public StmtNode {
public:
    std::string varName;
    std::unique_ptr<ExprNode> listExpr;
    std::vector<std::unique_ptr<StmtNode>> body;
    ForEachDecl(const std::string& varName, std::unique_ptr<ExprNode> listExpr)
        : varName(varName), listExpr(std::move(listExpr)) {}
    void accept(ASTVisitor& visitor) const override;
};

// if/elif/else/end — one IfClause per if/elif branch
struct IfClause {
    std::unique_ptr<ExprNode> condition;
    std::vector<std::unique_ptr<StmtNode>> body;

    IfClause(std::unique_ptr<ExprNode> cond, std::vector<std::unique_ptr<StmtNode>> b)
        : condition(std::move(cond)), body(std::move(b)) {}

    IfClause(IfClause&&) = default;
    IfClause& operator=(IfClause&&) = default;
    IfClause(const IfClause&) = delete;
    IfClause& operator=(const IfClause&) = delete;
};

class IfDecl : public StmtNode {
public:
    std::vector<IfClause> clauses;               // if + zero or more elif branches
    std::vector<std::unique_ptr<StmtNode>> else_body; // optional else branch
    void accept(ASTVisitor& visitor) const override;
};

// define <name> [taking p1, p2, ...]  ...body...  end
class FunctionDecl : public StmtNode {
public:
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<StmtNode>> body;
    FunctionDecl(const std::string& name, std::vector<std::string> params)
        : name(name), params(std::move(params)) {}
    void accept(ASTVisitor& visitor) const override;
};

// return <expr>
class ReturnStmt : public StmtNode {
public:
    std::unique_ptr<ExprNode> expr;
    ReturnStmt(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}
    void accept(ASTVisitor& visitor) const override;
};

// call <name> [passing arg1, arg2, ...]  — statement form (return value discarded)
class CallStmt : public StmtNode {
public:
    std::string name;
    std::vector<std::unique_ptr<ExprNode>> args;
    CallStmt(const std::string& name, std::vector<std::unique_ptr<ExprNode>> args)
        : name(name), args(std::move(args)) {}
    void accept(ASTVisitor& visitor) const override;
};

// put <name> at <index-expr> be <value-expr>
class PutDecl : public StmtNode {
public:
    std::string name;
    std::unique_ptr<ExprNode> index;
    std::unique_ptr<ExprNode> value;
    PutDecl(const std::string& name,
            std::unique_ptr<ExprNode> index,
            std::unique_ptr<ExprNode> value)
        : name(name), index(std::move(index)), value(std::move(value)) {}
    void accept(ASTVisitor& visitor) const override;
};

// ─── Pipe ────────────────────────────────────────────────────────────────────

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
    StepNode(StepOpType op, std::unique_ptr<ExprNode> expr)
        : op(op), expr(std::move(expr)) {}
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
    std::unique_ptr<ExprNode> condition;   // null when is_catch_error == true
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

    // Expressions
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const StringLiteralExpr& node) = 0;
    virtual void visit(const NumberExpr& node) = 0;
    virtual void visit(const BoolLiteralExpr& node) = 0;
    virtual void visit(const HoleExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const JsonObjectExpr& node) = 0;
    virtual void visit(const ListExpr& node) = 0;
    virtual void visit(const GetExpr& node) = 0;
    virtual void visit(const CallExpr& node) = 0;

    // Statements
    virtual void visit(const IntentDecl& node) = 0;
    virtual void visit(const LetDecl& node) = 0;
    virtual void visit(const SetDecl& node) = 0;
    virtual void visit(const PrintDecl& node) = 0;
    virtual void visit(const SkipDecl& node) = 0;
    virtual void visit(const ReadDecl& node) = 0;
    virtual void visit(const RepeatDecl& node) = 0;
    virtual void visit(const RepeatTimesDecl& node) = 0;
    virtual void visit(const ForEachDecl& node) = 0;
    virtual void visit(const IfDecl& node) = 0;
    virtual void visit(const FunctionDecl& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const CallStmt& node) = 0;
    virtual void visit(const PutDecl& node) = 0;

    // Pipe
    virtual void visit(const StepNode& node) = 0;
    virtual void visit(const PipeDecl& node) = 0;

    // Match
    virtual void visit(const MatchCaseNode& node) = 0;
    virtual void visit(const MatchDecl& node) = 0;

    // Program
    virtual void visit(const ProgramNode& node) = 0;
};
