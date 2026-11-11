// ligador: links up to three .asm modules into one executable object (.LIG).
//
//   ./ligador prog1.asm prog2.asm prog3.asm   -> prog1.LIG
//
// Each module is assembled by calling the `montador` executable (kept next to this binary).

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "linker.hpp"
#include "objfile.hpp"

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace {

bool fileExists(const std::string& path) {
    if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
        std::fclose(f);
        return true;
    }
    return false;
}

// Finds the montador executable: next to this ligador, then in the current directory, then on PATH.
// (Plain string handling instead of <filesystem>, which needs -lstdc++fs on older g++/MinGW.)
std::string montadorCommand(const std::string& argv0) {
#ifdef _WIN32
    const std::string exe = "montador.exe";
#else
    const std::string exe = "montador";
#endif
    const auto slash = argv0.find_last_of("/\\");
    if (slash != std::string::npos) {
        const std::string candidate = argv0.substr(0, slash + 1) + exe;
        if (fileExists(candidate)) return candidate;
    }
#ifdef _WIN32
    if (fileExists(exe)) return ".\\" + exe;
#else
    if (fileExists(exe)) return "./" + exe;
#endif
    return exe;  // fall back to PATH
}

int runMontador(const std::string& montador, const std::string& asmFile) {
    std::string cmd = "\"" + montador + "\" --modular \"" + asmFile + "\"";
#ifdef _WIN32
    cmd = "\"" + cmd + "\"";
#endif
    const int status = std::system(cmd.c_str());
#ifdef _WIN32
    return status;
#else
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Uso: ligador prog1.asm [prog2.asm] [prog3.asm]   (no maximo 3 modulos)\n";
        return 2;
    }
    std::vector<std::string> asmFiles(argv + 1, argv + argc);
    const std::string montador = montadorCommand(argv[0]);
    const std::string out = sb::replaceExtension(asmFiles[0], ".LIG");
    std::remove(out.c_str());  // never leave a stale executable from an earlier run

    // 1) Assemble every module separately.
    std::vector<sb::ModuleObject> modules;
    for (const auto& f : asmFiles) {
        const int rc = runMontador(montador, f);
        if (rc != 0) {
            std::cout << "Ligacao abortada: a montagem de '" << f << "' falhou.\n";
            return 1;
        }
        std::string text, error;
        sb::ModuleObject obj;
        const std::string objPath = sb::replaceExtension(f, ".OBJ");
        if (!sb::readWholeFile(objPath, text) || !sb::parseModuleObject(text, obj, error)) {
            std::cout << "Erro ao ler o objeto '" << objPath << "': " << (error.empty() ? "arquivo ilegivel" : error) << "\n";
            return 1;
        }
        modules.push_back(obj);
    }

    // 2) Link.
    const sb::LinkResult res = sb::linkModules(modules, asmFiles);
    if (!res.ok()) {
        for (const auto& e : res.errors) std::cout << "[ERRO LIGACAO] " << e << "\n";
        std::cout << "Ligacao falhou: " << res.errors.size() << " erro(s). Nenhum arquivo .LIG foi gerado.\n";
        return 1;
    }

    for (std::size_t i = 0; i < asmFiles.size(); ++i)
        std::cout << "  modulo " << asmFiles[i] << ": base " << res.bases[i] << ", " << modules[i].code.size() << " palavras\n";
    for (const auto& s : res.symbols)
        std::cout << "  simbolo " << s.name << " (" << s.module << ") = " << s.address << "\n";

    if (!sb::writeWholeFile(out, sb::formatPlainObject(res.code))) {
        std::cout << "Erro: nao foi possivel escrever '" << out << "'\n";
        return 2;
    }
    std::cout << "Ligacao concluida: " << out << " (" << res.code.size() << " palavras)\n";
    return 0;
}
