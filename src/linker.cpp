#include "linker.hpp"

#include <map>

namespace sb {

LinkResult linkModules(const std::vector<ModuleObject>& mods, const std::vector<std::string>& names) {
    LinkResult res;

    // 1) Load base of every module = sum of the sizes of the previous ones.
    int base = 0;
    for (const auto& m : mods) {
        res.bases.push_back(base);
        base += static_cast<int>(m.code.size());
    }

    // 2) Global symbol table from the D (definition) entries.
    std::map<std::string, LinkedSymbol> global;
    for (std::size_t i = 0; i < mods.size(); ++i) {
        for (const auto& d : mods[i].definitions) {
            if (d.address < 0 || d.address >= static_cast<int>(mods[i].code.size())) {
                res.errors.push_back("modulo " + names[i] + ": endereco invalido para o simbolo '" + d.name + "'");
                continue;
            }
            auto it = global.find(d.name);
            if (it != global.end()) {
                res.errors.push_back("simbolo '" + d.name + "' definido em mais de um modulo (" +
                                     it->second.module + " e " + names[i] + ")");
                continue;
            }
            global[d.name] = {d.name, names[i], res.bases[i] + d.address};
        }
    }
    for (const auto& kv : global) res.symbols.push_back(kv.second);

    // 3) Relocate and resolve.
    for (std::size_t i = 0; i < mods.size(); ++i) {
        std::vector<long long> code = mods[i].code;
        for (std::size_t k = 0; k < code.size(); ++k)
            if (mods[i].relocation[k]) code[k] += res.bases[i];

        for (const auto& u : mods[i].uses) {
            if (u.address < 0 || u.address >= static_cast<int>(code.size())) {
                res.errors.push_back("modulo " + names[i] + ": uso de '" + u.name + "' em endereco invalido " +
                                     std::to_string(u.address));
                continue;
            }
            auto it = global.find(u.name);
            if (it == global.end()) {
                res.errors.push_back("simbolo externo '" + u.name + "' usado em " + names[i] +
                                     " nao foi definido (PUBLIC) em nenhum modulo");
                continue;
            }
            code[static_cast<std::size_t>(u.address)] += it->second.address;
        }
        res.code.insert(res.code.end(), code.begin(), code.end());
    }
    return res;
}

}  // namespace sb
