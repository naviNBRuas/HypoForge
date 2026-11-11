#include "lexer.hpp"

#include <cctype>

#include "common.hpp"

namespace sb {

namespace {
bool isWordChar(unsigned char c) { return std::isalnum(c) || c == '_'; }
}  // namespace

LexResult tokenizeLine(const std::string& raw) {
    LexResult out;
    const std::size_t n = raw.size();
    std::size_t i = 0;

    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(raw[i]);

        if (c == ';') break;  // comment until end of line
        if (std::isspace(c)) { ++i; continue; }

        if (c == ':') { out.tokens.push_back({Token::Kind::Colon, ":", 0}); ++i; continue; }
        if (c == '+') { out.tokens.push_back({Token::Kind::Plus, "+", 0}); ++i; continue; }
        if (c == '-') { out.tokens.push_back({Token::Kind::Minus, "-", 0}); ++i; continue; }
        if (c == ',') { out.tokens.push_back({Token::Kind::Comma, ",", 0}); ++i; continue; }

        if (c < 0x80 && isWordChar(c)) {
            std::size_t j = i;
            while (j < n && static_cast<unsigned char>(raw[j]) < 0x80 &&
                   isWordChar(static_cast<unsigned char>(raw[j])))
                ++j;
            const std::string word = raw.substr(i, j - i);
            i = j;

            bool allDigits = true;
            for (char ch : word)
                if (!std::isdigit(static_cast<unsigned char>(ch))) { allDigits = false; break; }

            if (allDigits) {
                if (word.size() > 18) {
                    out.errors.push_back("numero '" + word.substr(0, 12) + "...' fora do intervalo permitido");
                } else {
                    out.tokens.push_back({Token::Kind::Number, word, std::stoll(word)});
                }
            } else if (std::isdigit(static_cast<unsigned char>(word[0]))) {
                out.errors.push_back("token invalido '" + word +
                                     "': nao e um identificador valido (nao pode comecar com numero) nem um numero decimal");
            } else if (word.size() > kMaxIdentifierLength) {
                out.errors.push_back("identificador '" + word.substr(0, 12) + "...' excede " +
                                     std::to_string(kMaxIdentifierLength) + " caracteres");
            } else {
                out.tokens.push_back({Token::Kind::Ident, toUpper(word), 0});
            }
            continue;
        }

        // Anything else is not part of the language's alphabet.
        if (c >= 0x80) {
            out.errors.push_back("caractere nao ASCII fora de comentario");
            ++i;
            while (i < n && (static_cast<unsigned char>(raw[i]) & 0xC0) == 0x80) ++i;  // skip UTF-8 tail
        } else {
            out.errors.push_back(std::string("caractere invalido '") + static_cast<char>(c) + "'");
            ++i;
        }
    }
    return out;
}

}  // namespace sb
