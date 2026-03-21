#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <stdexcept>
#include <unordered_set>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ProgramNode> parse();
    bool hasErrors() const { return errorCount > 0; }

private:
    std::vector<Token> tokens;
    size_t pos;
    int errorCount = 0;

    // Fix 2: flat set of declared variable names for loop-variable shadow check
    std::unordered_set<std::string> declaredVars;

    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void skipNewlines();

    // ── Statements ────────────────────────────────────────────────────────────
    std::unique_ptr<StmtNode>          parseStatement();
    std::unique_ptr<IntentDecl>        parseIntentDecl();
    std::unique_ptr<LetDecl>           parseLetDecl();
    std::unique_ptr<SetDecl>           parseSetDecl();
    std::unique_ptr<PrintDecl>         parsePrintDecl();
    std::unique_ptr<SkipDecl>          parseSkipDecl();
    std::unique_ptr<ReadDecl>          parseReadDecl();
    std::unique_ptr<StmtNode>          parseRepeatDecl();   // dispatches to while or times form
    std::unique_ptr<ForEachDecl>       parseForEachDecl();
    std::unique_ptr<IfDecl>            parseIfDecl();
    std::unique_ptr<FunctionDecl>      parseFunctionDecl();
    std::unique_ptr<ReturnStmt>        parseReturnStmt();
    std::unique_ptr<CallStmt>          parseCallStmt();
    std::unique_ptr<PutDecl>           parsePutDecl();
    std::unique_ptr<PipeDecl>          parsePipeDecl();
    std::unique_ptr<MatchDecl>         parseMatchDecl();

    std::unique_ptr<StepNode>          parseStep();
    std::unique_ptr<MatchCaseNode>     parseMatchCase();

    // ── Expressions (precedence climbing) ─────────────────────────────────────
    std::unique_ptr<ExprNode>          parseExpression();
    std::unique_ptr<ExprNode>          parseOrExpr();
    std::unique_ptr<ExprNode>          parseAndExpr();
    std::unique_ptr<ExprNode>          parseNotExpr();
    std::unique_ptr<ExprNode>          parseCompExpr();
    std::unique_ptr<ExprNode>          parseAddExpr();
    std::unique_ptr<ExprNode>          parseMulExpr();
    std::unique_ptr<ExprNode>          parsePrimary();

    std::unique_ptr<JsonObjectExpr>    parseJsonObject();

    // Helper: parse optional "passing arg1, arg2, ..." argument list
    std::vector<std::unique_ptr<ExprNode>> parseCallArgs();

    void error(const Token& token, const std::string& message);
};
