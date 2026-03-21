#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TokenType {
    // ── Declaration statements ────────────────────────────────────────────────
    INTENT,
    LET,
    BE,
    SET,
    PRINT,
    SKIP,
    READ,         // Fix 6: read <name>

    // ── Block constructs ──────────────────────────────────────────────────────
    REPEAT,
    WHILE,
    FOR,          // Fix 2: for each … in …
    EACH,         // Fix 2
    IN,           // Fix 2
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
    END,          // terminates repeat / pipe / match / if / define blocks

    // ── Conditional construct ─────────────────────────────────────────────────
    IF,
    THEN,
    ELIF,
    ELSE,

    // ── Functions ─────────────────────────────────────────────────────────────
    DEFINE,
    TAKING,
    RETURN_KW,
    CALL_KW,
    PASSING,

    // ── Lists ─────────────────────────────────────────────────────────────────
    LIST_KW,
    AT,
    PUT,
    GET_KW,

    // ── Arithmetic and logic operators (all single tokens) ────────────────────
    PLUS,
    MINUS,
    TIMES,
    DIVBY,        // "divided by" → single token
    MODULO,
    ABOVE,        // strictly greater than
    BELOW,        // strictly less than
    ATLEAST,      // Fix 1: greater or equal (>=)
    ATMOST,       // Fix 1: less or equal (<=)
    EQUALS,
    AND,
    OR,
    NOT,

    // ── Boolean literals ──────────────────────────────────────────────────────
    TRUE_LIT,
    FALSE_LIT,

    // ── Reserved pipe registers ───────────────────────────────────────────────
    VALUE,        // active pipe element register (Fix 3: RESULT removed)

    // ── Punctuation ───────────────────────────────────────────────────────────
    HOLE,         // ???
    LBRACKET,     // [
    RBRACKET,     // ]
    LBRACE,       // {
    RBRACE,       // }
    LPAREN,       // (
    RPAREN,       // )
    COLON,        // :
    COMMA,        // ,

    // ── Literals and identifiers ──────────────────────────────────────────────
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,

    // ── Control ───────────────────────────────────────────────────────────────
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
