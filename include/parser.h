#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <stdexcept>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens;
    size_t pos;

    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void skipNewlines();

    // Statements
    std::unique_ptr<StmtNode>    parseStatement();
    std::unique_ptr<IntentDecl>  parseIntentDecl();
    std::unique_ptr<LetDecl>     parseLetDecl();
    std::unique_ptr<SetDecl>     parseSetDecl();
    std::unique_ptr<PrintDecl>   parsePrintDecl();
    std::unique_ptr<RepeatDecl>  parseRepeatDecl();
    std::unique_ptr<PipeDecl>    parsePipeDecl();
    std::unique_ptr<MatchDecl>   parseMatchDecl();

    std::unique_ptr<StepNode>      parseStep();
    std::unique_ptr<ContextNode>   parseContext();
    std::unique_ptr<MatchCaseNode> parseMatchCase();

    // Expressions (precedence climbing)
    std::unique_ptr<ExprNode> parseExpression();
    std::unique_ptr<ExprNode> parseOrExpr();
    std::unique_ptr<ExprNode> parseAndExpr();
    std::unique_ptr<ExprNode> parseNotExpr();
    std::unique_ptr<ExprNode> parseCompExpr();
    std::unique_ptr<ExprNode> parseAddExpr();
    std::unique_ptr<ExprNode> parseMulExpr();
    std::unique_ptr<ExprNode> parsePrimary();

    std::unique_ptr<JsonObjectExpr> parseJsonObject();

    void error(const Token& token, const std::string& message);
};
