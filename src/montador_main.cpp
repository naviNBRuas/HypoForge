// montador: assembles one .asm file into a .OBJ file.
//
//   ./montador programa.asm            -> programa.OBJ (single line of words)
//   ./montador --modular programa.asm  -> programa.OBJ with D/U/R header (used by ligador)

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "assembler.hpp"
#include "objfile.hpp"

namespace {

void usage() {
    std::cerr << "Uso: montador [--modular] arquivo.asm\n"
                 "  Gera arquivo.OBJ (mesmo nome, extensao trocada por .OBJ).\n"
                 "  --modular  gera o objeto com tabelas D/U/R (usado pelo ligador).\n";
}

}  // namespace

int main(int argc, char** argv) {
    bool modular = false;
    std::string input;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--modular") modular = true;
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else if (input.empty()) input = a;
        else { usage(); return 2; }
    }
    if (input.empty()) { usage(); return 2; }

    std::vector<std::string> lines;
    // Convenience: "./montador prog" also finds "prog.asm" when "prog" itself does not exist.
    if (!sb::readLines(input, lines) && sb::replaceExtension(input, "") == input &&
        sb::readLines(input + ".asm", lines))
        input += ".asm";
    if (lines.empty() && !sb::readLines(input, lines)) {
        std::cout << "Erro: nao foi possivel abrir o arquivo '" << input << "'\n";
        return 2;
    }

    const sb::AssemblyResult result = sb::assemble(lines);

    const std::string out = sb::replaceExtension(input, ".OBJ");

    if (!result.ok()) {
        std::remove(out.c_str());  // never leave a stale object from an earlier run
        for (const auto& d : result.diagnostics) {
            if (d.line > 0) {
                std::cout << "[ERRO " << sb::errorTypeName(d.type) << "] linha " << d.line << ": " << d.message << "\n";
                const std::string& src = lines[static_cast<std::size_t>(d.line) - 1];
                std::cout << "    " << d.line << " | " << src << "\n";
            } else {
                std::cout << "[ERRO " << sb::errorTypeName(d.type) << "] arquivo: " << d.message << "\n";
            }
        }
        std::cout << "Montagem de '" << input << "' falhou: " << result.diagnostics.size()
                  << " erro(s). Nenhum arquivo objeto foi gerado.\n";
        return 1;
    }

    if (!modular && !result.uses.empty())
        std::cerr << "Aviso: '" << input << "' usa simbolos EXTERN; o objeto so e executavel apos a ligacao (use o ligador).\n";

    const std::string text = modular ? sb::formatModuleObject(result) : sb::formatPlainObject(result.words);
    if (!sb::writeWholeFile(out, text)) {
        std::cout << "Erro: nao foi possivel escrever '" << out << "'\n";
        return 2;
    }
    std::cout << "Montagem concluida: " << out << " (" << result.words.size() << " palavras)\n";
    return 0;
}
