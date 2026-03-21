#pragma once
#include "ast.h"
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <stdexcept>

// ─── LumoValue (recursive via forward-declared LumoList / LumoObject) ─────────

struct LumoList;
struct LumoObject;

using LumoValue = std::variant<
    double,
    std::string,
    bool,
    std::shared_ptr<LumoList>,
    std::shared_ptr<LumoObject>   // Fix 4: first-class JSON objects
>;

struct LumoList   { std::vector<LumoValue> elements; };
struct LumoObject { std::map<std::string, LumoValue> fields; };  // Fix 4

std::string lumoValueToString(const LumoValue& v);

// ─── Exceptions ───────────────────────────────────────────────────────────────

class HaltException : public std::exception {
public:
    std::string message;
    explicit HaltException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class ReturnException : public std::exception {
public:
    LumoValue value;
    explicit ReturnException(LumoValue v) : value(std::move(v)) {}
    const char* what() const noexcept override { return "return"; }
};

// ─── Interpreter ─────────────────────────────────────────────────────────────

class Interpreter : public ASTVisitor {
public:
    void execute(const ProgramNode& program);

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
    std::unordered_map<std::string, LumoValue> env;       // current scope
    std::unordered_map<std::string, LumoValue> globalEnv; // Fix 5: top-level scope snapshot
    std::unordered_map<std::string, const FunctionDecl*> functions;
    LumoValue lastValue;
    LumoValue pipeValue;   // register for the active pipe element

    bool inFunction  = false; // Fix 5: true while inside a function call
    bool inPipeStep  = false; // Fix 7: true while inside a map/filter/transform step

    LumoValue eval(const ExprNode& expr);
    void execStmt(const StmtNode& stmt);
    LumoValue callFunction(const std::string& name,
                           const std::vector<std::unique_ptr<ExprNode>>& argExprs);

    bool isTruthy(const LumoValue& v) const;
    bool valuesEqual(const LumoValue& a, const LumoValue& b) const;
};
