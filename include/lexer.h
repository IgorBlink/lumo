#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TokenType {
    // Keywords
    INTENT,
    LET,
    BE,
    SET,
    PRINT,
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
    REPEAT,
    WHILE,

    // Arithmetic / logic operators
    PLUS,
    MINUS,
    TIMES,
    DIVIDED,
    BY,
    MODULO,
    GREATER,
    LESS,
    THAN,
    EQUALS,
    AND,
    OR,
    NOT,

    // Boolean literals
    TRUE_LIT,
    FALSE_LIT,

    // Special pipe identifiers
    VALUE,
    RESULT,

    // Punctuation and special tokens
    HOLE,      // ???
    DASH,      // -
    LBRACKET,  // [
    RBRACKET,  // ]
    LBRACE,    // {
    RBRACE,    // }
    LPAREN,    // (
    RPAREN,    // )
    COLON,     // :
    COMMA,     // ,

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
