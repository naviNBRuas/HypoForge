// Lexical analysis: one source line -> tokens (+ lexical errors).
#pragma once

#include <string>
#include <vector>

namespace sb {

struct Token {
    enum class Kind { Ident, Number, Colon, Plus, Minus, Comma };
    Kind kind;
    std::string text;     // identifiers are upper-cased (the language is case-insensitive)
    long long value = 0;  // for Number
};

struct LexResult {
    std::vector<Token> tokens;
    std::vector<std::string> errors;  // lexical error messages (invalid chars, bad tokens...)
};

/// Tokenises a single line. Everything after ';' is a comment. Tabs/extra blanks are ignored.
LexResult tokenizeLine(const std::string& line);

}  // namespace sb
