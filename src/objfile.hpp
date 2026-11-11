// Object file formats and small file helpers shared by montador and ligador.
#pragma once

#include <string>
#include <vector>

#include "assembler.hpp"

namespace sb {

/// Contents of a modular object file (the format montador emits when called by ligador).
struct ModuleObject {
    std::vector<Definition> definitions;  // D, NAME ADDRESS
    std::vector<Use> uses;                // U, NAME ADDRESS
    std::vector<int> relocation;          // R, bit per word
    std::vector<long long> code;          // single line of words
};

/// Plain executable-style object: ONE line, words separated by blanks (what the spec requires).
std::string formatPlainObject(const std::vector<long long>& words);

/// Modular object: D/U/R header lines followed by the single code line.
std::string formatModuleObject(const AssemblyResult& result);

/// Parses the text produced by formatModuleObject. Returns false and fills `error` on failure.
bool parseModuleObject(const std::string& text, ModuleObject& out, std::string& error);

/// Reads a text file into lines (handles CRLF and a UTF-8 BOM). Returns false when it cannot open.
bool readLines(const std::string& path, std::vector<std::string>& lines);
bool readWholeFile(const std::string& path, std::string& content);
bool writeWholeFile(const std::string& path, const std::string& content);

/// "dir/prog.asm" + ".OBJ" -> "dir/prog.OBJ" (appends when there is no extension).
std::string replaceExtension(const std::string& path, const std::string& newExt);

}  // namespace sb
