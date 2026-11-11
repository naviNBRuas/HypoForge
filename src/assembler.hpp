// Two-pass assembler for the course's hypothetical machine.
#pragma once

#include <string>
#include <vector>

#include "common.hpp"

namespace sb {

/// A PUBLIC symbol exported by a module: name -> address inside the module.
struct Definition {
    std::string name;
    int address = 0;
};

/// A use of an EXTERN symbol: the word at `address` must receive the symbol's final address.
struct Use {
    std::string name;
    int address = 0;
};

struct AssemblyResult {
    std::vector<Diagnostic> diagnostics;   // empty when assembly succeeded
    std::vector<long long> words;          // object code: program first, then CONST/SPACE data
    std::vector<int> relocation;           // 1 per word; 1 = address that depends on the load base
    std::vector<Definition> definitions;   // PUBLIC symbols
    std::vector<Use> uses;                 // EXTERN symbol uses
    std::string moduleName;                // label of BEGIN (empty when absent)
    bool isModule = false;                 // source contains BEGIN
    bool ok() const { return diagnostics.empty(); }
};

/// Assembles a program given as its source lines (without line terminators).
/// The whole program is always analysed so that every error can be reported.
AssemblyResult assemble(const std::vector<std::string>& sourceLines);

}  // namespace sb
