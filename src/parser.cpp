#include "parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::peek() const {
    return tokens[pos];
}

Token Parser::previous() const {
    return tokens[pos - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) pos++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(peek(), message);
    throw std::runtime_error(message);
}

void Parser::error(const Token& token, const std::string& message) {
    std::cerr << "Parse error at line " << token.line << ", col " << token.column
              << " [" << token.toString() << "]: " << message << std::endl;
}

void Parser::skipNewlines() {
    while (match(TokenType::NEWLINE)) {}
}

// ─── Program ──────────────────────────────────────────────────────────────────

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>();
    try {
        while (!isAtEnd()) {
            skipNewlines();
            if (isAtEnd()) break;
            program->statements.push_back(parseStatement());
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
    }
    return program;
}

// ─── Statements ───────────────────────────────────────────────────────────────

std::unique_ptr<StmtNode> Parser::parseStatement() {
    if (match(TokenType::INTENT))  return parseIntentDecl();
    if (match(TokenType::LET))     return parseLetDecl();
    if (match(TokenType::SET))     return parseSetDecl();
    if (match(TokenType::PRINT))   return parsePrintDecl();
    if (match(TokenType::REPEAT))  return parseRepeatDecl();
    if (match(TokenType::PIPE))    return parsePipeDecl();
    if (match(TokenType::MATCH))   return parseMatchDecl();

    // Fallback: treat a bare expression as an expression-statement (e.g. `yield true`)
    auto expr = parseExpression();
    return std::make_unique<ExprStatement>(std::move(expr));
}

std::unique_ptr<IntentDecl> Parser::parseIntentDecl() {
    Token strToken = consume(TokenType::STRING_LITERAL, "Expected string after 'intent'");
    return std::make_unique<IntentDecl>(strToken.value);
}

std::unique_ptr<LetDecl> Parser::parseLetDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    consume(TokenType::BE, "Expected 'be' after variable name");
    auto expr = parseExpression();
    return std::make_unique<LetDecl>(name.value, std::move(expr));
}

std::unique_ptr<SetDecl> Parser::parseSetDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'set'");
    consume(TokenType::BE, "Expected 'be' after variable name");
    auto expr = parseExpression();
    return std::make_unique<SetDecl>(name.value, std::move(expr));
}

std::unique_ptr<PrintDecl> Parser::parsePrintDecl() {
    auto expr = parseExpression();
    return std::make_unique<PrintDecl>(std::move(expr));
}

std::unique_ptr<RepeatDecl> Parser::parseRepeatDecl() {
    consume(TokenType::WHILE, "Expected 'while' after 'repeat'");
    auto condition = parseExpression();
    consume(TokenType::NEWLINE, "Expected newline after repeat condition");
    skipNewlines();

    auto repeatDecl = std::make_unique<RepeatDecl>(std::move(condition));

    while (check(TokenType::DASH)) {
        advance(); // consume '-'
        skipNewlines();
        auto stmt = parseStatement();
        repeatDecl->body.push_back(std::move(stmt));
        skipNewlines();
    }

    if (repeatDecl->body.empty()) {
        error(peek(), "Expected at least one statement in repeat body");
        throw std::runtime_error("Empty repeat body");
    }
    return repeatDecl;
}

std::unique_ptr<PipeDecl> Parser::parsePipeDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected pipeline name after 'pipe'");
    consume(TokenType::NEWLINE, "Expected newline after pipeline declaration");
    skipNewlines();

    auto pipe = std::make_unique<PipeDecl>(name.value);

    while (check(TokenType::DASH)) {
        pipe->steps.push_back(parseStep());
        skipNewlines();
    }

    if (pipe->steps.empty()) {
        error(peek(), "Expected at least one step in pipeline");
        throw std::runtime_error("Empty pipeline");
    }
    return pipe;
}

std::unique_ptr<StepNode> Parser::parseStep() {
    consume(TokenType::DASH, "Expected '-' for pipeline step");

    StepOpType opType;
    if (match(TokenType::START)) {
        consume(TokenType::WITH, "Expected 'with' after 'start'");
        opType = StepOpType::START_WITH;
    } else if (match(TokenType::MAP)) {
        opType = StepOpType::MAP;
    } else if (match(TokenType::FILTER)) {
        opType = StepOpType::FILTER;
    } else if (match(TokenType::TRANSFORM)) {
        opType = StepOpType::TRANSFORM;
    } else if (match(TokenType::YIELD)) {
        opType = StepOpType::YIELD;
    } else {
        error(peek(), "Expected step operation");
        throw std::runtime_error("Invalid step operation");
    }

    std::unique_ptr<ExprNode> expr = nullptr;
    std::unique_ptr<ContextNode> context = nullptr;

    if (opType == StepOpType::FILTER && match(TokenType::WHEN)) {
        expr = parseExpression();
    } else if (!check(TokenType::USING) && !check(TokenType::NEWLINE) && !isAtEnd()) {
        expr = parseExpression();
    }

    if (match(TokenType::USING)) {
        context = parseContext();
    }

    return std::make_unique<StepNode>(opType, std::move(expr), std::move(context));
}

std::unique_ptr<ContextNode> Parser::parseContext() {
    consume(TokenType::LBRACKET, "Expected '[' after 'using'");
    std::vector<std::string> vars;
    if (!check(TokenType::RBRACKET)) {
        do {
            vars.push_back(consume(TokenType::IDENTIFIER, "Expected variable name in context").value);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RBRACKET, "Expected ']' after context variables");
    return std::make_unique<ContextNode>(vars);
}

std::unique_ptr<MatchDecl> Parser::parseMatchDecl() {
    auto matchExpr = parseExpression();
    consume(TokenType::NEWLINE, "Expected newline after match expression");
    skipNewlines();

    auto matchDecl = std::make_unique<MatchDecl>(std::move(matchExpr));

    while (check(TokenType::DASH)) {
        // Only consume this dash if the next token is 'when' or 'catch' (match case)
        TokenType next = (pos + 1 < tokens.size()) ? tokens[pos + 1].type : TokenType::END_OF_FILE;
        if (next != TokenType::WHEN && next != TokenType::CATCH) break;
        matchDecl->cases.push_back(parseMatchCase());
        skipNewlines();
    }

    if (matchDecl->cases.empty()) {
        error(peek(), "Expected at least one case in match");
        throw std::runtime_error("Empty match");
    }
    return matchDecl;
}

std::unique_ptr<MatchCaseNode> Parser::parseMatchCase() {
    consume(TokenType::DASH, "Expected '-' for match case");

    if (match(TokenType::WHEN)) {
        auto cond = parseExpression();
        consume(TokenType::YIELD, "Expected 'yield' in match case");
        auto yield_stmt = parseStatement();
        return std::make_unique<MatchCaseNode>(false, std::move(cond), std::move(yield_stmt));
    } else if (match(TokenType::CATCH)) {
        consume(TokenType::ERROR, "Expected 'error' after 'catch'");
        consume(TokenType::YIELD, "Expected 'yield' in catch case");
        auto yield_stmt = parseStatement();
        return std::make_unique<MatchCaseNode>(true, nullptr, std::move(yield_stmt));
    } else {
        error(peek(), "Expected 'when' or 'catch error' in match case");
        throw std::runtime_error("Invalid match case");
    }
}

// ─── Expressions (precedence climbing) ────────────────────────────────────────

std::unique_ptr<ExprNode> Parser::parseExpression() {
    return parseOrExpr();
}

std::unique_ptr<ExprNode> Parser::parseOrExpr() {
    auto left = parseAndExpr();
    while (match(TokenType::OR)) {
        auto right = parseAndExpr();
        left = std::make_unique<BinaryExpr>("or", std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseAndExpr() {
    auto left = parseNotExpr();
    while (match(TokenType::AND)) {
        auto right = parseNotExpr();
        left = std::make_unique<BinaryExpr>("and", std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseNotExpr() {
    if (match(TokenType::NOT)) {
        auto operand = parseNotExpr();
        return std::make_unique<UnaryExpr>("not", std::move(operand));
    }
    return parseCompExpr();
}

std::unique_ptr<ExprNode> Parser::parseCompExpr() {
    auto left = parseAddExpr();

    if (match(TokenType::EQUALS)) {
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("equals", std::move(left), std::move(right));
    }
    // "greater than" — two-token operator
    if (match(TokenType::GREATER)) {
        consume(TokenType::THAN, "Expected 'than' after 'greater'");
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("greater than", std::move(left), std::move(right));
    }
    // "less than" — two-token operator
    if (match(TokenType::LESS)) {
        consume(TokenType::THAN, "Expected 'than' after 'less'");
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("less than", std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExprNode> Parser::parseAddExpr() {
    auto left = parseMulExpr();
    while (true) {
        if (match(TokenType::PLUS)) {
            auto right = parseMulExpr();
            left = std::make_unique<BinaryExpr>("plus", std::move(left), std::move(right));
        } else if (match(TokenType::MINUS)) {
            auto right = parseMulExpr();
            left = std::make_unique<BinaryExpr>("minus", std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parseMulExpr() {
    auto left = parsePrimary();
    while (true) {
        if (match(TokenType::TIMES)) {
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpr>("times", std::move(left), std::move(right));
        } else if (match(TokenType::DIVIDED)) {
            consume(TokenType::BY, "Expected 'by' after 'divided'");
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpr>("divided by", std::move(left), std::move(right));
        } else if (match(TokenType::MODULO)) {
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpr>("modulo", std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    if (match(TokenType::NUMBER)) {
        return std::make_unique<NumberExpr>(previous().value);
    }
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<StringLiteralExpr>(previous().value);
    }
    if (match(TokenType::TRUE_LIT)) {
        return std::make_unique<BoolLiteralExpr>(true);
    }
    if (match(TokenType::FALSE_LIT)) {
        return std::make_unique<BoolLiteralExpr>(false);
    }
    if (match(TokenType::HOLE)) {
        Token strToken = consume(TokenType::STRING_LITERAL, "Expected string literal after hole");
        return std::make_unique<HoleExpr>(strToken.value, strToken.line);
    }
    if (match(TokenType::LBRACE)) {
        return parseJsonObject();
    }
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' to close grouped expression");
        return expr;
    }
    // `value` and `result` are treated as identifiers in expressions
    if (match({TokenType::IDENTIFIER, TokenType::VALUE, TokenType::RESULT})) {
        return std::make_unique<IdentifierExpr>(previous().value);
    }

    error(peek(), "Expected expression");
    throw std::runtime_error("Expected expression");
}

std::unique_ptr<JsonObjectExpr> Parser::parseJsonObject() {
    auto obj = std::make_unique<JsonObjectExpr>();
    if (!check(TokenType::RBRACE)) {
        do {
            Token key = consume(TokenType::STRING_LITERAL, "Expected string key in JSON object");
            consume(TokenType::COLON, "Expected ':' after key");
            auto val = parseExpression();
            obj->fields.emplace_back(key.value, std::move(val));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RBRACE, "Expected '}' to close JSON object");
    return obj;
}
