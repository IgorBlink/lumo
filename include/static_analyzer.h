#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Lumo's type universe for static analysis
enum class LumoType {
    NUMBER,
    STRING,
    BOOLEAN,
    LIST,
    OBJECT,
    ANY,     // unknown or polymorphic
    NEVER    // unreachable code
};

std::string lumoTypeToString(LumoType t);

struct TypeInfo {
    LumoType type;
    int declarationLine;
};

struct FuncSig {
    std::string name;
    int paramCount;
    std::vector<std::string> paramNames;
    int declarationLine;
};

struct AnalysisError {
    enum class Severity { ERROR, WARNING };
    Severity severity;
    int line;
    std::string message;
};

struct AnalysisResult {
    std::unordered_map<std::string, TypeInfo> variables;
    std::unordered_map<std::string, FuncSig> functions;
    std::vector<AnalysisError> errors;
    // Variables that are mutated (via set)
    std::unordered_set<std::string> mutatedVars;
    // Lines where each variable is mutated
    std::unordered_map<std::string, std::vector<int>> mutationLines;
    // Variables that are referenced
    std::unordered_set<std::string> referencedVars;
    // Hole count
    int holeCount = 0;

    bool hasErrors() const;
};

class StaticAnalyzer : public ASTVisitor {
public:
    AnalysisResult analyze(const ProgramNode& program);

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
    AnalysisResult result;
    LumoType lastExprType = LumoType::ANY;
    bool inFunctionBody = false;
    bool afterReturn = false;

    // Scope stack for nested scopes (functions, loops)
    std::vector<std::unordered_map<std::string, TypeInfo>> scopes;

    void pushScope();
    void popScope();
    void declare(const std::string& name, LumoType type, int line);
    bool isDeclared(const std::string& name) const;
    LumoType lookupType(const std::string& name) const;
    void addError(int line, const std::string& msg);
    void addWarning(int line, const std::string& msg);
    LumoType inferExprType(const ExprNode& expr);
};
