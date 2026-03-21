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
    ++errorCount;
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
    if (match(TokenType::INTENT))    return parseIntentDecl();
    if (match(TokenType::LET))       return parseLetDecl();
    if (match(TokenType::SET))       return parseSetDecl();
    if (match(TokenType::PRINT))     return parsePrintDecl();
    if (match(TokenType::SKIP))      return parseSkipDecl();
    if (match(TokenType::READ))      return parseReadDecl();    // Fix 6
    if (match(TokenType::REPEAT))    return parseRepeatDecl();
    if (match(TokenType::FOR))       return parseForEachDecl(); // Fix 2
    if (match(TokenType::IF))        return parseIfDecl();
    if (match(TokenType::DEFINE))    return parseFunctionDecl();
    if (match(TokenType::RETURN_KW)) return parseReturnStmt();
    if (match(TokenType::CALL_KW))   return parseCallStmt();
    if (match(TokenType::PUT))       return parsePutDecl();
    if (match(TokenType::PIPE))      return parsePipeDecl();
    if (match(TokenType::MATCH))     return parseMatchDecl();

    error(peek(), "Expected statement");
    throw std::runtime_error("Expected statement, got: " + peek().toString());
}

std::unique_ptr<IntentDecl> Parser::parseIntentDecl() {
    Token strToken = consume(TokenType::STRING_LITERAL, "Expected string after 'intent'");
    return std::make_unique<IntentDecl>(strToken.value);
}

std::unique_ptr<LetDecl> Parser::parseLetDecl() {
    // Fix 3: `value` is the only reserved pipe register; `result` is now a plain identifier
    if (check(TokenType::VALUE)) {
        error(peek(), "'value' is a reserved word and cannot be used as a variable name");
        throw std::runtime_error("Reserved word used as variable name");
    }
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    consume(TokenType::BE, "Expected 'be' after variable name");
    auto expr = parseExpression();
    // Fix 2: track declared variables for for-each shadow check
    declaredVars.insert(name.value);
    return std::make_unique<LetDecl>(name.value, std::move(expr));
}

std::unique_ptr<SetDecl> Parser::parseSetDecl() {
    if (check(TokenType::VALUE)) {
        error(peek(), "'value' is a reserved word and cannot be used as a variable name");
        throw std::runtime_error("Reserved word used as variable name");
    }
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'set'");
    consume(TokenType::BE, "Expected 'be' after variable name");
    auto expr = parseExpression();
    return std::make_unique<SetDecl>(name.value, std::move(expr));
}

std::unique_ptr<PrintDecl> Parser::parsePrintDecl() {
    auto expr = parseExpression();
    return std::make_unique<PrintDecl>(std::move(expr));
}

std::unique_ptr<SkipDecl> Parser::parseSkipDecl() {
    return std::make_unique<SkipDecl>();
}

// Fix 6: read <name>
std::unique_ptr<ReadDecl> Parser::parseReadDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'read'");
    return std::make_unique<ReadDecl>(name.value);
}

// ─── repeat while <cond> … end  /  repeat <count> times … end ────────────────

std::unique_ptr<StmtNode> Parser::parseRepeatDecl() {
    if (match(TokenType::WHILE)) {
        // Original while form
        auto condition = parseExpression();
        consume(TokenType::NEWLINE, "Expected newline after repeat condition");
        skipNewlines();

        auto repeatDecl = std::make_unique<RepeatDecl>(std::move(condition));
        while (!check(TokenType::END) && !isAtEnd()) {
            skipNewlines();
            if (check(TokenType::END)) break;
            repeatDecl->body.push_back(parseStatement());
            skipNewlines();
        }
        consume(TokenType::END, "Expected 'end' to close repeat block");
        return repeatDecl;
    }

    // Fix 2: count-controlled form — `repeat <primary-expr> times … end`
    // Using parsePrimary so `times` is not consumed as a multiplication operator.
    // Wrap complex count expressions in parentheses: repeat (a times b) times
    auto countExpr = parsePrimary();
    consume(TokenType::TIMES, "Expected 'while' or '<count> times' after 'repeat'");
    consume(TokenType::NEWLINE, "Expected newline after 'times'");
    skipNewlines();

    auto repeatTimes = std::make_unique<RepeatTimesDecl>(std::move(countExpr));
    while (!check(TokenType::END) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::END)) break;
        repeatTimes->body.push_back(parseStatement());
        skipNewlines();
    }
    consume(TokenType::END, "Expected 'end' to close repeat block");
    return repeatTimes;
}

// Fix 2: for each <name> in <list-expr> … end
std::unique_ptr<ForEachDecl> Parser::parseForEachDecl() {
    consume(TokenType::EACH, "Expected 'each' after 'for'");
    Token varTok = consume(TokenType::IDENTIFIER, "Expected loop variable name after 'each'");

    // Enforce no shadowing of an outer-scope variable
    if (declaredVars.count(varTok.value)) {
        error(varTok, "'" + varTok.value + "' already declared in outer scope");
        throw std::runtime_error("Loop variable shadows outer scope variable: " + varTok.value);
    }

    consume(TokenType::IN, "Expected 'in' after loop variable name");
    auto listExpr = parseExpression();
    consume(TokenType::NEWLINE, "Expected newline after list expression");
    skipNewlines();

    auto forEach = std::make_unique<ForEachDecl>(varTok.value, std::move(listExpr));
    while (!check(TokenType::END) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::END)) break;
        forEach->body.push_back(parseStatement());
        skipNewlines();
    }
    consume(TokenType::END, "Expected 'end' to close for each block");
    return forEach;
}

// ─── if <cond> then … [elif <cond> then …] [else …] end ──────────────────────

std::unique_ptr<IfDecl> Parser::parseIfDecl() {
    auto ifDecl = std::make_unique<IfDecl>();

    auto firstCond = parseExpression();
    consume(TokenType::THEN, "Expected 'then' after if condition");
    consume(TokenType::NEWLINE, "Expected newline after 'then'");
    skipNewlines();

    std::vector<std::unique_ptr<StmtNode>> firstBody;
    while (!check(TokenType::ELIF) && !check(TokenType::ELSE) &&
           !check(TokenType::END) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::ELIF) || check(TokenType::ELSE) || check(TokenType::END)) break;
        firstBody.push_back(parseStatement());
        skipNewlines();
    }
    ifDecl->clauses.push_back(IfClause(std::move(firstCond), std::move(firstBody)));

    while (match(TokenType::ELIF)) {
        auto elifCond = parseExpression();
        consume(TokenType::THEN, "Expected 'then' after elif condition");
        consume(TokenType::NEWLINE, "Expected newline after 'then'");
        skipNewlines();

        std::vector<std::unique_ptr<StmtNode>> elifBody;
        while (!check(TokenType::ELIF) && !check(TokenType::ELSE) &&
               !check(TokenType::END) && !isAtEnd()) {
            skipNewlines();
            if (check(TokenType::ELIF) || check(TokenType::ELSE) || check(TokenType::END)) break;
            elifBody.push_back(parseStatement());
            skipNewlines();
        }
        ifDecl->clauses.push_back(IfClause(std::move(elifCond), std::move(elifBody)));
    }

    if (match(TokenType::ELSE)) {
        skipNewlines();
        while (!check(TokenType::END) && !isAtEnd()) {
            skipNewlines();
            if (check(TokenType::END)) break;
            ifDecl->else_body.push_back(parseStatement());
            skipNewlines();
        }
    }

    consume(TokenType::END, "Expected 'end' to close if block");
    return ifDecl;
}

// ─── define <name> [taking p1, p2] … end ─────────────────────────────────────

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'define'");

    std::vector<std::string> params;
    if (match(TokenType::TAKING)) {
        if (check(TokenType::IDENTIFIER)) {
            params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name").value);
            while (match(TokenType::COMMA)) {
                params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name").value);
            }
        }
    }

    consume(TokenType::NEWLINE, "Expected newline after function declaration");
    skipNewlines();

    auto funcDecl = std::make_unique<FunctionDecl>(name.value, std::move(params));

    while (!check(TokenType::END) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::END)) break;
        funcDecl->body.push_back(parseStatement());
        skipNewlines();
    }

    consume(TokenType::END, "Expected 'end' to close function definition");
    return funcDecl;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    auto expr = parseExpression();
    return std::make_unique<ReturnStmt>(std::move(expr));
}

// ─── call <name> [passing arg1, arg2]  — statement form ──────────────────────

std::unique_ptr<CallStmt> Parser::parseCallStmt() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'call'");
    auto args = parseCallArgs();
    return std::make_unique<CallStmt>(name.value, std::move(args));
}

std::vector<std::unique_ptr<ExprNode>> Parser::parseCallArgs() {
    std::vector<std::unique_ptr<ExprNode>> args;
    if (match(TokenType::PASSING)) {
        args.push_back(parseExpression());
        while (match(TokenType::COMMA)) {
            args.push_back(parseExpression());
        }
    }
    return args;
}

// ─── put <name> at <index> be <value> ────────────────────────────────────────

std::unique_ptr<PutDecl> Parser::parsePutDecl() {
    if (check(TokenType::VALUE)) {
        error(peek(), "'value' is a reserved word");
        throw std::runtime_error("Reserved word used as variable name");
    }
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'put'");
    consume(TokenType::AT, "Expected 'at' after variable name");
    auto index = parseExpression();
    consume(TokenType::BE, "Expected 'be' after index");
    auto val = parseExpression();
    return std::make_unique<PutDecl>(name.value, std::move(index), std::move(val));
}

// ─── pipe <name> … end ───────────────────────────────────────────────────────

std::unique_ptr<PipeDecl> Parser::parsePipeDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected pipeline name after 'pipe'");
    consume(TokenType::NEWLINE, "Expected newline after pipeline name");
    skipNewlines();

    auto pipe = std::make_unique<PipeDecl>(name.value);

    while (!check(TokenType::END) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::END)) break;
        pipe->steps.push_back(parseStep());
        skipNewlines();
    }

    consume(TokenType::END, "Expected 'end' to close pipe block");
    return pipe;
}

std::unique_ptr<StepNode> Parser::parseStep() {
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
        error(peek(), "Expected step keyword: start, map, filter, transform, or yield");
        throw std::runtime_error("Invalid step operation");
    }

    std::unique_ptr<ExprNode> expr = nullptr;

    if (opType == StepOpType::FILTER) {
        consume(TokenType::WHEN, "Expected 'when' after 'filter'");
        expr = parseExpression();
    } else if (opType == StepOpType::YIELD) {
        // Fix 3: yield must be followed by exactly one plain IDENTIFIER
        Token yieldName = consume(TokenType::IDENTIFIER,
                                  "Expected variable name after 'yield'");
        expr = std::make_unique<IdentifierExpr>(yieldName.value);
    } else if (!check(TokenType::NEWLINE) && !isAtEnd()) {
        expr = parseExpression();
    }

    return std::make_unique<StepNode>(opType, std::move(expr));
}

// ─── match <expr> … end ──────────────────────────────────────────────────────

std::unique_ptr<MatchDecl> Parser::parseMatchDecl() {
    auto matchExpr = parseExpression();
    consume(TokenType::NEWLINE, "Expected newline after match expression");
    skipNewlines();

    auto matchDecl = std::make_unique<MatchDecl>(std::move(matchExpr));

    while (!check(TokenType::END) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::END)) break;
        matchDecl->cases.push_back(parseMatchCase());
        skipNewlines();
    }

    consume(TokenType::END, "Expected 'end' to close match block");

    if (matchDecl->cases.empty() || !matchDecl->cases.back()->is_catch_error) {
        error(previous(), "match block must end with 'catch error yield ...'");
        throw std::runtime_error("match block must end with 'catch error yield ...'");
    }

    return matchDecl;
}

std::unique_ptr<MatchCaseNode> Parser::parseMatchCase() {
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
        error(peek(), "Expected 'when' or 'catch error' in match block");
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

    // Fix 1: add atleast (>=) and atmost (<=)
    if (match(TokenType::EQUALS)) {
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("equals", std::move(left), std::move(right));
    }
    if (match(TokenType::ABOVE)) {
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("above", std::move(left), std::move(right));
    }
    if (match(TokenType::BELOW)) {
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("below", std::move(left), std::move(right));
    }
    if (match(TokenType::ATLEAST)) {
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("atleast", std::move(left), std::move(right));
    }
    if (match(TokenType::ATMOST)) {
        auto right = parseAddExpr();
        return std::make_unique<BinaryExpr>("atmost", std::move(left), std::move(right));
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
        } else if (match(TokenType::DIVBY)) {
            auto right = parsePrimary();
            left = std::make_unique<BinaryExpr>("divby", std::move(left), std::move(right));
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
    // list <elem1>, <elem2>, ...
    if (match(TokenType::LIST_KW)) {
        auto listExpr = std::make_unique<ListExpr>();
        if (!check(TokenType::NEWLINE) && !isAtEnd()) {
            listExpr->elements.push_back(parseExpression());
            while (match(TokenType::COMMA)) {
                listExpr->elements.push_back(parseExpression());
            }
        }
        return listExpr;
    }
    // get <list-expr> at <index-expr>
    if (match(TokenType::GET_KW)) {
        auto containerExpr = parseExpression();
        consume(TokenType::AT, "Expected 'at' after container expression in 'get'");
        auto indexExpr = parseExpression();
        return std::make_unique<GetExpr>(std::move(containerExpr), std::move(indexExpr));
    }
    // call <name> [passing arg1, arg2, ...]  — expression form
    if (match(TokenType::CALL_KW)) {
        Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'call'");
        auto args = parseCallArgs();
        return std::make_unique<CallExpr>(name.value, std::move(args));
    }
    // Fix 3: `value` is still a reserved readable primary; `result` is now a plain IDENTIFIER
    if (match({TokenType::IDENTIFIER, TokenType::VALUE})) {
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
