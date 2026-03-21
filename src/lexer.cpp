#include "lexer.h"
#include <cctype>
#include <unordered_map>

std::string Token::toString() const {
    switch (type) {
        case TokenType::INTENT:       return "INTENT";
        case TokenType::LET:          return "LET";
        case TokenType::BE:           return "BE";
        case TokenType::SET:          return "SET";
        case TokenType::PRINT:        return "PRINT";
        case TokenType::PIPE:         return "PIPE";
        case TokenType::MATCH:        return "MATCH";
        case TokenType::WHEN:         return "WHEN";
        case TokenType::CATCH:        return "CATCH";
        case TokenType::ERROR:        return "ERROR";
        case TokenType::YIELD:        return "YIELD";
        case TokenType::START:        return "START";
        case TokenType::WITH:         return "WITH";
        case TokenType::MAP:          return "MAP";
        case TokenType::FILTER:       return "FILTER";
        case TokenType::TRANSFORM:    return "TRANSFORM";
        case TokenType::USING:        return "USING";
        case TokenType::REPEAT:       return "REPEAT";
        case TokenType::WHILE:        return "WHILE";
        case TokenType::PLUS:         return "PLUS";
        case TokenType::MINUS:        return "MINUS";
        case TokenType::TIMES:        return "TIMES";
        case TokenType::DIVIDED:      return "DIVIDED";
        case TokenType::BY:           return "BY";
        case TokenType::MODULO:       return "MODULO";
        case TokenType::GREATER:      return "GREATER";
        case TokenType::LESS:         return "LESS";
        case TokenType::THAN:         return "THAN";
        case TokenType::EQUALS:       return "EQUALS";
        case TokenType::AND:          return "AND";
        case TokenType::OR:           return "OR";
        case TokenType::NOT:          return "NOT";
        case TokenType::TRUE_LIT:     return "TRUE";
        case TokenType::FALSE_LIT:    return "FALSE";
        case TokenType::VALUE:        return "VALUE";
        case TokenType::RESULT:       return "RESULT";
        case TokenType::HOLE:         return "HOLE";
        case TokenType::DASH:         return "DASH";
        case TokenType::LBRACKET:     return "LBRACKET";
        case TokenType::RBRACKET:     return "RBRACKET";
        case TokenType::LBRACE:       return "LBRACE";
        case TokenType::RBRACE:       return "RBRACE";
        case TokenType::LPAREN:       return "LPAREN";
        case TokenType::RPAREN:       return "RPAREN";
        case TokenType::COLON:        return "COLON";
        case TokenType::COMMA:        return "COMMA";
        case TokenType::IDENTIFIER:   return "IDENTIFIER(" + value + ")";
        case TokenType::STRING_LITERAL: return "STRING(\"" + value + "\")";
        case TokenType::NUMBER:       return "NUMBER(" + value + ")";
        case TokenType::NEWLINE:      return "NEWLINE";
        case TokenType::END_OF_FILE:  return "EOF";
        default:                      return "UNKNOWN(" + value + ")";
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
    advance(); // consume opening "
    std::string value;
    while (!isAtEnd() && peek() != '"') {
        value += advance();
    }
    if (isAtEnd()) {
        return Token{TokenType::UNKNOWN, value, startLine, startCol};
    }
    advance(); // consume closing "
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
        {"intent",     TokenType::INTENT},
        {"let",        TokenType::LET},
        {"be",         TokenType::BE},
        {"set",        TokenType::SET},
        {"print",      TokenType::PRINT},
        {"pipe",       TokenType::PIPE},
        {"match",      TokenType::MATCH},
        {"when",       TokenType::WHEN},
        {"catch",      TokenType::CATCH},
        {"error",      TokenType::ERROR},
        {"yield",      TokenType::YIELD},
        {"start",      TokenType::START},
        {"with",       TokenType::WITH},
        {"map",        TokenType::MAP},
        {"filter",     TokenType::FILTER},
        {"transform",  TokenType::TRANSFORM},
        {"using",      TokenType::USING},
        {"repeat",     TokenType::REPEAT},
        {"while",      TokenType::WHILE},
        {"plus",       TokenType::PLUS},
        {"minus",      TokenType::MINUS},
        {"times",      TokenType::TIMES},
        {"divided",    TokenType::DIVIDED},
        {"by",         TokenType::BY},
        {"modulo",     TokenType::MODULO},
        {"greater",    TokenType::GREATER},
        {"less",       TokenType::LESS},
        {"than",       TokenType::THAN},
        {"equals",     TokenType::EQUALS},
        {"and",        TokenType::AND},
        {"or",         TokenType::OR},
        {"not",        TokenType::NOT},
        {"true",       TokenType::TRUE_LIT},
        {"false",      TokenType::FALSE_LIT},
        {"value",      TokenType::VALUE},
        {"result",     TokenType::RESULT},
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

    // Handle ??? (three question marks) — avoid C trigraph issues by checking char by char
    if (c == '?') {
        advance();
        if (!isAtEnd() && peek() == '?') {
            advance();
            if (!isAtEnd() && peek() == '?') {
                advance();
                return Token{TokenType::HOLE, "???", startLine, startCol};
            }
        }
        return Token{TokenType::UNKNOWN, "?", startLine, startCol};
    }

    advance();
    switch (c) {
        case '-': return Token{TokenType::DASH,     "-", startLine, startCol};
        case '[': return Token{TokenType::LBRACKET, "[", startLine, startCol};
        case ']': return Token{TokenType::RBRACKET, "]", startLine, startCol};
        case '{': return Token{TokenType::LBRACE,   "{", startLine, startCol};
        case '}': return Token{TokenType::RBRACE,   "}", startLine, startCol};
        case '(': return Token{TokenType::LPAREN,   "(", startLine, startCol};
        case ')': return Token{TokenType::RPAREN,   ")", startLine, startCol};
        case ':': return Token{TokenType::COLON,    ":", startLine, startCol};
        case ',': return Token{TokenType::COMMA,    ",", startLine, startCol};
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
