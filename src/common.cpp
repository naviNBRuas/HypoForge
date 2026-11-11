#include "common.hpp"

#include <algorithm>
#include <cctype>

namespace sb {

namespace {
const InstructionInfo kInstructions[] = {
    {"ADD", 1, 1, 2},  {"SUB", 2, 1, 2},   {"MULT", 3, 1, 2}, {"DIV", 4, 1, 2},
    {"JMP", 5, 1, 2},  {"JMPN", 6, 1, 2},  {"JMPP", 7, 1, 2}, {"JMPZ", 8, 1, 2},
    {"COPY", 9, 2, 3}, {"LOAD", 10, 1, 2}, {"STORE", 11, 1, 2}, {"INPUT", 12, 1, 2},
    {"OUTPUT", 13, 1, 2}, {"STOP", 14, 0, 1},
};

const char* const kDirectives[] = {"SECTION", "SECAO", "SPACE", "CONST", "EQU",
                                   "IF",      "BEGIN", "END",   "PUBLIC", "EXTERN"};
}  // namespace

const char* errorTypeName(ErrorType type) {
    switch (type) {
        case ErrorType::Lexical: return "LEXICO";
        case ErrorType::Syntactic: return "SINTATICO";
        case ErrorType::Semantic: return "SEMANTICO";
    }
    return "SEMANTICO";
}

const InstructionInfo* findInstruction(const std::string& upperName) {
    for (const auto& info : kInstructions)
        if (upperName == info.mnemonic) return &info;
    return nullptr;
}

bool isDirective(const std::string& upperName) {
    for (const char* d : kDirectives)
        if (upperName == d) return true;
    return false;
}

std::string toUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

}  // namespace sb
