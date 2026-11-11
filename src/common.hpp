// Shared types and tables for the assembler (montador) and linker (ligador).
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace sb {

/// Error classes required by the assignment specification.
enum class ErrorType { Lexical, Syntactic, Semantic };

/// Name printed on screen (the specification uses these exact words).
const char* errorTypeName(ErrorType type);

/// One reported error. `line == 0` means "whole file" (e.g. missing TEXT section).
struct Diagnostic {
    int line = 0;
    ErrorType type = ErrorType::Semantic;
    std::string message;
};

/// Row of the instruction table of the hypothetical machine.
struct InstructionInfo {
    const char* mnemonic;
    int opcode;
    int operands;  // number of operands in the source
    int size;      // size in words once assembled (1 + operands)
};

/// Looks up an (already upper-cased) mnemonic; nullptr when it is not an instruction.
const InstructionInfo* findInstruction(const std::string& upperName);

/// True for SECTION/SECAO, SPACE, CONST, EQU, IF, BEGIN, END, PUBLIC, EXTERN.
bool isDirective(const std::string& upperName);

std::string toUpper(std::string text);

/// Identifiers (labels, variables) are limited to 30 characters.
constexpr std::size_t kMaxIdentifierLength = 30;

}  // namespace sb
