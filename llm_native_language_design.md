# Lumo v2: Full Implementation Prompt

## Role

You are a Lead Compiler Engineer. You will upgrade an existing LLM-Native programming language called **Lumo** from a parse-only frontend into a **fully executable, Turing-complete language** with an interpreter backend. The language must compile and run `.lumo` files from the terminal.

## Goal

After your changes, the following command must work end-to-end:

```bash
clang++ -std=c++17 -Iinclude src/*.cpp -o lumo
./lumo examples/hello.lumo
```

And the user must be able to write programs like:

```lumo
intent "Basic arithmetic and console output"
let a be 5
let b be 10
let result be a plus b
print result
```

**Expected terminal output:**

```
15
```

---

## What Lumo Must Support After Your Changes

### 1. Arithmetic Expressions (Turing-complete requirement: computation)

Use plain English keywords instead of symbols. This is a core LLM-Native design rule.

| Lumo keyword | Meaning |
|---|---|
| `plus` | `+` |
| `minus` | `-` |
| `times` | `*` |
| `divided by` | `/` |
| `modulo` | `%` |
| `greater than` | `>` |
| `less than` | `<` |
| `equals` | `==` |
| `and` | `&&` |
| `or` | `\|\|` |
| `not` | `!` |

Example:

```lumo
let x be 10 plus 5 times 2
print x
```

Output: `20` (standard math precedence: `times` binds tighter than `plus`)

### 2. Print Statement (console output)

```lumo
print "Hello, World!"
print 5 plus 5
print x
```

### 3. Boolean Literals

`true` and `false` are first-class values.

```lumo
let flag be true
```

### 4. Conditionals via `match` (Turing-complete requirement: branching)

```lumo
let status be "active"
match status
- when "active" yield print "User is active"
- when "banned" yield print "User is banned"
```

### 5. Loops via `repeat` (Turing-complete requirement: unbounded iteration)

Use a flat, autoregressive-friendly loop construct. No deep nesting. The `repeat while` keyword reads like natural language.

```lumo
let counter be 0
repeat while counter less than 10
- print counter
- set counter be counter plus 1
```

This gives us Turing-completeness (variables + branching + unbounded loops).

### 6. Mutable Reassignment via `set`

`let` creates a new binding. `set` mutates an existing one. This explicit distinction prevents LLM hallucination about which variables are mutable.

```lumo
let x be 5
set x be x plus 1
print x
```

Output: `6`

### 7. Pipes (already parsed, now must execute)

```lumo
let data be 100
pipe double_it
- start with data
- transform value times 2
- yield result
print result
```

Output: `200`

### 8. First-Class Holes (already parsed, runtime behavior)

When the interpreter encounters a `???` node, it must print a clear message and halt:

```
[HOLE] Unresolved hole at line 5: "Format as a friendly greeting string"
```

### 9. Intent (already parsed, runtime behavior)

When the interpreter encounters an `intent`, it prints it as a comment/trace:

```
[INTENT] Initialize user profile for processing
```

---

## Updated EBNF Grammar

```ebnf
Program      ::= Statement*
Statement    ::= IntentDecl | LetDecl | SetDecl | PrintDecl
               | PipeDecl | MatchDecl | RepeatDecl

IntentDecl   ::= "intent" StringLiteral
LetDecl      ::= "let" Identifier "be" Expression
SetDecl      ::= "set" Identifier "be" Expression
PrintDecl    ::= "print" Expression

PipeDecl     ::= "pipe" Identifier Newline Step+
Step         ::= "-" StepOp Expression? Context?
StepOp       ::= "start" "with" | "map" | "filter" | "transform" | "yield"
Context      ::= "using" "[" IdentifierList "]"

MatchDecl    ::= "match" Expression Newline MatchCase+
MatchCase    ::= "-" "when" Expression "yield" Statement
               | "-" "catch" "error" "yield" Statement

RepeatDecl   ::= "repeat" "while" Expression Newline RepeatBody+
RepeatBody   ::= "-" Statement

Expression   ::= OrExpr
OrExpr       ::= AndExpr ("or" AndExpr)*
AndExpr      ::= NotExpr ("and" NotExpr)*
NotExpr      ::= "not" NotExpr | CompExpr
CompExpr     ::= AddExpr (("equals"|"greater than"|"less than") AddExpr)?
AddExpr      ::= MulExpr (("plus"|"minus") MulExpr)*
MulExpr      ::= Primary (("times"|"divided by"|"modulo") Primary)*
Primary      ::= Number | StringLiteral | "true" | "false"
               | Identifier | Hole | "(" Expression ")"
Hole         ::= "???" StringLiteral
```

---

## Current Codebase (COMPLETE — Do Not Guess, Use These Exactly)

### File: `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(LumoCompiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(lumo ${SOURCES})
```

### File: `include/lexer.h`

```cpp
#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TokenType {
    // Keywords
    INTENT,
    LET,
    BE,
    PIPE,
    MATCH,
    WHEN,
    CATCH,
    ERROR,
    YIELD,
    START,
    WITH,
    MAP,
    FILTER,
    TRANSFORM,
    USING,

    // Punctuation and special tokens
    HOLE, // ???
    DASH, // -
    LBRACKET, // [
    RBRACKET, // ]
    LBRACE, // {
    RBRACE, // }
    COLON, // :
    COMMA, // ,
    
    // Literals
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,
    
    // Control
    NEWLINE,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    std::string toString() const;
};

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos;
    int line;
    int column;

    char peek() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();
    Token nextToken();
    Token makeToken(TokenType type, const std::string& value);
    Token lexString();
    Token lexNumber();
    Token lexIdentifierOrKeyword();
};
```

### File: `src/lexer.cpp`

```cpp
#include "lexer.h"
#include <cctype>
#include <unordered_map>

std::string Token::toString() const {
    switch (type) {
        case TokenType::INTENT: return "INTENT";
        case TokenType::LET: return "LET";
        case TokenType::BE: return "BE";
        case TokenType::PIPE: return "PIPE";
        case TokenType::MATCH: return "MATCH";
        case TokenType::WHEN: return "WHEN";
        case TokenType::CATCH: return "CATCH";
        case TokenType::ERROR: return "ERROR";
        case TokenType::YIELD: return "YIELD";
        case TokenType::START: return "START";
        case TokenType::WITH: return "WITH";
        case TokenType::MAP: return "MAP";
        case TokenType::FILTER: return "FILTER";
        case TokenType::TRANSFORM: return "TRANSFORM";
        case TokenType::USING: return "USING";
        case TokenType::HOLE: return "HOLE";
        case TokenType::DASH: return "DASH";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COLON: return "COLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::IDENTIFIER: return "IDENTIFIER(" + value + ")";
        case TokenType::STRING_LITERAL: return "STRING(\"" + value + "\")";
        case TokenType::NUMBER: return "NUMBER(" + value + ")";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::END_OF_FILE: return "EOF";
        default: return "UNKNOWN(" + value + ")";
    }
}

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), column(1) {}

bool Lexer::isAtEnd() const {
    return pos >= source.length();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') {
            advance();
        } else if (c == '#') {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::string& value) {
    return Token{type, value, line, column - (int)value.length()};
}

Token Lexer::lexString() {
    int startCol = column;
    int startLine = line;
    advance();
    std::string value;
    while (!isAtEnd() && peek() != '"') {
        value += advance();
    }
    if (isAtEnd()) {
        return Token{TokenType::UNKNOWN, value, startLine, startCol};
    }
    advance();
    return Token{TokenType::STRING_LITERAL, value, startLine, startCol};
}

Token Lexer::lexNumber() {
    int startCol = column;
    int startLine = line;
    std::string value;
    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.')) {
        value += advance();
    }
    return Token{TokenType::NUMBER, value, startLine, startCol};
}

Token Lexer::lexIdentifierOrKeyword() {
    int startCol = column;
    int startLine = line;
    std::string value;
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }

    static const std::unordered_map<std::string, TokenType> keywords = {
        {"intent", TokenType::INTENT},
        {"let", TokenType::LET},
        {"be", TokenType::BE},
        {"pipe", TokenType::PIPE},
        {"match", TokenType::MATCH},
        {"when", TokenType::WHEN},
        {"catch", TokenType::CATCH},
        {"error", TokenType::ERROR},
        {"yield", TokenType::YIELD},
        {"start", TokenType::START},
        {"with", TokenType::WITH},
        {"map", TokenType::MAP},
        {"filter", TokenType::FILTER},
        {"transform", TokenType::TRANSFORM},
        {"using", TokenType::USING}
    };

    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return Token{it->second, value, startLine, startCol};
    }
    return Token{TokenType::IDENTIFIER, value, startLine, startCol};
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();
    if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, "");

    char c = peek();
    
    if (c == '\n') {
        int startCol = column;
        int startLine = line;
        advance();
        return Token{TokenType::NEWLINE, "\\n", startLine, startCol};
    }

    if (std::isalpha(c) || c == '_') {
        return lexIdentifierOrKeyword();
    }
    
    if (std::isdigit(c)) {
        return lexNumber();
    }
    
    if (c == '"') {
        return lexString();
    }

    int startCol = column;
    int startLine = line;
    advance();
    switch (c) {
        case '-': return Token{TokenType::DASH, "-", startLine, startCol};
        case '[': return Token{TokenType::LBRACKET, "[", startLine, startCol};
        case ']': return Token{TokenType::RBRACKET, "]", startLine, startCol};
        case '{': return Token{TokenType::LBRACE, "{", startLine, startCol};
        case '}': return Token{TokenType::RBRACE, "}", startLine, startCol};
        case ':': return Token{TokenType::COLON, ":", startLine, startCol};
        case ',': return Token{TokenType::COMMA, ",", startLine, startCol};
        case '?':
            if (peek() == '?' && source.length() > pos && source[pos] == '?') {
                advance();
                advance();
                return Token{TokenType::HOLE, "???", startLine, startCol};
            }
            break;
    }

    return Token{TokenType::UNKNOWN, std::string(1, c), startLine, startCol};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token t = nextToken();
        tokens.push_back(t);
        if (t.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    return tokens;
}
```

### File: `include/ast.h`

```cpp
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

class HoleExpr : public ExprNode {
public:
    std::string instruction;
    HoleExpr(const std::string& instruction) : instruction(instruction) {}
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

class MatchCaseNode : public ASTNode {
public:
    bool is_catch_error;
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<ExprNode> yield_expr;
    MatchCaseNode(bool is_catch_error, std::unique_ptr<ExprNode> condition, std::unique_ptr<ExprNode> yield_expr)
        : is_catch_error(is_catch_error), condition(std::move(condition)), yield_expr(std::move(yield_expr)) {}
    void accept(ASTVisitor& visitor) const override;
};

class MatchDecl : public StmtNode {
public:
    std::string name;
    std::vector<std::unique_ptr<MatchCaseNode>> cases;
    MatchDecl(const std::string& name) : name(name) {}
    void accept(ASTVisitor& visitor) const override;
};

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;
    void accept(ASTVisitor& visitor) const override;
};

class ASTVisitor {
public:
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const StringLiteralExpr& node) = 0;
    virtual void visit(const NumberExpr& node) = 0;
    virtual void visit(const HoleExpr& node) = 0;
    virtual void visit(const JsonObjectExpr& node) = 0;
    virtual void visit(const IntentDecl& node) = 0;
    virtual void visit(const LetDecl& node) = 0;
    virtual void visit(const StepNode& node) = 0;
    virtual void visit(const PipeDecl& node) = 0;
    virtual void visit(const MatchCaseNode& node) = 0;
    virtual void visit(const MatchDecl& node) = 0;
    virtual void visit(const ProgramNode& node) = 0;
};
```

### File: `src/ast.cpp`

```cpp
#include "ast.h"

void IdentifierExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void StringLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void NumberExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void HoleExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void JsonObjectExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

void IntentDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void LetDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

void StepNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void PipeDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

void MatchCaseNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MatchDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

void ProgramNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
```

### File: `include/parser.h`

```cpp
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

    std::unique_ptr<StmtNode> parseStatement();
    std::unique_ptr<IntentDecl> parseIntentDecl();
    std::unique_ptr<LetDecl> parseLetDecl();
    std::unique_ptr<PipeDecl> parsePipeDecl();
    std::unique_ptr<MatchDecl> parseMatchDecl();
    
    std::unique_ptr<StepNode> parseStep();
    std::unique_ptr<ContextNode> parseContext();
    std::unique_ptr<MatchCaseNode> parseMatchCase();

    std::unique_ptr<ExprNode> parseExpression();
    std::unique_ptr<JsonObjectExpr> parseJsonObject();
    
    void error(const Token& token, const std::string& message);
};
```

### File: `src/parser.cpp`

```cpp
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
    std::cerr << "Error at line " << token.line << ", col " << token.column 
              << " [" << token.toString() << "]: " << message << std::endl;
}

void Parser::skipNewlines() {
    while (match(TokenType::NEWLINE)) {}
}

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>();
    try {
        while (!isAtEnd()) {
            skipNewlines();
            if (isAtEnd()) break;
            program->statements.push_back(parseStatement());
        }
    } catch (const std::exception& e) {
    }
    return program;
}

std::unique_ptr<StmtNode> Parser::parseStatement() {
    if (match(TokenType::INTENT)) return parseIntentDecl();
    if (match(TokenType::LET)) return parseLetDecl();
    if (match(TokenType::PIPE)) return parsePipeDecl();
    if (match(TokenType::MATCH)) return parseMatchDecl();

    error(peek(), "Expected statement");
    throw std::runtime_error("Expected statement");
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
    Token name = consume(TokenType::IDENTIFIER, "Expected identifier after 'match'");
    consume(TokenType::NEWLINE, "Expected newline after match declaration");
    skipNewlines();

    auto matchDecl = std::make_unique<MatchDecl>(name.value);
    
    while (check(TokenType::DASH)) {
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
        auto yield_expr = parseExpression();
        return std::make_unique<MatchCaseNode>(false, std::move(cond), std::move(yield_expr));
    } else if (match(TokenType::CATCH)) {
        consume(TokenType::ERROR, "Expected 'error' after 'catch'");
        consume(TokenType::YIELD, "Expected 'yield' in catch case");
        auto yield_expr = parseExpression();
        return std::make_unique<MatchCaseNode>(true, nullptr, std::move(yield_expr));
    } else {
        error(peek(), "Expected 'when' or 'catch error' in match case");
        throw std::runtime_error("Invalid match case");
    }
}

std::unique_ptr<ExprNode> Parser::parseExpression() {
    if (match(TokenType::HOLE)) {
        Token strToken = consume(TokenType::STRING_LITERAL, "Expected string literal after hole");
        return std::make_unique<HoleExpr>(strToken.value);
    }
    
    if (match(TokenType::LBRACE)) {
        return parseJsonObject();
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<StringLiteralExpr>(previous().value);
    }
    
    if (match(TokenType::NUMBER)) {
        return std::make_unique<NumberExpr>(previous().value);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        auto expr = std::make_unique<IdentifierExpr>(previous().value);
        while (check(TokenType::IDENTIFIER) || check(TokenType::NUMBER)) {
            advance();
        }
        return expr;
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
```

### File: `include/ast_printer.h`

```cpp
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
    void visit(const HoleExpr& node) override;
    void visit(const JsonObjectExpr& node) override;
    
    void visit(const IntentDecl& node) override;
    void visit(const LetDecl& node) override;
    
    void visit(const StepNode& node) override;
    void visit(const PipeDecl& node) override;
    
    void visit(const MatchCaseNode& node) override;
    void visit(const MatchDecl& node) override;
    
    void visit(const ProgramNode& node) override;

private:
    int indent = 0;
    void printIndent() const;
};
```

### File: `src/ast_printer.cpp`

```cpp
#include "ast_printer.h"
#include <iostream>

void ASTPrinter::printIndent() const {
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
}

void ASTPrinter::print(const ASTNode& node) {
    node.accept(*this);
}

void ASTPrinter::visit(const IdentifierExpr& node) {
    std::cout << "IdentifierExpr(" << node.name << ")";
}

void ASTPrinter::visit(const StringLiteralExpr& node) {
    std::cout << "StringLiteralExpr(\"" << node.value << "\")";
}

void ASTPrinter::visit(const NumberExpr& node) {
    std::cout << "NumberExpr(" << node.value << ")";
}

void ASTPrinter::visit(const HoleExpr& node) {
    std::cout << "HoleExpr(instruction=\"" << node.instruction << "\")";
}

void ASTPrinter::visit(const JsonObjectExpr& node) {
    std::cout << "JsonObjectExpr({";
    for (size_t i = 0; i < node.fields.size(); ++i) {
        std::cout << "\"" << node.fields[i].key << "\": ";
        node.fields[i].value->accept(*this);
        if (i + 1 < node.fields.size()) std::cout << ", ";
    }
    std::cout << "})";
}

void ASTPrinter::visit(const IntentDecl& node) {
    printIndent();
    std::cout << "IntentDecl(\"" << node.intent_string << "\")\n";
}

void ASTPrinter::visit(const LetDecl& node) {
    printIndent();
    std::cout << "LetDecl(name=" << node.name << ", value=";
    node.value->accept(*this);
    std::cout << ")\n";
}

void ASTPrinter::visit(const StepNode& node) {
    printIndent();
    std::cout << "StepNode(op=";
    switch (node.op) {
        case StepOpType::START_WITH: std::cout << "START_WITH"; break;
        case StepOpType::MAP: std::cout << "MAP"; break;
        case StepOpType::FILTER: std::cout << "FILTER"; break;
        case StepOpType::TRANSFORM: std::cout << "TRANSFORM"; break;
        case StepOpType::YIELD: std::cout << "YIELD"; break;
    }
    if (node.expr) {
        std::cout << ", expr=";
        node.expr->accept(*this);
    }
    if (node.context) {
        std::cout << ", context=[";
        for (size_t i = 0; i < node.context->variables.size(); ++i) {
            std::cout << node.context->variables[i];
            if (i + 1 < node.context->variables.size()) std::cout << ", ";
        }
        std::cout << "]";
    }
    std::cout << ")\n";
}

void ASTPrinter::visit(const PipeDecl& node) {
    printIndent();
    std::cout << "PipeDecl(name=" << node.name << ") {\n";
    indent++;
    for (const auto& step : node.steps) {
        step->accept(*this);
    }
    indent--;
    printIndent();
    std::cout << "}\n";
}

void ASTPrinter::visit(const MatchCaseNode& node) {
    printIndent();
    if (node.is_catch_error) {
        std::cout << "MatchCase(catch error, yield=";
        node.yield_expr->accept(*this);
        std::cout << ")\n";
    } else {
        std::cout << "MatchCase(when=";
        node.condition->accept(*this);
        std::cout << ", yield=";
        node.yield_expr->accept(*this);
        std::cout << ")\n";
    }
}

void ASTPrinter::visit(const MatchDecl& node) {
    printIndent();
    std::cout << "MatchDecl(name=" << node.name << ") {\n";
    indent++;
    for (const auto& matchCase : node.cases) {
        matchCase->accept(*this);
    }
    indent--;
    printIndent();
    std::cout << "}\n";
}

void ASTPrinter::visit(const ProgramNode& node) {
    std::cout << "Program {\n";
    indent++;
    for (const auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    indent--;
    std::cout << "}\n";
}
```

### File: `src/main.cpp`

```cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"

int main(int argc, char* argv[]) {
    std::string source;
    
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << argv[1] << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
    } else {
        source = 
            "intent \"Initialize user profile for processing\"\n"
            "let user_data be { \"name\": \"Alice\", \"age\": 30 }\n"
            "\n"
            "pipe process_user\n"
            "- start with user_data\n"
            "- map using [age]\n"
            "- filter when age greater than 18\n"
            "- yield result\n"
            "\n"
            "pipe generate_greeting\n"
            "- start with user_data\n"
            "- map using [name]\n"
            "- transform ??? \"Format as a friendly greeting string\"\n"
            "- yield result\n"
            "\n"
            "match user_status\n"
            "- when \"active\" yield true\n"
            "- when \"banned\" yield false\n"
            "- catch error yield ??? \"Provide a fallback boolean\"\n";
    }
    
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    Parser parser(tokens);
    auto program = parser.parse();
    
    std::cout << "--- AST ---" << std::endl;
    ASTPrinter printer;
    program->accept(printer);

    return 0;
}
```

---

## Exact Changes Required

You must modify and create files to achieve the following. Do NOT delete or rename existing files unless replacing their content entirely.

### PHASE 1: Extend the Lexer

Add the following new keywords to `TokenType` in `include/lexer.h` and to the keyword map in `src/lexer.cpp`:

| Keyword | TokenType |
|---|---|
| `print` | `PRINT` |
| `set` | `SET` |
| `repeat` | `REPEAT` |
| `while` | `WHILE` |
| `plus` | `PLUS` |
| `minus` | `MINUS` |
| `times` | `TIMES` |
| `divided` | `DIVIDED` |
| `by` | `BY` |
| `modulo` | `MODULO` |
| `greater` | `GREATER` |
| `less` | `LESS` |
| `than` | `THAN` |
| `equals` | `EQUALS` |
| `and` | `AND` |
| `or` | `OR` |
| `not` | `NOT` |
| `true` | `TRUE_LIT` |
| `false` | `FALSE_LIT` |
| `value` | `VALUE` |
| `result` | `RESULT` |

Also add `LPAREN` and `RPAREN` for `(` and `)` to allow grouped expressions.

### PHASE 2: Extend the AST

Add the following new node types to `include/ast.h`:

1. **`BinaryExpr`**: left operand, operator string (e.g., "plus"), right operand. Represents `5 plus 5`, `x times 2`, `a greater than b`, etc.
2. **`UnaryExpr`**: operator string (e.g., "not"), operand. Represents `not flag`.
3. **`BoolLiteralExpr`**: boolean value (`true` / `false`).
4. **`PrintDecl`**: statement that prints an expression. (`print x`)
5. **`SetDecl`**: statement that reassigns a variable. (`set x be expr`)
6. **`RepeatDecl`**: statement with a condition expression and a body of statements (list of `-` prefixed statements). (`repeat while condition ...`)

Add corresponding `visit()` methods to `ASTVisitor`.
Add `accept()` implementations in `src/ast.cpp`.

### PHASE 3: Rewrite the Parser's Expression Handling

The current `parseExpression()` is a flat hack that consumes trailing identifiers. **Replace it entirely** with a proper precedence-climbing / recursive descent expression parser:

- `parseExpression()` -> calls `parseOrExpr()`
- `parseOrExpr()` -> `parseAndExpr() ("or" parseAndExpr())*`
- `parseAndExpr()` -> `parseNotExpr() ("and" parseNotExpr())*`
- `parseNotExpr()` -> `"not" parseNotExpr() | parseCompExpr()`
- `parseCompExpr()` -> `parseAddExpr() (("equals"|"greater than"|"less than") parseAddExpr())?`
- `parseAddExpr()` -> `parseMulExpr() (("plus"|"minus") parseMulExpr())*`
- `parseMulExpr()` -> `parsePrimary() (("times"|"divided by"|"modulo") parsePrimary())*`
- `parsePrimary()` -> Number | String | true | false | Identifier | Hole | `(` Expression `)` | JsonObject

Also add parsing for:
- `parseStatement()` must handle `PRINT`, `SET`, `REPEAT` tokens.
- `parsePrintDecl()`: consumes `print` then an expression.
- `parseSetDecl()`: consumes `set`, identifier, `be`, expression.
- `parseRepeatDecl()`: consumes `repeat`, `while`, expression, newline, then a list of `- statement` body lines.
- **`parseMatchDecl()`** must accept an **expression** (not just an identifier) after `match`. And `MatchDecl` should store `std::unique_ptr<ExprNode>` for the matched value, not a `std::string name`.
- **`MatchCaseNode` yield** must allow a full **statement** (like `print "hello"`), not just an expression.

### PHASE 4: Build the Interpreter

Create two new files:
- `include/interpreter.h`
- `src/interpreter.cpp`

The interpreter is an `ASTVisitor` subclass called `Interpreter`. It walks the AST and executes it.

**Runtime value representation:**

Use `std::variant<double, std::string, bool>` as the `LumoValue` type. Store variables in a `std::unordered_map<std::string, LumoValue>` environment.

**Execution rules:**

| AST Node | Behavior |
|---|---|
| `IntentDecl` | Print `[INTENT] <text>` to stderr |
| `LetDecl` | Evaluate the expression, store in env |
| `SetDecl` | Evaluate expression, update existing var in env |
| `PrintDecl` | Evaluate expression, print result to stdout |
| `NumberExpr` | Return `double` value |
| `StringLiteralExpr` | Return `string` value |
| `BoolLiteralExpr` | Return `bool` value |
| `IdentifierExpr` | Look up variable in env, return its value |
| `BinaryExpr` | Evaluate left and right, apply operator |
| `UnaryExpr` | Evaluate operand, apply `not` |
| `HoleExpr` | Print `[HOLE] Unresolved: "<instruction>"` to stderr, halt |
| `JsonObjectExpr` | Return string representation (for now) |
| `PipeDecl` | Execute steps sequentially, `start with` loads a value into a `pipe_value` register, `transform` applies expression to it (using a special `value` identifier that refers to `pipe_value`), `yield result` stores `pipe_value` into env as `result` |
| `MatchDecl` | Evaluate the matched expression, compare to each `when` case, execute the first matching case's yield statement. If none match and a `catch error` exists, execute that. |
| `RepeatDecl` | Evaluate condition. While truthy, execute body statements. Re-evaluate condition after each iteration. |

### PHASE 5: Update `main.cpp`

After parsing, instead of printing the AST, create an `Interpreter` instance and call `interpreter.execute(*program)`.

Add a `--ast` flag: if the user runs `./lumo --ast file.lumo`, print the AST. Otherwise, execute the program.

### PHASE 6: Create Example Files

Create a folder `examples/` with these files:

**`examples/hello.lumo`:**

```lumo
intent "Print hello world"
print "Hello, World!"
```

**`examples/arithmetic.lumo`:**

```lumo
intent "Basic arithmetic operations"
let a be 5
let b be 10
let sum be a plus b
print sum

let product be a times b
print product

let complex be a plus b times 2
print complex
```

Expected output:

```
15
50
25
```

**`examples/loop.lumo`:**

```lumo
intent "Count from 0 to 4"
let i be 0
repeat while i less than 5
- print i
- set i be i plus 1
```

Expected output:

```
0
1
2
3
4
```

**`examples/fizzbuzz.lumo`:**

```lumo
intent "Classic FizzBuzz from 1 to 20"
let i be 1
repeat while i less than 21
- let mod3 be i modulo 3
- let mod5 be i modulo 5
- let mod15 be i modulo 15
- match mod15
  - when 0 yield print "FizzBuzz"
  - catch error yield match mod3
    - when 0 yield print "Fizz"
    - catch error yield match mod5
      - when 0 yield print "Buzz"
      - catch error yield print i
- set i be i plus 1
```

---

## Critical Rules

1. **All code must be C++17.** Compile command: `clang++ -std=c++17 -Iinclude src/*.cpp -o lumo`
2. **Do NOT use any external libraries.** Everything is hand-rolled from scratch.
3. **Do NOT break backward compatibility.** The existing `test.lumo` file must still parse without errors (though some constructs like `pipe` may now also execute).
4. **Do NOT add code comments explaining what each line does.** Only add comments for non-obvious logic.
5. **Make sure the `???` (HOLE) token is handled correctly.** The three question marks `???` are a trigraph in older C++ standards. Use string escaping or pragma to avoid trigraph warnings.
6. **The interpreter must halt cleanly on Holes.** Print the hole instruction to stderr and exit with code 1.
7. **Output files as complete replacements.** For each file you modify, output the entire file content, not a diff.
8. **Test mentally.** Before outputting, trace through `examples/arithmetic.lumo` and `examples/loop.lumo` to verify correctness.

---

## File Tree After Changes

```
.
├── CMakeLists.txt
├── include/
│   ├── ast.h           (MODIFIED - new node types)
│   ├── ast_printer.h   (MODIFIED - new visit methods)
│   ├── interpreter.h   (NEW)
│   ├── lexer.h         (MODIFIED - new token types)
│   └── parser.h        (MODIFIED - new parse methods)
├── src/
│   ├── ast.cpp         (MODIFIED - new accept methods)
│   ├── ast_printer.cpp (MODIFIED - new visit implementations)
│   ├── interpreter.cpp (NEW)
│   ├── lexer.cpp       (MODIFIED - new keywords)
│   ├── main.cpp        (MODIFIED - interpreter integration)
│   └── parser.cpp      (MODIFIED - full expression parser + new statements)
├── examples/
│   ├── hello.lumo
│   ├── arithmetic.lumo
│   ├── loop.lumo
│   └── fizzbuzz.lumo
└── test.lumo           (UNCHANGED)
```

## Output Format

Output every file listed in the file tree above, one by one, with its full path and complete contents. Start with the header files, then source files, then example files.
