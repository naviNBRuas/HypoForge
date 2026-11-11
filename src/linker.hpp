// Linker: merges up to three modular objects into one executable object.
#pragma once

#include <string>
#include <vector>

#include "objfile.hpp"

namespace sb {

struct LinkedSymbol {
    std::string name;
    std::string module;
    int address = 0;  // global address
};

struct LinkResult {
    std::vector<std::string> errors;
    std::vector<long long> code;
    std::vector<int> bases;            // load base of each module
    std::vector<LinkedSymbol> symbols; // global symbol table
    bool ok() const { return errors.empty(); }
};

/// `names` are only used in messages (typically the .asm file names).
LinkResult linkModules(const std::vector<ModuleObject>& modules, const std::vector<std::string>& names);

}  // namespace sb
